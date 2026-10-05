#pragma once

#include <QQuickImageProvider>

class DocumentManager;

class SessionImageProvider final : public QQuickImageProvider {
public:
    explicit SessionImageProvider(DocumentManager *manager);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
private:
    DocumentManager *m_manager{nullptr};
};
