#pragma once
#include "IPdfBackend.h"
#include <QPdfDocument>

class QtPdfBackend final : public IPdfBackend {
public:
    QString name() const override { return "Qt PDF Structural Reader"; }
    bool open(const QString &path, const QString &password = QString(), QString *error = nullptr) override;
    int pageCount() const override { return m_doc.pageCount(); }
    QSizeF pagePointSize(int page) const override { return m_doc.pagePointSize(page); }
    QImage render(int page, const QSize &pixels) const override { return m_doc.render(page, pixels); }
    QString pageText(int page) const override;
    QVariantList search(const QString &needle, int maxResults = 200) const override;
private:
    // Qt 6.11 render()/getAllText() update internal caches even for logical read operations.
    mutable QPdfDocument m_doc;
};
