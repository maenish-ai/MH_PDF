#pragma once
#include <QString>
#include <QSizeF>
#include <QSize>
#include <QImage>
#include <QVariantList>

class IPdfBackend {
public:
    virtual ~IPdfBackend() = default;
    virtual QString name() const = 0;
    virtual bool open(const QString &path, QString *error = nullptr) = 0;
    virtual int pageCount() const = 0;
    virtual QSizeF pagePointSize(int page) const = 0;
    virtual QImage render(int page, const QSize &pixels) const = 0;
    virtual QString pageText(int page) const = 0;
    virtual QVariantList search(const QString &needle, int maxResults = 200) const = 0;
};
