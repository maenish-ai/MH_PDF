#pragma once

#include <QQuickImageProvider>

class PageModel;

class PageImageProvider final : public QQuickImageProvider {
public:
    explicit PageImageProvider(PageModel *model);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
private:
    PageModel *m_model{nullptr};
};
