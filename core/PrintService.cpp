#include "PrintService.h"
#include "PdfDocument.h"
#include "PageModel.h"
#include "MemoryPolicy.h"

#ifndef Q_OS_ANDROID
#include <QAbstractPrintDialog>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
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

bool PrintService::printDocument(QObject *documentObject, bool currentPageOnly, int scalePercent) {
#ifdef Q_OS_ANDROID
    Q_UNUSED(documentObject)
    Q_UNUSED(currentPageOnly)
    Q_UNUSED(scalePercent)
    emit operationFailed(QStringLiteral("print.error.unavailable"), {});
    return false;
#else
    auto *document = qobject_cast<PdfDocument *>(documentObject);
    if (!document || document->pageCount() <= 0) {
        emit operationFailed(QStringLiteral("print.error.no_document"), {});
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(document->title());
    printer.setFullPage(false);

    QPrintDialog dialog(&printer);
    dialog.setWindowTitle(QStringLiteral("MaenPDF"));
    dialog.setMinMax(1, document->pageCount());
    dialog.setFromTo(currentPageOnly ? document->currentPage() + 1 : 1,
                     currentPageOnly ? document->currentPage() + 1 : document->pageCount());
    dialog.setOption(QAbstractPrintDialog::PrintPageRange, true);

    if (dialog.exec() != QDialog::Accepted)
        return true;

    const bool ok = paintDocument(document, &printer, currentPageOnly, scalePercent);
    if (ok)
        emit operationFinished(QStringLiteral("print.done"), {});
    else
        emit operationFailed(QStringLiteral("print.error.failed"), {});
    return ok;
#endif
}

bool PrintService::printPreview(QObject *documentObject, bool currentPageOnly, int scalePercent) {
#ifdef Q_OS_ANDROID
    Q_UNUSED(documentObject)
    Q_UNUSED(currentPageOnly)
    Q_UNUSED(scalePercent)
    emit operationFailed(QStringLiteral("print.error.unavailable"), {});
    return false;
#else
    auto *document = qobject_cast<PdfDocument *>(documentObject);
    if (!document || document->pageCount() <= 0) {
        emit operationFailed(QStringLiteral("print.error.no_document"), {});
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(document->title());
    printer.setFullPage(false);
    QPrintPreviewDialog preview(&printer);
    preview.setWindowTitle(QStringLiteral("MaenPDF"));
    connect(&preview, &QPrintPreviewDialog::paintRequested, this,
            [this, document, currentPageOnly, scalePercent](QPrinter *requestedPrinter) {
                paintDocument(document, requestedPrinter, currentPageOnly, scalePercent);
            });
    preview.exec();
    return true;
#endif
}

#ifndef Q_OS_ANDROID
bool PrintService::paintDocument(PdfDocument *document, QPrinter *printer, bool currentPageOnly, int scalePercent) const {
    if (!document || !printer || !document->pageModel() || document->pageCount() <= 0)
        return false;

    int first = currentPageOnly ? document->currentPage() : 0;
    int last = currentPageOnly ? document->currentPage() : document->pageCount() - 1;

    if (!currentPageOnly && printer->printRange() == QPrinter::PageRange) {
        first = qBound(0, printer->fromPage() - 1, document->pageCount() - 1);
        last = qBound(first, printer->toPage() - 1, document->pageCount() - 1);
    }

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
    for (int pageIndex = first; pageIndex <= last; ++pageIndex) {
        if (pageIndex > first && !printer->newPage()) {
            ok = false;
            break;
        }

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
    painter.end();
    return ok;
}
#endif
