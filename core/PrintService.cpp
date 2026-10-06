#include "PrintService.h"
#include "PdfDocument.h"
#include "PageModel.h"

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

bool PrintService::printDocument(QObject *documentObject, bool currentPageOnly) {
#ifdef Q_OS_ANDROID
    Q_UNUSED(documentObject)
    Q_UNUSED(currentPageOnly)
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

    const bool ok = paintDocument(document, &printer, currentPageOnly);
    if (ok)
        emit operationFinished(QStringLiteral("print.done"), {});
    else
        emit operationFailed(QStringLiteral("print.error.failed"), {});
    return ok;
#endif
}

bool PrintService::printPreview(QObject *documentObject, bool currentPageOnly) {
#ifdef Q_OS_ANDROID
    Q_UNUSED(documentObject)
    Q_UNUSED(currentPageOnly)
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
    QPrintPreviewDialog preview(&printer);
    preview.setWindowTitle(QStringLiteral("MaenPDF"));
    connect(&preview, &QPrintPreviewDialog::paintRequested, this,
            [this, document, currentPageOnly](QPrinter *requestedPrinter) {
                paintDocument(document, requestedPrinter, currentPageOnly);
            });
    preview.exec();
    return true;
#endif
}

#ifndef Q_OS_ANDROID
bool PrintService::paintDocument(PdfDocument *document, QPrinter *printer, bool currentPageOnly) const {
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

    bool ok = true;
    for (int pageIndex = first; pageIndex <= last; ++pageIndex) {
        if (pageIndex > first && !printer->newPage()) {
            ok = false;
            break;
        }

        const QRect paintRect = printer->pageLayout().paintRectPixels(printer->resolution());
        QSize renderTarget = paintRect.size();
        renderTarget.setWidth(qBound(800, renderTarget.width(), 4200));
        renderTarget.setHeight(qBound(800, renderTarget.height(), 6000));
        const QImage page = document->pageModel()->renderPage(pageIndex, renderTarget);
        if (page.isNull()) {
            ok = false;
            break;
        }

        QSize target = page.size();
        target.scale(paintRect.size(), Qt::KeepAspectRatio);
        const QRect destination(
            paintRect.x() + (paintRect.width() - target.width()) / 2,
            paintRect.y() + (paintRect.height() - target.height()) / 2,
            target.width(), target.height());
        painter.drawImage(destination, page);
    }
    painter.end();
    return ok;
}
#endif
