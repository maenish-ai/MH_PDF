#include "QtPdfBackend.h"
#include <QMutexLocker>
#include <QPdfSelection>
#include <QPolygonF>
#include <limits>

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
    QVariantMap empty;
    empty[QStringLiteral("valid")] = false;
    empty[QStringLiteral("text")] = QString();
    empty[QStringLiteral("rects")] = QVariantList{};

    QMutexLocker<QMutex> locker(&m_mutex);
    if (page < 0 || page >= m_doc.pageCount())
        return empty;

    const QSizeF points = m_doc.pagePointSize(page);
    if (points.width() <= 0 || points.height() <= 0)
        return empty;

    auto selectionToResult = [&points](const QPdfSelection &selection) {
        QVariantMap result;
        QVariantList rects;
        if (selection.isValid() && !selection.text().isEmpty()) {
            for (const QPolygonF &polygon : selection.bounds()) {
                const QRectF r = polygon.boundingRect().normalized();
                if (r.width() <= 0 || r.height() <= 0)
                    continue;
                QVariantMap item;
                item[QStringLiteral("x")] = qBound(0.0, r.x() / points.width(), 1.0);
                item[QStringLiteral("y")] = qBound(0.0, r.y() / points.height(), 1.0);
                item[QStringLiteral("w")] = qBound(0.0, r.width() / points.width(), 1.0);
                item[QStringLiteral("h")] = qBound(0.0, r.height() / points.height(), 1.0);
                rects.push_back(item);
            }
        }
        result[QStringLiteral("valid")] = !rects.isEmpty();
        result[QStringLiteral("text")] = selection.isValid() ? selection.text() : QString();
        result[QStringLiteral("rects")] = rects;
        return result;
    };

    // QPdfDocument::getSelection() expects its endpoints to land on text.
    // A normal mouse drag often begins/ends in the page margin, especially
    // when the user sweeps over an entire line/page. Try the literal points
    // first, then snap whitespace endpoints to the nearest text geometry.
    const QPdfSelection selection = m_doc.getSelection(page, start, end);
    QVariantMap result = selectionToResult(selection);
    if (result.value(QStringLiteral("valid")).toBool())
        return result;

    const QPdfSelection allText = m_doc.getAllText(page);
    if (!allText.isValid() || allText.text().isEmpty() || allText.bounds().isEmpty())
        return empty;

    const QList<QPolygonF> textPolygons = allText.bounds();
    auto snapToTextBounds = [&textPolygons](const QPointF &point) {
        QPointF best = point;
        qreal bestDistance = std::numeric_limits<qreal>::max();
        for (const QPolygonF &polygon : textPolygons) {
            const QRectF rect = polygon.boundingRect().normalized();
            if (rect.width() <= 0 || rect.height() <= 0)
                continue;

            const qreal insetX = qMin<qreal>(0.25, rect.width() * 0.20);
            const qreal insetY = qMin<qreal>(0.25, rect.height() * 0.20);
            qreal left = rect.left() + insetX;
            qreal right = rect.right() - insetX;
            qreal top = rect.top() + insetY;
            qreal bottom = rect.bottom() - insetY;
            if (left > right)
                left = right = rect.center().x();
            if (top > bottom)
                top = bottom = rect.center().y();

            const QPointF candidate(qBound(left, point.x(), right),
                                    qBound(top, point.y(), bottom));
            const qreal dx = candidate.x() - point.x();
            const qreal dy = candidate.y() - point.y();
            const qreal distance = dx * dx + dy * dy;
            if (distance < bestDistance) {
                bestDistance = distance;
                best = candidate;
            }
        }
        return best;
    };

    const QPointF snappedStart = snapToTextBounds(start);
    const QPointF snappedEnd = snapToTextBounds(end);
    const QPdfSelection snappedSelection = m_doc.getSelection(page, snappedStart, snappedEnd);
    result = selectionToResult(snappedSelection);
    if (result.value(QStringLiteral("valid")).toBool())
        return result;

    // If the user's drag rectangle encloses the document text, selecting all
    // text is the least surprising fallback and also handles single-line PDFs
    // where both snapped endpoints can resolve to the same glyph.
    QRectF textBounds;
    bool haveTextBounds = false;
    for (const QPolygonF &polygon : textPolygons) {
        const QRectF rect = polygon.boundingRect().normalized();
        if (rect.width() <= 0 || rect.height() <= 0)
            continue;
        textBounds = haveTextBounds ? textBounds.united(rect) : rect;
        haveTextBounds = true;
    }
    const QRectF dragRect(start, end);
    const QRectF normalizedDrag = dragRect.normalized().adjusted(-1.0, -1.0, 1.0, 1.0);
    if (haveTextBounds && normalizedDrag.contains(textBounds))
        return selectionToResult(allText);

    return empty;
}
