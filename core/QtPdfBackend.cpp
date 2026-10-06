#include "QtPdfBackend.h"
#include <QMutexLocker>
#include <QPdfSelection>
#include <QPolygonF>

bool QtPdfBackend::open(const QString &path, const QString &password, QString *error) {
    QMutexLocker<QMutex> locker(&m_mutex);
    m_doc.close();
    if (!password.isEmpty())
        m_doc.setPassword(password);
    const auto e = m_doc.load(path);
    if (e == QPdfDocument::Error::None)
        return true;
    if (error) {
        if (e == QPdfDocument::Error::IncorrectPassword)
            *error = QStringLiteral("password");
        else if (e == QPdfDocument::Error::UnsupportedSecurityScheme)
            *error = QStringLiteral("unsupported-security");
        else
            *error = QStringLiteral("open-failed");
    }
    return false;
}

int QtPdfBackend::pageCount() const {
    QMutexLocker<QMutex> locker(&m_mutex);
    return m_doc.pageCount();
}

QSizeF QtPdfBackend::pagePointSize(int page) const {
    QMutexLocker<QMutex> locker(&m_mutex);
    return (page >= 0 && page < m_doc.pageCount()) ? m_doc.pagePointSize(page) : QSizeF{};
}

QImage QtPdfBackend::render(int page, const QSize &pixels) const {
    QMutexLocker<QMutex> locker(&m_mutex);
    if (page < 0 || page >= m_doc.pageCount())
        return {};
    return m_doc.render(page, pixels);
}

QString QtPdfBackend::pageText(int page) const {
    QMutexLocker<QMutex> locker(&m_mutex);
    if (page < 0 || page >= m_doc.pageCount()) return {};
    return m_doc.getAllText(page).text();
}

QVariantList QtPdfBackend::search(const QString &needle, int maxResults) const {
    QVariantList out;
    const QString q = needle.trimmed();
    if (q.isEmpty() || maxResults <= 0) return out;
    QMutexLocker<QMutex> locker(&m_mutex);
    for (int p=0; p<m_doc.pageCount() && out.size()<maxResults; ++p) {
        const QString text = m_doc.getAllText(p).text();
        int from=0;
        while (out.size()<maxResults) {
            const int at=text.indexOf(q, from, Qt::CaseInsensitive);
            if (at<0) break;
            const int start=qMax(0, at-45), end=qMin(text.size(), at+q.size()+70);
            QString excerpt=text.mid(start,end-start).simplified();
            QVariantMap hit; hit["page"]=p; hit["index"]=at; hit["excerpt"]=excerpt;
            out.push_back(hit); from=at+qMax(1,q.size());
        }
    }
    return out;
}

QVariantMap QtPdfBackend::textSelection(int page, const QPointF &start, const QPointF &end) const {
    QVariantMap result;
    result[QStringLiteral("valid")] = false;
    result[QStringLiteral("text")] = QString();
    result[QStringLiteral("rects")] = QVariantList{};

    QMutexLocker<QMutex> locker(&m_mutex);
    if (page < 0 || page >= m_doc.pageCount())
        return result;

    const QPdfSelection selection = m_doc.getSelection(page, start, end);
    if (!selection.isValid() || selection.text().isEmpty())
        return result;

    QVariantList rects;
    const QSizeF points = m_doc.pagePointSize(page);
    if (points.width() <= 0 || points.height() <= 0)
        return result;

    for (const QPolygonF &polygon : selection.bounds()) {
        const QRectF r = polygon.boundingRect();
        if (r.width() <= 0 || r.height() <= 0)
            continue;
        QVariantMap item;
        item[QStringLiteral("x")] = r.x() / points.width();
        item[QStringLiteral("y")] = r.y() / points.height();
        item[QStringLiteral("w")] = r.width() / points.width();
        item[QStringLiteral("h")] = r.height() / points.height();
        rects.push_back(item);
    }

    result[QStringLiteral("valid")] = !rects.isEmpty();
    result[QStringLiteral("text")] = selection.text();
    result[QStringLiteral("rects")] = rects;
    return result;
}
