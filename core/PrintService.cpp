#include "PrintService.h"
#include "PdfDocument.h"
#include "PageModel.h"
#include "MemoryPolicy.h"

#ifndef Q_OS_ANDROID
#include <QPainter>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPageLayout>
#endif

PrintService::PrintService(QObject *parent) : QObject(parent) {}

bool PrintService::available() const {
#ifdef Q_OS_ANDROID
    return false;
#else
    return true;
#endif
}

QVariantList PrintService::printers() const {
    QVariantList result;
#ifndef Q_OS_ANDROID
    const QString defaultName = QPrinterInfo::defaultPrinterName();
    for (const QString &name : QPrinterInfo::availablePrinterNames()) {
        const QVariantMap entry{{QStringLiteral("name"), name}};
        if (name == defaultName)
            result.prepend(entry);
        else
            result.append(entry);
    }
#endif
    return result;
}

QVariantList PrintService::paperSizes(const QString &printerName) const {
    QVariantList result;
#ifndef Q_OS_ANDROID
    const QPrinterInfo info = QPrinterInfo::printerInfo(printerName);
    const QPageSize defaultSize = info.defaultPageSize();
    for (const QPageSize &size : info.supportedPageSizes()) {
        const QVariantMap entry{{QStringLiteral("name"), size.name()},
                                {QStringLiteral("key"), size.key()}};
        if (size.key() == defaultSize.key())
            result.prepend(entry);
        else
            result.append(entry);
    }
#else
    Q_UNUSED(printerName)
#endif
    return result;
}

bool PrintService::printDocument(QObject *documentObject, const QVariantMap &settings) {
    return runJob(documentObject, settings, false);
}

bool PrintService::printPreview(QObject *documentObject, const QVariantMap &settings) {
    return runJob(documentObject, settings, true);
}

bool PrintService::runJob(QObject *documentObject, const QVariantMap &settings, bool preview) {
#ifdef Q_OS_ANDROID
    Q_UNUSED(documentObject)
    Q_UNUSED(settings)
    Q_UNUSED(preview)
    emit operationFailed(QStringLiteral("print.error.unavailable"), {});
    return false;
#else
    auto *document = qobject_cast<PdfDocument *>(documentObject);
    if (!document || document->pageCount() <= 0) {
        emit operationFailed(QStringLiteral("print.error.no_document"), {});
        return false;
    }

    const QString name = settings.value(QStringLiteral("printer")).toString();
    const QPrinterInfo info = QPrinterInfo::printerInfo(name);
    if ((!preview || !name.isEmpty()) && info.isNull()) {
        emit operationFailed(QStringLiteral("print.error.no_printer"), {});
        return false;
    }

    QPrinter printer(info, QPrinter::HighResolution);
    printer.setDocName(document->title());
    printer.setFullPage(false);
    const QString paperKey = settings.value(QStringLiteral("paper")).toString();
    for (const QPageSize &size : info.supportedPageSizes()) {
        if (size.key() == paperKey) {
            printer.setPageSize(size);
            break;
        }
    }
    printer.setPageOrientation(settings.value(QStringLiteral("landscape")).toBool()
                               ? QPageLayout::Landscape : QPageLayout::Portrait);

    const int first = qBound(1, settings.value(QStringLiteral("from"), 1).toInt(), document->pageCount()) - 1;
    const int last = qBound(first + 1, settings.value(QStringLiteral("to"), document->pageCount()).toInt(),
                            document->pageCount()) - 1;
    const int copies = qBound(1, settings.value(QStringLiteral("copies"), 1).toInt(), 99);
    const int scale = settings.value(QStringLiteral("scale"), 100).toInt();
    const bool driverCopies = printer.supportsMultipleCopies();
    if (driverCopies) {
        printer.setCopyCount(copies);
        printer.setCollateCopies(true);
    }

    if (preview) {
        QPrintPreviewDialog dialog(&printer);
        dialog.setWindowTitle(QStringLiteral("MaenPDF"));
        connect(&dialog, &QPrintPreviewDialog::paintRequested, this,
                [this, document, first, last, scale](QPrinter *requestedPrinter) {
                    paintDocument(document, requestedPrinter, first, last, 1, scale);
                });
        dialog.exec();
        return true;
    }

    const bool ok = paintDocument(document, &printer, first, last, driverCopies ? 1 : copies, scale);
    if (ok)
        emit operationFinished(QStringLiteral("print.done"), {});
    else
        emit operationFailed(QStringLiteral("print.error.failed"), {});
    return ok;
#endif
}

#ifndef Q_OS_ANDROID
bool PrintService::paintDocument(PdfDocument *document, QPrinter *printer, int first, int last,
                                 int copies, int scalePercent) const {
    QPainter painter;
    if (!painter.begin(printer))
        return false;

    // PDF dimensions are in points (72 per inch). The printer painter
    // already starts at the printable origin when fullPage is false.
    const int dpi = printer->resolution();
    const QSize printable = printer->pageLayout().paintRectPixels(dpi).size();
    const PageModel *model = document->pageModel();
    const qreal pixelsPerPoint = dpi / 72.0 * (qBound(10, scalePercent, 400) / 100.0);
    const int maxDimension = MemoryPolicy::maxRenderDimension();
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setClipRect(QRectF(QPointF(), QSizeF(printable)));

    bool ok = true;
    const int pagesPerCopy = last - first + 1;
    for (int sheet = 0; sheet < pagesPerCopy * copies; ++sheet) {
        if (sheet > 0 && !printer->newPage()) {
            ok = false;
            break;
        }

        const int pageIndex = first + sheet % pagesPerCopy;
        const QSizeF points(model->pageWidth(pageIndex), model->pageHeight(pageIndex));
        const QSizeF target = points * pixelsPerPoint;
        QSizeF raster = target;
        if (raster.width() > maxDimension || raster.height() > maxDimension)
            raster.scale(maxDimension, maxDimension, Qt::KeepAspectRatio);
        const QImage page = model->renderPage(pageIndex, raster.toSize());
        if (page.isNull()) {
            ok = false;
            break;
        }

        const QRectF destination((printable.width() - target.width()) / 2,
                                 (printable.height() - target.height()) / 2,
                                 target.width(), target.height());
        painter.drawImage(destination, page);
    }
    return painter.end() && ok;
}
#endif
