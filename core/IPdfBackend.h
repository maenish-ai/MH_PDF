#pragma once
#include <QString>
#include <QSizeF>
#include <QSize>
#include <QImage>
#include <QVariantList>
#include <QVariantMap>
#include <QPointF>

class IPdfBackend {
public:
    virtual ~IPdfBackend() = default;
    virtual QString name() const = 0;
    virtual bool open(const QString &path, const QString &password = QString(), QString *error = nullptr) = 0;
    virtual int pageCount() const = 0;
    virtual QSizeF pagePointSize(int page) const = 0;
    virtual QImage render(int page, const QSize &pixels) const = 0;
    virtual QString pageText(int page) const = 0;
    virtual QVariantList search(const QString &needle, int maxResults = 200) const = 0;
    virtual QVariantMap textSelection(int page, const QPointF &start, const QPointF &end) const = 0;
};
