#include "SessionImageProvider.h"
#include "DocumentManager.h"
#include <QStringList>

SessionImageProvider::SessionImageProvider(DocumentManager *manager)
    : QQuickImageProvider(QQuickImageProvider::Image), m_manager(manager) {}

QImage SessionImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    if (!m_manager)
        return {};
    const QStringList parts = id.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.size() < 3 || parts.at(1) != QStringLiteral("page"))
        return {};
    bool ok = false;
    const int page = parts.at(2).toInt(&ok);
    if (!ok)
        return {};
    QImage image = m_manager->renderPage(parts.at(0), page, requestedSize);
    if (size)
        *size = image.size();
    return image;
}
