#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

struct TextOverlayItem {
    QString id;
    QString text;
    double x{0.15};       // normalized baseline x
    double y{0.20};       // normalized baseline y
    double width{0.10};   // normalized selection bounds
    double height{0.03};  // normalized selection bounds
    int fontSize{18};
    QString color{QStringLiteral("#111827")};
    int strikeStart{-1};
    int strikeLength{0};
};

struct FormOverlayItem {
    QString id;
    QString type{QStringLiteral("text")}; // text, checkbox, radio, dropdown
    QString label;
    QString value;
    QStringList options;
    double x{0.15};
    double y{0.20};
    double width{0.25};
    double height{0.045};
    bool checked{false};
    int selectedIndex{0};
};

struct PageItem {
    QImage base;
    QImage overlay;
    QSizeF points{595, 842};
    QString label;
    QString sourceId;
    int sourcePage{-1};
    int rotation{0};
    QString uid;
    int revision{0};
    QString watermarkText;
    int watermarkFontSize{42};
    int watermarkOpacity{0};
    bool pageNumber{false};
    QString batesText;
    QVector<TextOverlayItem> textItems;
    QVector<FormOverlayItem> formItems;

    bool sourceBacked() const { return !sourceId.isEmpty() && sourcePage >= 0 && base.isNull(); }
};

class PageModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int modelRevision READ modelRevision NOTIFY modelRevisionChanged)
public:
    enum Roles { ImageRole = Qt::UserRole + 1, LabelRole, WidthRole, HeightRole };
    using RenderFunction = std::function<QImage(const PageItem &, const QSize &)>;

    explicit PageModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex & = {}) const override { return m_pages.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRenderer(RenderFunction renderer);
    void setProviderNamespace(const QString &value);
    QString providerNamespace() const { return m_providerNamespace; }
    void refreshMemoryPolicy();
    void clear();
    void append(PageItem page);
    void insert(int at, PageItem page);
    void remove(int at);
    void movePage(int from, int to);
    PageItem *page(int index);
    const PageItem *page(int index) const;
    int count() const { return m_pages.size(); }
    int modelRevision() const { return m_modelRevision; }
    void changed(int index);

    Q_INVOKABLE QString imageSource(int row) const { return imageUrlForRow(row); }
    Q_INVOKABLE double pageWidth(int row) const { const auto *p = page(row); return p ? p->points.width() : 595.0; }
    Q_INVOKABLE double pageHeight(int row) const { const auto *p = page(row); return p ? p->points.height() : 842.0; }
    Q_INVOKABLE double maxPageWidth() const;
    Q_INVOKABLE double maxPageHeight() const;
    QImage renderPage(int index, const QSize &requestedSize = {}) const;
    QVariantMap cacheStats() const;
    void clearCache() const;

signals:
    void modelRevisionChanged();

private:
    struct CacheEntry { QImage image; qint64 bytes{0}; };
    QString imageUrlForRow(int row) const;
    QString cacheKey(int row, const QSize &size) const;
    QSize defaultPixelSize(const PageItem &page) const;
    void touchCacheKey(const QString &key) const;
    void putCache(const QString &key, const QImage &image) const;
    void ensureIdentity(PageItem &page);

    QVector<PageItem> m_pages;
    RenderFunction m_renderer;
    mutable QHash<QString, CacheEntry> m_cache;
    mutable QList<QString> m_lru;
    mutable qint64 m_cacheBytes{0};
    mutable QMutex m_cacheMutex;
    qint64 m_cacheBudget{0};
    int m_modelRevision{0};
    QString m_providerNamespace{QStringLiteral("0")};
};
