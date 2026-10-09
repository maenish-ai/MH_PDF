#include "PageModel.h"
#include "MemoryPolicy.h"

#include <QPainter>
#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QMutexLocker>
#include <QTransform>
#include <QTextOption>
#include <QUuid>
#include <utility>

PageModel::PageModel(QObject *parent)
    : QAbstractListModel(parent), m_cacheBudget(MemoryPolicy::renderCacheBudgetBytes()) {}

QVariant PageModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_pages.size())
        return {};
    const auto &page = m_pages[index.row()];
    switch (role) {
    case ImageRole: return imageUrlForRow(index.row());
    case LabelRole: return page.label;
    case WidthRole: return page.points.width();
    case HeightRole: return page.points.height();
    default: return {};
    }
}

QHash<int, QByteArray> PageModel::roleNames() const {
    return {{ImageRole, "pageImage"}, {LabelRole, "label"}, {WidthRole, "pageWidth"}, {HeightRole, "pageHeight"}};
}

void PageModel::setRenderer(RenderFunction renderer) {
    m_renderer = std::move(renderer);
    clearCache();
}

void PageModel::setProviderNamespace(const QString &value) {
    const QString normalized = value.isEmpty() ? QStringLiteral("0") : value;
    if (normalized == m_providerNamespace)
        return;
    m_providerNamespace = normalized;
    ++m_modelRevision;
    emit modelRevisionChanged();
    clearCache();
}

void PageModel::refreshMemoryPolicy() {
    QMutexLocker locker(&m_cacheMutex);
    m_cacheBudget = MemoryPolicy::renderCacheBudgetBytes();
    m_cache.clear();
    m_lru.clear();
    m_cacheBytes = 0;
}

void PageModel::ensureIdentity(PageItem &page) {
    if (page.uid.isEmpty())
        page.uid = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void PageModel::clear() {
    beginResetModel();
    m_pages.clear();
    endResetModel();
    ++m_modelRevision; emit modelRevisionChanged();
    clearCache();
}

void PageModel::append(PageItem page) {
    ensureIdentity(page);
    const int row = m_pages.size();
    beginInsertRows({}, row, row);
    m_pages.push_back(std::move(page));
    endInsertRows();
    ++m_modelRevision; emit modelRevisionChanged();
}

void PageModel::insert(int at, PageItem page) {
    at = qBound(0, at, m_pages.size());
    ensureIdentity(page);
    beginInsertRows({}, at, at);
    m_pages.insert(at, std::move(page));
    endInsertRows();
    ++m_modelRevision; emit modelRevisionChanged();
}

void PageModel::remove(int at) {
    if (at < 0 || at >= m_pages.size())
        return;
    beginRemoveRows({}, at, at);
    m_pages.removeAt(at);
    endRemoveRows();
    ++m_modelRevision; emit modelRevisionChanged();
    clearCache();
}

void PageModel::movePage(int from, int to) {
    if (from < 0 || to < 0 || from >= m_pages.size() || to >= m_pages.size() || from == to)
        return;
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_pages.move(from, to);
    endMoveRows();
    ++m_modelRevision; emit modelRevisionChanged();
    clearCache();
}

PageItem *PageModel::page(int index) {
    return index >= 0 && index < m_pages.size() ? &m_pages[index] : nullptr;
}

const PageItem *PageModel::page(int index) const {
    return index >= 0 && index < m_pages.size() ? &m_pages[index] : nullptr;
}

void PageModel::changed(int index) {
    if (index < 0 || index >= m_pages.size())
        return;
    ++m_pages[index].revision;
    ++m_modelRevision; emit modelRevisionChanged();
    emit dataChanged(this->index(index), this->index(index), {ImageRole, WidthRole, HeightRole});
}

QString PageModel::imageUrlForRow(int row) const {
    if (row < 0 || row >= m_pages.size())
        return {};
    const auto &page = m_pages[row];
    return QStringLiteral("image://maenpdf/%1/page/%2/%3/%4").arg(m_providerNamespace).arg(row).arg(page.revision).arg(page.uid);
}

QSize PageModel::defaultPixelSize(const PageItem &page) const {
    QSize size = (page.points * 1.35).toSize();
    const int maxDim = MemoryPolicy::maxRenderDimension();
    size.setWidth(qBound(240, size.width(), maxDim));
    size.setHeight(qBound(320, size.height(), maxDim));
    return size;
}

QString PageModel::cacheKey(int row, const QSize &size) const {
    if (row < 0 || row >= m_pages.size())
        return {};
    const auto &page = m_pages[row];
    return QStringLiteral("%1:%2:%3x%4").arg(page.uid).arg(page.revision).arg(size.width()).arg(size.height());
}

void PageModel::touchCacheKey(const QString &key) const {
    m_lru.removeAll(key);
    m_lru.push_back(key);
}

void PageModel::putCache(const QString &key, const QImage &image) const {
    if (key.isEmpty() || image.isNull())
        return;
    QMutexLocker locker(&m_cacheMutex);
    const qint64 bytes = image.sizeInBytes();
    if (bytes <= 0 || bytes > m_cacheBudget / 2)
        return;

    if (const auto old = m_cache.find(key); old != m_cache.end()) {
        m_cacheBytes -= old->bytes;
        m_cache.erase(old);
        m_lru.removeAll(key);
    }

    while (!m_lru.isEmpty() && m_cacheBytes + bytes > m_cacheBudget) {
        const QString victim = m_lru.takeFirst();
        auto it = m_cache.find(victim);
        if (it != m_cache.end()) {
            m_cacheBytes -= it->bytes;
            m_cache.erase(it);
        }
    }

    m_cache.insert(key, {image, bytes});
    m_cacheBytes += bytes;
    touchCacheKey(key);
}

double PageModel::maxPageWidth() const {
    double result = 595.0;
    for (const auto &page : m_pages)
        result = qMax(result, page.points.width());
    return result;
}

double PageModel::maxPageHeight() const {
    double result = 842.0;
    for (const auto &page : m_pages)
        result = qMax(result, page.points.height());
    return result;
}

QImage PageModel::renderPage(int row, const QSize &requestedSize) const {
    if (row < 0 || row >= m_pages.size())
        return {};

    // Render from a short-lived snapshot so asynchronous QML image requests do
    // not keep references into the mutable page vector while the UI is editing.
    const PageItem page = m_pages.at(row);
    QSize size = requestedSize;
    if (!size.isValid() || size.width() < 2 || size.height() < 2)
        size = defaultPixelSize(page);
    const int maxDim = MemoryPolicy::maxRenderDimension();
    size.setWidth(qBound(64, size.width(), maxDim));
    size.setHeight(qBound(64, size.height(), maxDim));
    // Bucket nearby zoom requests so wheel/pinch zoom does not create a new
    // full-size cached bitmap for every one-pixel size change.
    const int bucket = MemoryPolicy::performanceProfile() == QStringLiteral("eco") ? 128
                     : MemoryPolicy::performanceProfile() == QStringLiteral("balanced") ? 96 : 64;
    size.setWidth(qMin(maxDim, qMax(64, ((size.width() + bucket / 2) / bucket) * bucket)));
    size.setHeight(qMin(maxDim, qMax(64, ((size.height() + bucket / 2) / bucket) * bucket)));

    const QString key = QStringLiteral("%1:%2:%3x%4").arg(page.uid).arg(page.revision).arg(size.width()).arg(size.height());
    {
        QMutexLocker locker(&m_cacheMutex);
        if (const auto it = m_cache.find(key); it != m_cache.end()) {
            const QImage cached = it->image;
            touchCacheKey(key);
            return cached;
        }
    }

    QImage base;
    if (m_renderer)
        base = m_renderer(page, size);
    if (base.isNull()) {
        base = QImage(size, QImage::Format_ARGB32_Premultiplied);
        base.fill(Qt::white);
    } else if (base.size() != size) {
        base = base.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    if (base.format() != QImage::Format_ARGB32_Premultiplied)
        base = base.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&base);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (!page.overlay.isNull())
        painter.drawImage(base.rect(), page.overlay);

    // Keep inserted text as lightweight structured overlays instead of baking
    // it immediately into a page-sized bitmap. This makes inserted text
    // selectable/removable and avoids a full overlay copy for every text edit.
    for (const TextOverlayItem &item : page.textItems) {
        if (item.text.isEmpty())
            continue;
        QFont font;
        const qreal scale = base.height() / qMax<qreal>(1.0, page.points.height());
        font.setPixelSize(qMax(8, qRound(item.fontSize * scale)));
        painter.setFont(font);
        QColor color(item.color);
        if (!color.isValid())
            color = QColor(QStringLiteral("#111827"));
        painter.setPen(color);
        // QString/QTextOption keep the inserted text fully Unicode. Using a
        // directional text rectangle (instead of a raw baseline draw) lets Qt
        // shape Arabic joining forms, bidi runs, Latin text and mixed Arabic /
        // English content consistently while preserving copy/edit code points.
        const bool rtlText = item.text.isRightToLeft();
        const qreal left = item.x * base.width();
        const qreal top = qMax<qreal>(0.0, (item.y - item.height) * base.height());
        const qreal width = qMax<qreal>(2.0, item.width * base.width());
        const qreal height = qMax<qreal>(2.0, item.height * base.height());
        const QRectF textRect(left, top, width, height);
        QTextOption option;
        option.setWrapMode(QTextOption::NoWrap);
        option.setTextDirection(rtlText ? Qt::RightToLeft : Qt::LeftToRight);
        option.setAlignment((rtlText ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
        painter.drawText(textRect, item.text, option);
        if (item.strikeStart >= 0 && item.strikeLength > 0 && item.strikeStart < item.text.size()) {
            const int start = qBound(0, item.strikeStart, int(item.text.size()));
            const int length = qBound(0, item.strikeLength, int(item.text.size()) - start);
            const QFontMetricsF metrics(font);
            const qreal prefix = metrics.horizontalAdvance(item.text.left(start));
            const qreal strikeWidth = metrics.horizontalAdvance(item.text.mid(start, length));
            const qreal strikeY = textRect.top() + textRect.height() * 0.52;
            const qreal strikeX = rtlText ? textRect.right() - prefix - strikeWidth : textRect.left() + prefix;
            QPen strikePen(color, qMax<qreal>(1.0, scale * 1.2));
            painter.setPen(strikePen);
            painter.drawLine(QPointF(strikeX, strikeY),
                             QPointF(strikeX + strikeWidth, strikeY));
        }
    }

    // Lightweight local form widgets. They remain structured in memory and are
    // flattened only by the normal export path, so editing does not require a
    // page-sized bitmap mutation.
    for (const FormOverlayItem &form : page.formItems) {
        QRectF box(form.x * base.width(), form.y * base.height(),
                   form.width * base.width(), form.height * base.height());
        if (box.width() < 4 || box.height() < 4) continue;
        painter.save();
        painter.setPen(QPen(QColor(QStringLiteral("#64748b")), qMax<qreal>(1.0, base.width() / 1000.0)));
        painter.setBrush(QColor(255, 255, 255, 245));
        painter.drawRoundedRect(box, qMin<qreal>(5.0, box.height() * 0.18), qMin<qreal>(5.0, box.height() * 0.18));
        QFont formFont;
        formFont.setPixelSize(qMax(9, qRound(box.height() * 0.52)));
        painter.setFont(formFont);
        painter.setPen(QColor(QStringLiteral("#172033")));
        if (form.type == QStringLiteral("checkbox")) {
            if (form.checked) {
                QPen tickPen(QColor(QStringLiteral("#2563eb")), qMax<qreal>(2.0, box.width() * 0.08), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                painter.setPen(tickPen);
                painter.drawLine(QPointF(box.left() + box.width()*0.20, box.top() + box.height()*0.52),
                                 QPointF(box.left() + box.width()*0.43, box.top() + box.height()*0.75));
                painter.drawLine(QPointF(box.left() + box.width()*0.43, box.top() + box.height()*0.75),
                                 QPointF(box.left() + box.width()*0.82, box.top() + box.height()*0.24));
            }
        } else if (form.type == QStringLiteral("radio")) {
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(box.adjusted(box.width()*0.14, box.height()*0.14, -box.width()*0.14, -box.height()*0.14));
            if (form.checked) {
                painter.setBrush(QColor(QStringLiteral("#2563eb")));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(box.adjusted(box.width()*0.31, box.height()*0.31, -box.width()*0.31, -box.height()*0.31));
            }
        } else {
            QString shown = form.value;
            if (shown.isEmpty()) shown = form.type == QStringLiteral("dropdown") ? QStringLiteral("Select") : QStringLiteral("Text field");
            QTextOption formOption;
            const bool rtlValue = shown.isRightToLeft();
            formOption.setTextDirection(rtlValue ? Qt::RightToLeft : Qt::LeftToRight);
            formOption.setAlignment((rtlValue ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
            painter.drawText(box.adjusted(6, 1, form.type == QStringLiteral("dropdown") ? -18 : -6, -1), shown, formOption);
            if (form.type == QStringLiteral("dropdown"))
                painter.drawText(box.adjusted(4, 1, -6, -1), Qt::AlignVCenter | Qt::AlignRight, QStringLiteral("▾"));
        }
        painter.restore();
    }

    if (!page.watermarkText.isEmpty() && page.watermarkOpacity > 0) {
        painter.save();
        painter.translate(base.width() / 2.0, base.height() / 2.0);
        painter.rotate(-35);
        QFont font;
        font.setBold(true);
        const qreal scale = base.height() / qMax<qreal>(1.0, page.points.height());
        font.setPixelSize(qMax(10, qRound(page.watermarkFontSize * scale)));
        painter.setFont(font);
        painter.setPen(QColor(80, 90, 110, qBound(0, page.watermarkOpacity, 100) * 255 / 100));
        painter.drawText(QRectF(-base.width() / 2.0, -base.height() * 0.08, base.width(), base.height() * 0.16),
                         Qt::AlignCenter, page.watermarkText);
        painter.restore();
    }

    if (page.pageNumber) {
        QFont font;
        font.setPixelSize(qMax(12, qRound(base.height() * 0.02)));
        painter.setFont(font);
        painter.setPen(QColor(50, 50, 50));
        painter.drawText(QRect(0, qRound(base.height() * 0.93), base.width(), qRound(base.height() * 0.05)),
                         Qt::AlignCenter, QString::number(row + 1));
    }

    if (!page.batesText.isEmpty()) {
        QFont font;
        font.setPixelSize(qMax(10, qRound(base.height() * 0.016)));
        painter.setFont(font);
        painter.setPen(QColor(55, 55, 55));
        const int margin = qMax(10, qRound(base.width() * 0.025));
        painter.drawText(QRect(margin, qRound(base.height() * 0.92), base.width() - margin * 2, qRound(base.height() * 0.05)),
                         Qt::AlignRight | Qt::AlignVCenter, page.batesText);
    }
    painter.end();

    putCache(key, base);
    return base;
}

QVariantMap PageModel::cacheStats() const {
    QMutexLocker locker(&m_cacheMutex);
    return {{QStringLiteral("entries"), m_cache.size()},
            {QStringLiteral("bytes"), m_cacheBytes},
            {QStringLiteral("budgetBytes"), m_cacheBudget}};
}

void PageModel::clearCache() const {
    QMutexLocker locker(&m_cacheMutex);
    m_cache.clear();
    m_lru.clear();
    m_cacheBytes = 0;
}
