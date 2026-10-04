#include "PageImageProvider.h"
#include "PageModel.h"

PageImageProvider::PageImageProvider(PageModel *model)
    : QQuickImageProvider(QQuickImageProvider::Image), m_model(model) {}

QImage PageImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    if (!m_model)
        return {};
    const QStringList parts = id.split('/');
    bool ok = false;
    const int row = parts.size() >= 2 ? parts.at(1).toInt(&ok) : -1;
    if (!ok || row < 0)
        return {};
    QImage image = m_model->renderPage(row, requestedSize);
    if (size)
        *size = image.size();
    return image;
}
