#pragma once
#include "IPdfBackend.h"
#include <QPdfDocument>
#include <QMutex>

class QtPdfBackend final : public IPdfBackend {
public:
    QString name() const override { return "Qt PDF Structural Reader"; }
    bool open(const QString &path, const QString &password = QString(), QString *error = nullptr) override;
    int pageCount() const override;
    QSizeF pagePointSize(int page) const override;
    QImage render(int page, const QSize &pixels) const override;
    QString pageText(int page) const override;
    QVariantList search(const QString &needle, int maxResults = 200) const override;
    QVariantMap textSelection(int page, const QPointF &start, const QPointF &end) const override;
private:
    // Qt 6.11 render()/getAllText() update internal caches even for logical read operations.
    mutable QPdfDocument m_doc;
    mutable QMutex m_mutex;
};
