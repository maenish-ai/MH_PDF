#include "PdfDocument.h"
#include "AppLogger.h"
#include "EngineCapabilities.h"
#include "IPdfBackend.h"
#include "MemoryPolicy.h"
#include "QtPdfBackend.h"

#include <QCoreApplication>
#include <QClipboard>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QBuffer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QGuiApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QImageReader>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTransform>
#include <QUndoCommand>
#include <QUrl>
#include <QUuid>
#include <QtMath>
#include <functional>
#include <utility>

class LambdaCommand final : public QUndoCommand {
public:
    LambdaCommand(QString text, std::function<void()> undoFn, std::function<void()> redoFn, bool skipFirst = false)
        : m_undo(std::move(undoFn)), m_redo(std::move(redoFn)), m_skipFirst(skipFirst) { setText(std::move(text)); }
    void undo() override { m_undo(); }
    void redo() override { if (m_skipFirst) { m_skipFirst = false; return; } m_redo(); }
private:
    std::function<void()> m_undo;
    std::function<void()> m_redo;
    bool m_skipFirst{false};
};


namespace {
QPointF displayToSourceNormalized(const QPointF &displayPoint, int rotation) {
    const qreal u = qBound<qreal>(0.0, displayPoint.x(), 1.0);
    const qreal v = qBound<qreal>(0.0, displayPoint.y(), 1.0);
    switch (((rotation % 360) + 360) % 360) {
    case 90:  return QPointF(v, 1.0 - u);
    case 180: return QPointF(1.0 - u, 1.0 - v);
    case 270: return QPointF(1.0 - v, u);
    default:  return QPointF(u, v);
    }
}

QPointF sourceToDisplayNormalized(const QPointF &sourcePoint, int rotation) {
    const qreal u = qBound<qreal>(0.0, sourcePoint.x(), 1.0);
    const qreal v = qBound<qreal>(0.0, sourcePoint.y(), 1.0);
    switch (((rotation % 360) + 360) % 360) {
    case 90:  return QPointF(1.0 - v, u);
    case 180: return QPointF(1.0 - u, 1.0 - v);
    case 270: return QPointF(v, 1.0 - u);
    default:  return QPointF(u, v);
    }
}

QVariantMap normalizedDisplayRect(const QVariantMap &sourceRect, int rotation) {
    const qreal x = sourceRect.value(QStringLiteral("x")).toDouble();
    const qreal y = sourceRect.value(QStringLiteral("y")).toDouble();
    const qreal w = sourceRect.value(QStringLiteral("w")).toDouble();
    const qreal h = sourceRect.value(QStringLiteral("h")).toDouble();
    const QPointF a = sourceToDisplayNormalized(QPointF(x, y), rotation);
    const QPointF b = sourceToDisplayNormalized(QPointF(x + w, y + h), rotation);
    const qreal left = qMin(a.x(), b.x());
    const qreal top = qMin(a.y(), b.y());
    const qreal right = qMax(a.x(), b.x());
    const qreal bottom = qMax(a.y(), b.y());
    return {{QStringLiteral("x"), qBound<qreal>(0.0, left, 1.0)},
            {QStringLiteral("y"), qBound<qreal>(0.0, top, 1.0)},
            {QStringLiteral("w"), qBound<qreal>(0.0, right - left, 1.0 - left)},
            {QStringLiteral("h"), qBound<qreal>(0.0, bottom - top, 1.0 - top)}};
}

QString encodeRecoveryImage(const QImage &image) {
    if (image.isNull())
        return {};
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
        return {};
    return QString::fromLatin1(bytes.toBase64());
}

QImage decodeRecoveryImage(const QString &value) {
    if (value.isEmpty())
        return {};
    QImage image;
    image.loadFromData(QByteArray::fromBase64(value.toLatin1()), "PNG");
    return image;
}

QString normalizeEditableUnicode(const QString &text) {
    // Compatibility normalization converts Arabic Presentation Forms and other
    // compatibility glyph code points to editable Unicode text while retaining
    // Arabic/English letters, combining marks and bidi content in QString.
    return text.normalized(QString::NormalizationForm_KC);
}

QSizeF measuredEditableText(const QString &text, const QFontMetricsF &metrics) {
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    qreal width = 0.0;
    for (const QString &line : lines)
        width = qMax(width, metrics.horizontalAdvance(line.isEmpty() ? QStringLiteral(" ") : line));
    return QSizeF(width, metrics.height() * qMax(1, lines.size()));
}
}

PdfDocument::PdfDocument(QObject *parent) : QObject(parent) {
    m_pages.setRenderer([this](const PageItem &page, const QSize &size) { return renderBase(page, size); });
    connect(&m_undo, &QUndoStack::canUndoChanged, this, &PdfDocument::historyChanged);
    connect(&m_undo, &QUndoStack::canRedoChanged, this, &PdfDocument::historyChanged);
    // Recovery is idle-debounced instead of exporting the full document every
    // minute while the user is actively drawing/scrolling. This is critical on
    // older machines and large PDFs. Every new edit restarts the timer.
    m_autosaveTimer.setInterval(120000);
    m_autosaveTimer.setSingleShot(true);
    connect(&m_autosaveTimer, &QTimer::timeout, this, &PdfDocument::autosave);
    m_undo.setUndoLimit(MemoryPolicy::undoLimit());
    newDocument();
    m_modified = false;
    refreshRecoveryState();
    AppLogger::write(QStringLiteral("INFO"), QStringLiteral("Document engine 7 initialized"));
}

PdfDocument::~PdfDocument() { cleanupTemporaryInputs(); }

QString PdfDocument::title() const {
    if (m_filePath.isEmpty())
        return QStringLiteral("Untitled.pdf");
    const QUrl url(m_filePath);
    if (url.isValid() && !url.scheme().isEmpty() && !url.isLocalFile()) {
        const QString name = url.fileName();
        if (!name.isEmpty()) return name;
    }
    return QFileInfo(m_filePath).fileName();
}

QString PdfDocument::normalizedPath(const QString &path) const {
    const QUrl url(path);
    return url.isLocalFile() ? url.toLocalFile() : path;
}

bool PdfDocument::isContentUri(const QString &path) const {
    return QUrl(path).scheme().compare(QStringLiteral("content"), Qt::CaseInsensitive) == 0;
}

void PdfDocument::cleanupTemporaryInputs() {
    for (const QString &path : std::as_const(m_tempInputs))
        QFile::remove(path);
    m_tempInputs.clear();
}

QString PdfDocument::prepareReadablePath(const QString &path) {
    const QString normalized = normalizedPath(path);
    if (!isContentUri(normalized))
        return normalized;

    QFile source(normalized);
    if (!source.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QStringLiteral("error.content_read"), {});
        return {};
    }

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/imports");
    QDir().mkpath(dir);
    const QString local = QDir(dir).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".pdf"));
    QFile destination(local);
    if (!destination.open(QIODevice::WriteOnly)) {
        QFile::remove(local);
        emit errorOccurred(QStringLiteral("error.content_read"), {});
        return {};
    }
    QByteArray buffer(256 * 1024, '\0');
    while (!source.atEnd()) {
        const qint64 read = source.read(buffer.data(), buffer.size());
        if (read < 0 || destination.write(buffer.constData(), read) != read) {
            destination.close();
            QFile::remove(local);
            emit errorOccurred(QStringLiteral("error.content_read"), {});
            return {};
        }
    }
    destination.close();
    m_tempInputs.push_back(local);
    return local;
}

QString PdfDocument::prepareReadableImagePath(const QString &path) {
    const QString normalized = normalizedPath(path);
    if (!isContentUri(normalized))
        return normalized;

    QFile source(normalized);
    if (!source.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QStringLiteral("error.image_read"), {});
        return {};
    }
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/imports");
    QDir().mkpath(dir);
    QString suffix = QFileInfo(QUrl(normalized).path()).suffix().toLower();
    if (suffix.isEmpty())
        suffix = QStringLiteral("png");
    const QString local = QDir(dir).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".") + suffix);
    QFile destination(local);
    if (!destination.open(QIODevice::WriteOnly)) {
        QFile::remove(local);
        emit errorOccurred(QStringLiteral("error.image_read"), {});
        return {};
    }
    QByteArray buffer(256 * 1024, '\0');
    while (!source.atEnd()) {
        const qint64 read = source.read(buffer.data(), buffer.size());
        if (read < 0 || destination.write(buffer.constData(), read) != read) {
            destination.close();
            QFile::remove(local);
            emit errorOccurred(QStringLiteral("error.image_read"), {});
            return {};
        }
    }
    destination.close();
    m_tempInputs.push_back(local);
    return local;
}

PageItem PdfDocument::blank() const {
    PageItem page;
    page.points = QSizeF(595, 842);
    page.label = QStringLiteral("Page");
    return page;
}

void PdfDocument::markModified(bool value) {
    const bool changed = m_modified != value;
    m_modified = value;
    if (m_modified)
        m_autosaveTimer.start(); // every edit restarts the idle-recovery debounce
    else
        m_autosaveTimer.stop();
    if (changed)
        emit modifiedChanged();
}

bool PdfDocument::newDocument() {
    cleanupTemporaryInputs();
    m_backends.clear();
    m_pages.clear();
    m_pages.append(blank());
    m_filePath.clear();
    m_currentPage = 0;
    m_undo.clear();
    m_hasClipboard = false;
    emit clipboardChanged();
    markModified(false);
    emit filePathChanged();
    emit pageCountChanged();
    emit currentPageChanged();
    return true;
}

bool PdfDocument::loadIntoModel(const QString &path, bool append, const QString &password) {
    const QString localPath = prepareReadablePath(path);
    if (localPath.isEmpty())
        return false;

    auto backend = std::make_shared<QtPdfBackend>();
    QString error;
    if (!backend->open(localPath, password, &error)) {
        if (error == QStringLiteral("password"))
            emit passwordRequired(path);
        else if (error == QStringLiteral("unsupported-security"))
            emit errorOccurred(QStringLiteral("error.unsupported_security"), {});
        else
            emit errorOccurred(QStringLiteral("error.open_pdf"), {});
        return false;
    }

    if (!append) {
        m_pages.clear();
        m_backends.clear();
    }
    m_backends.insert(localPath, backend);

    for (int pageIndex = 0; pageIndex < backend->pageCount(); ++pageIndex) {
        PageItem page;
        page.points = backend->pagePointSize(pageIndex);
        page.sourceId = localPath;
        page.sourcePage = pageIndex;
        page.label = QString::number(m_pages.count() + 1);
        m_pages.append(std::move(page));
    }
    return true;
}

bool PdfDocument::openDocument(const QString &path) {
    if (!loadIntoModel(path, false))
        return false;
    if (!m_pages.count())
        m_pages.append(blank());
    m_filePath = normalizedPath(path);
    m_currentPage = 0;
    m_undo.clear();
    m_hasClipboard = false;
    emit clipboardChanged();
    markModified(false);
    emit filePathChanged();
    emit pageCountChanged();
    emit currentPageChanged();
    return true;
}

bool PdfDocument::openDocumentWithPassword(const QString &path, const QString &password) {
    if (password.isEmpty()) {
        emit passwordRequired(path);
        return false;
    }
    if (!loadIntoModel(path, false, password))
        return false;
    if (!m_pages.count())
        m_pages.append(blank());
    m_filePath = normalizedPath(path);
    m_currentPage = 0;
    m_undo.clear();
    m_hasClipboard = false;
    emit clipboardChanged();
    markModified(false);
    emit filePathChanged();
    emit pageCountChanged();
    emit currentPageChanged();
    return true;
}

bool PdfDocument::appendPdf(const QString &path) {
    if (m_locked)
        return false;
    const int before = m_pages.count();
    if (!loadIntoModel(path, true))
        return false;

    QVector<PageItem> added;
    for (int i = before; i < m_pages.count(); ++i)
        added.push_back(*m_pages.page(i));
    const int count = added.size();
    if (!count)
        return true;

    m_undo.push(new LambdaCommand(QStringLiteral("Combine PDF"),
        [this, before, count] {
            for (int n = 0; n < count; ++n) m_pages.remove(before);
            emit pageCountChanged();
            setCurrentPage(qMax(0, before - 1));
            markModified();
        },
        [this, before, added] {
            for (int n = 0; n < added.size(); ++n) m_pages.insert(before + n, added[n]);
            emit pageCountChanged();
            setCurrentPage(before);
            markModified();
        }, true));
    markModified();
    emit pageCountChanged();
    setCurrentPage(before);
    emit info(QStringLiteral("info.pages_added"), QVariantList{count});
    return true;
}

QImage PdfDocument::renderBase(const PageItem &page, const QSize &requestedSize) const {
    QSize finalSize = requestedSize;
    if (!finalSize.isValid())
        finalSize = (page.points * 1.35).toSize();
    finalSize.setWidth(qBound(64, finalSize.width(), MemoryPolicy::maxRenderDimension()));
    finalSize.setHeight(qBound(64, finalSize.height(), MemoryPolicy::maxRenderDimension()));

    const int rotation = ((page.rotation % 360) + 360) % 360;
    QSize sourceSize = finalSize;
    if (rotation == 90 || rotation == 270)
        sourceSize.transpose();

    QImage image;
    if (!page.base.isNull()) {
        image = page.base.scaled(sourceSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else if (!page.sourceId.isEmpty() && page.sourcePage >= 0) {
        const auto backend = m_backends.value(page.sourceId);
        if (backend)
            image = backend->render(page.sourcePage, sourceSize);
    }

    if (image.isNull()) {
        image = QImage(sourceSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
    }

    if (rotation != 0) {
        QTransform transform;
        transform.rotate(rotation);
        image = image.transformed(transform, Qt::SmoothTransformation);
    }
    if (image.size() != finalSize)
        image = image.scaled(finalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return image;
}

void PdfDocument::ensureOverlay(PageItem *page) {
    if (!page || !page->overlay.isNull())
        return;
    QSize size = (page->points * 1.5).toSize();
    const int maxDim = MemoryPolicy::overlayMaxDimension();
    size.setWidth(qBound(320, size.width(), maxDim));
    size.setHeight(qBound(420, size.height(), maxDim));
    page->overlay = QImage(size, QImage::Format_ARGB32_Premultiplied);
    page->overlay.fill(Qt::transparent);
}

bool PdfDocument::exportPdf(const QString &localPath, int onlyPage) {
    if (localPath.isEmpty() || m_pages.count() == 0)
        return false;
    const int firstIndex = onlyPage >= 0 ? onlyPage : 0;
    const int lastIndex = onlyPage >= 0 ? onlyPage : m_pages.count() - 1;
    if (!m_pages.page(firstIndex))
        return false;

    QPdfWriter writer(localPath);
    writer.setResolution(144);
    writer.setTitle(title());
    writer.setCreator(QStringLiteral("MaenPDF"));
    writer.setPageSize(QPageSize(m_pages.page(firstIndex)->points, QPageSize::Point, QStringLiteral("PDF page")));
    QPainter painter(&writer);
    if (!painter.isActive()) {
        emit errorOccurred(QStringLiteral("error.cannot_write"), {});
        return false;
    }

    bool firstOutput = true;
    for (int i = firstIndex; i <= lastIndex; ++i) {
        const PageItem *page = m_pages.page(i);
        if (!page) continue;
        if (!firstOutput) {
            writer.setPageSize(QPageSize(page->points, QPageSize::Point, QStringLiteral("PDF page")));
            writer.newPage();
        }
        firstOutput = false;
        QSize pixels = (page->points * 2.0).toSize();
        const int maxDim = MemoryPolicy::maxRenderDimension();
        pixels.setWidth(qBound(600, pixels.width(), maxDim));
        pixels.setHeight(qBound(800, pixels.height(), maxDim));
        const QImage image = m_pages.renderPage(i, pixels);
        painter.drawImage(QRect(0, 0, writer.width(), writer.height()), image);
        // Keep the native window responsive during large multi-page saves.
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 8);
    }
    painter.end();
    return true;
}

bool PdfDocument::atomicExportPdf(const QString &target) {
    QFileInfo info(target);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        emit errorOccurred(QStringLiteral("error.destination_not_writable"), {});
        return false;
    }

    const QString temp = dir.filePath(QStringLiteral(".") + info.fileName() + QStringLiteral(".")
                                      + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".tmp.pdf"));
    if (!exportPdf(temp)) {
        QFile::remove(temp);
        return false;
    }

    QPdfDocument check;
    if (check.load(temp) != QPdfDocument::Error::None || check.pageCount() != m_pages.count()) {
        QFile::remove(temp);
        emit errorOccurred(QStringLiteral("error.save_verification"), {});
        return false;
    }

    const bool existed = QFile::exists(target);
    const QString backup = target + QStringLiteral(".maenpdf-backup");
    if (existed) {
        QFile::remove(backup);
        if (!QFile::rename(target, backup)) {
            QFile::remove(temp);
            emit errorOccurred(QStringLiteral("error.safe_replace"), {});
            return false;
        }
    }

    if (!QFile::rename(temp, target)) {
        if (existed) QFile::rename(backup, target);
        QFile::remove(temp);
        emit errorOccurred(QStringLiteral("error.save_restored"), {});
        return false;
    }
    if (existed) QFile::remove(backup);
    return true;
}

bool PdfDocument::copyLocalFileToDestination(const QString &localPath, const QString &destination) const {
    QFile source(localPath);
    if (!source.open(QIODevice::ReadOnly))
        return false;
    QFile output(destination);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QByteArray buffer(256 * 1024, '\0');
    while (!source.atEnd()) {
        const qint64 read = source.read(buffer.data(), buffer.size());
        if (read < 0 || output.write(buffer.constData(), read) != read)
            return false;
    }
    return output.flush();
}

bool PdfDocument::exportToDestination(const QString &destination, int onlyPage, bool atomicFullDocument) {
    const QString normalized = normalizedPath(destination);
    if (!isContentUri(normalized)) {
        return atomicFullDocument && onlyPage < 0 ? atomicExportPdf(normalized) : exportPdf(normalized, onlyPage);
    }

    QTemporaryFile temporary(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                 .filePath(QStringLiteral("maenpdf-output-XXXXXX.pdf")));
    temporary.setAutoRemove(false);
    if (!temporary.open()) {
        emit errorOccurred(QStringLiteral("error.content_write"), {});
        return false;
    }
    const QString local = temporary.fileName();
    temporary.close();
    QFile::remove(local);

    const bool rendered = exportPdf(local, onlyPage);
    if (!rendered) {
        QFile::remove(local);
        return false;
    }
    const bool copied = copyLocalFileToDestination(local, normalized);
    QFile::remove(local);
    if (!copied)
        emit errorOccurred(QStringLiteral("error.content_write"), {});
    return copied;
}

bool PdfDocument::save() {
    if (m_filePath.isEmpty()) {
        emit errorOccurred(QStringLiteral("error.choose_save_as"), {});
        return false;
    }
    if (!exportToDestination(m_filePath, -1, true))
        return false;
    markModified(false);
    discardRecovery();
    emit info(QStringLiteral("info.pdf_saved"), {});
    return true;
}

bool PdfDocument::saveAs(const QString &path) {
    QString destination = normalizedPath(path);
    if (!isContentUri(destination) && !destination.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        destination += QStringLiteral(".pdf");
    if (!exportToDestination(destination, -1, true))
        return false;
    m_filePath = destination;
    markModified(false);
    discardRecovery();
    emit filePathChanged();
    emit info(QStringLiteral("info.pdf_saved"), {});
    return true;
}

bool PdfDocument::validateCurrentDocument() const {
    return m_pages.count() > 0 && m_currentPage >= 0 && m_currentPage < m_pages.count();
}

QVariantList PdfDocument::searchText(const QString &query, int maxResults) const {
    QVariantList results;
    const QString needle = query.trimmed();
    if (needle.isEmpty() || maxResults <= 0)
        return results;

    for (int row = 0; row < m_pages.count() && results.size() < maxResults; ++row) {
        const PageItem *page = m_pages.page(row);
        if (!page || page->sourceId.isEmpty() || page->sourcePage < 0)
            continue;
        const auto backend = m_backends.value(page->sourceId);
        if (!backend)
            continue;
        const QString text = backend->pageText(page->sourcePage);
        int from = 0;
        while (results.size() < maxResults) {
            const int at = text.indexOf(needle, from, Qt::CaseInsensitive);
            if (at < 0) break;
            const int start = qMax(0, at - 45);
            const int end = qMin(text.size(), at + needle.size() + 70);
            QVariantMap hit;
            hit[QStringLiteral("page")] = row;
            hit[QStringLiteral("index")] = at;
            hit[QStringLiteral("excerpt")] = text.mid(start, end - start).simplified();
            results.push_back(hit);
            from = at + qMax(1, needle.size());
        }
    }
    return results;
}


QVariantMap PdfDocument::textSelection(int pageIndex, double x1, double y1, double x2, double y2) const {
    QVariantMap empty{{QStringLiteral("valid"), false},
                      {QStringLiteral("text"), QString()},
                      {QStringLiteral("rects"), QVariantList{}}};
    const PageItem *page = m_pages.page(pageIndex);
    if (!page || page->sourceId.isEmpty() || page->sourcePage < 0 || !page->base.isNull())
        return empty;
    const auto backend = m_backends.value(page->sourceId);
    if (!backend)
        return empty;

    const QPointF displayStart(qBound(0.0, x1, 1.0), qBound(0.0, y1, 1.0));
    const QPointF displayEnd(qBound(0.0, x2, 1.0), qBound(0.0, y2, 1.0));
    const QPointF sourceStartNorm = displayToSourceNormalized(displayStart, page->rotation);
    const QPointF sourceEndNorm = displayToSourceNormalized(displayEnd, page->rotation);
    const QSizeF sourcePoints = backend->pagePointSize(page->sourcePage);
    if (sourcePoints.width() <= 0 || sourcePoints.height() <= 0)
        return empty;

    QVariantMap result = backend->textSelection(
        page->sourcePage,
        QPointF(sourceStartNorm.x() * sourcePoints.width(), sourceStartNorm.y() * sourcePoints.height()),
        QPointF(sourceEndNorm.x() * sourcePoints.width(), sourceEndNorm.y() * sourcePoints.height()));

    QVariantList displayRects;
    const QVariantList sourceRects = result.value(QStringLiteral("rects")).toList();
    displayRects.reserve(sourceRects.size());
    for (const QVariant &rectValue : sourceRects)
        displayRects.push_back(normalizedDisplayRect(rectValue.toMap(), page->rotation));
    result[QStringLiteral("rects")] = displayRects;
    result[QStringLiteral("valid")] = result.value(QStringLiteral("valid")).toBool() && !displayRects.isEmpty();
    return result;
}

void PdfDocument::copyTextToClipboard(const QString &text) const {
    if (text.isEmpty())
        return;
    if (QClipboard *clipboard = QGuiApplication::clipboard())
        clipboard->setText(text, QClipboard::Clipboard);
    emit const_cast<PdfDocument *>(this)->info(QStringLiteral("info.text_copied"), {});
}

QVariantMap PdfDocument::engineCapabilities() const { return EngineCapabilities::current(); }

QString PdfDocument::recoveryPath() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/recovery");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/session.json");
}

QString PdfDocument::diagnosticLogPath() const { return AppLogger::logFilePath(); }

void PdfDocument::refreshRecoveryState() {
    const bool available = QFileInfo::exists(recoveryPath()) && QFileInfo(recoveryPath()).size() > 0;
    if (available != m_recoveryAvailable) {
        m_recoveryAvailable = available;
        emit recoveryAvailableChanged();
    }
}

bool PdfDocument::writeRecoveryJournal() {
    if (!m_modified || m_pages.count() == 0)
        return false;

    const QFileInfo journalInfo(recoveryPath());
    QDir recoveryDir = journalInfo.dir();
    if (!recoveryDir.exists() && !recoveryDir.mkpath(QStringLiteral(".")))
        return false;
    const QString sourcesDirPath = recoveryDir.filePath(QStringLiteral("sources"));
    QDir().mkpath(sourcesDirPath);

    QHash<QString, QString> recoverySources;
    QJsonArray pages;
    for (int i = 0; i < m_pages.count(); ++i) {
        const PageItem *page = m_pages.page(i);
        if (!page)
            continue;
        QString source = page->sourceId;
        if (!source.isEmpty() && m_tempInputs.contains(source) && QFileInfo::exists(source)) {
            QString copy = recoverySources.value(source);
            if (copy.isEmpty()) {
                const QString suffix = QFileInfo(source).suffix().isEmpty() ? QStringLiteral("pdf") : QFileInfo(source).suffix();
                copy = QDir(sourcesDirPath).filePath(QStringLiteral("source-%1.%2")
                    .arg(recoverySources.size()).arg(suffix));
                QFile::remove(copy);
                if (!QFile::copy(source, copy))
                    return false;
                recoverySources.insert(source, copy);
            }
            source = copy;
        }

        QJsonObject item;
        item.insert(QStringLiteral("source"), source);
        item.insert(QStringLiteral("sourcePage"), page->sourcePage);
        item.insert(QStringLiteral("width"), page->points.width());
        item.insert(QStringLiteral("height"), page->points.height());
        item.insert(QStringLiteral("label"), page->label);
        item.insert(QStringLiteral("rotation"), page->rotation);
        item.insert(QStringLiteral("watermarkText"), page->watermarkText);
        item.insert(QStringLiteral("watermarkFontSize"), page->watermarkFontSize);
        item.insert(QStringLiteral("watermarkOpacity"), page->watermarkOpacity);
        item.insert(QStringLiteral("pageNumber"), page->pageNumber);
        item.insert(QStringLiteral("batesText"), page->batesText);
        QJsonArray textItems;
        for (const TextOverlayItem &textItem : page->textItems) {
            QJsonObject textObject;
            textObject.insert(QStringLiteral("id"), textItem.id);
            textObject.insert(QStringLiteral("text"), textItem.text);
            textObject.insert(QStringLiteral("x"), textItem.x);
            textObject.insert(QStringLiteral("y"), textItem.y);
            textObject.insert(QStringLiteral("width"), textItem.width);
            textObject.insert(QStringLiteral("height"), textItem.height);
            textObject.insert(QStringLiteral("fontSize"), textItem.fontSize);
            textObject.insert(QStringLiteral("color"), textItem.color);
            textObject.insert(QStringLiteral("strikeStart"), textItem.strikeStart);
            textObject.insert(QStringLiteral("strikeLength"), textItem.strikeLength);
            textItems.append(textObject);
        }
        item.insert(QStringLiteral("textItems"), textItems);
        QJsonArray formItems;
        for (const FormOverlayItem &formItem : page->formItems) {
            QJsonObject formObject;
            formObject.insert(QStringLiteral("id"), formItem.id);
            formObject.insert(QStringLiteral("type"), formItem.type);
            formObject.insert(QStringLiteral("label"), formItem.label);
            formObject.insert(QStringLiteral("value"), formItem.value);
            QJsonArray options;
            for (const QString &option : formItem.options) options.append(option);
            formObject.insert(QStringLiteral("options"), options);
            formObject.insert(QStringLiteral("x"), formItem.x);
            formObject.insert(QStringLiteral("y"), formItem.y);
            formObject.insert(QStringLiteral("width"), formItem.width);
            formObject.insert(QStringLiteral("height"), formItem.height);
            formObject.insert(QStringLiteral("checked"), formItem.checked);
            formObject.insert(QStringLiteral("selectedIndex"), formItem.selectedIndex);
            formItems.append(formObject);
        }
        item.insert(QStringLiteral("formItems"), formItems);
        if (!page->base.isNull())
            item.insert(QStringLiteral("basePng"), encodeRecoveryImage(page->base));
        if (!page->overlay.isNull())
            item.insert(QStringLiteral("overlayPng"), encodeRecoveryImage(page->overlay));
        pages.append(item);
    }

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("MaenPDF-Recovery-1"));
    root.insert(QStringLiteral("originalFile"), m_filePath);
    root.insert(QStringLiteral("currentPage"), m_currentPage);
    root.insert(QStringLiteral("pages"), pages);

    const QString tempPath = recoveryPath() + QStringLiteral(".tmp");
    QFile temp(tempPath);
    if (!temp.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (temp.write(payload) != payload.size() || !temp.flush()) {
        temp.close();
        QFile::remove(tempPath);
        return false;
    }
    temp.close();
    QFile::remove(recoveryPath());
    if (!QFile::rename(tempPath, recoveryPath())) {
        QFile::remove(tempPath);
        return false;
    }
    return true;
}

void PdfDocument::autosave() {
    if (!m_modified || m_pages.count() == 0)
        return;
    if (writeRecoveryJournal()) {
        AppLogger::write(QStringLiteral("INFO"), QStringLiteral("Recovery journal updated"));
        refreshRecoveryState();
    } else {
        AppLogger::write(QStringLiteral("WARN"), QStringLiteral("Recovery journal failed"));
    }
}

bool PdfDocument::recoverAutosave() {
    QFile file(recoveryPath());
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject())
        return false;
    const QJsonObject root = json.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("MaenPDF-Recovery-1"))
        return false;
    const QJsonArray pages = root.value(QStringLiteral("pages")).toArray();
    if (pages.isEmpty())
        return false;

    cleanupTemporaryInputs();
    m_backends.clear();
    m_pages.clear();
    QHash<QString, std::shared_ptr<IPdfBackend>> opened;

    for (const QJsonValue &value : pages) {
        const QJsonObject item = value.toObject();
        PageItem page;
        page.points = QSizeF(item.value(QStringLiteral("width")).toDouble(595.0),
                             item.value(QStringLiteral("height")).toDouble(842.0));
        page.label = item.value(QStringLiteral("label")).toString();
        page.rotation = item.value(QStringLiteral("rotation")).toInt();
        page.watermarkText = item.value(QStringLiteral("watermarkText")).toString();
        page.watermarkFontSize = item.value(QStringLiteral("watermarkFontSize")).toInt(42);
        page.watermarkOpacity = item.value(QStringLiteral("watermarkOpacity")).toInt();
        page.pageNumber = item.value(QStringLiteral("pageNumber")).toBool();
        page.batesText = item.value(QStringLiteral("batesText")).toString();
        const QJsonArray textItems = item.value(QStringLiteral("textItems")).toArray();
        for (const QJsonValue &textValue : textItems) {
            const QJsonObject textObject = textValue.toObject();
            TextOverlayItem textItem;
            textItem.id = textObject.value(QStringLiteral("id")).toString();
            if (textItem.id.isEmpty())
                textItem.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            textItem.text = textObject.value(QStringLiteral("text")).toString();
            textItem.x = textObject.value(QStringLiteral("x")).toDouble(0.15);
            textItem.y = textObject.value(QStringLiteral("y")).toDouble(0.20);
            textItem.width = textObject.value(QStringLiteral("width")).toDouble(0.10);
            textItem.height = textObject.value(QStringLiteral("height")).toDouble(0.03);
            textItem.fontSize = textObject.value(QStringLiteral("fontSize")).toInt(18);
            textItem.color = textObject.value(QStringLiteral("color")).toString(QStringLiteral("#111827"));
            textItem.strikeStart = textObject.value(QStringLiteral("strikeStart")).toInt(-1);
            textItem.strikeLength = textObject.value(QStringLiteral("strikeLength")).toInt(0);
            if (!textItem.text.isEmpty())
                page.textItems.push_back(std::move(textItem));
        }
        const QJsonArray formItems = item.value(QStringLiteral("formItems")).toArray();
        for (const QJsonValue &formValue : formItems) {
            const QJsonObject formObject = formValue.toObject();
            FormOverlayItem formItem;
            formItem.id = formObject.value(QStringLiteral("id")).toString();
            if (formItem.id.isEmpty()) formItem.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            formItem.type = formObject.value(QStringLiteral("type")).toString(QStringLiteral("text"));
            formItem.label = formObject.value(QStringLiteral("label")).toString();
            formItem.value = formObject.value(QStringLiteral("value")).toString();
            for (const QJsonValue &option : formObject.value(QStringLiteral("options")).toArray()) formItem.options.push_back(option.toString());
            formItem.x = formObject.value(QStringLiteral("x")).toDouble(0.15);
            formItem.y = formObject.value(QStringLiteral("y")).toDouble(0.20);
            formItem.width = formObject.value(QStringLiteral("width")).toDouble(0.25);
            formItem.height = formObject.value(QStringLiteral("height")).toDouble(0.045);
            formItem.checked = formObject.value(QStringLiteral("checked")).toBool(false);
            formItem.selectedIndex = formObject.value(QStringLiteral("selectedIndex")).toInt(0);
            page.formItems.push_back(std::move(formItem));
        }
        page.base = decodeRecoveryImage(item.value(QStringLiteral("basePng")).toString());
        page.overlay = decodeRecoveryImage(item.value(QStringLiteral("overlayPng")).toString());

        const QString source = item.value(QStringLiteral("source")).toString();
        const int sourcePage = item.value(QStringLiteral("sourcePage")).toInt(-1);
        if (page.base.isNull() && !source.isEmpty() && sourcePage >= 0) {
            auto backend = opened.value(source);
            if (!backend) {
                backend = std::make_shared<QtPdfBackend>();
                QString error;
                if (!backend->open(source, QString(), &error)) {
                    m_pages.clear();
                    m_backends.clear();
                    emit errorOccurred(QStringLiteral("error.open_pdf"), {});
                    return false;
                }
                opened.insert(source, backend);
                m_backends.insert(source, backend);
            }
            if (sourcePage >= backend->pageCount())
                return false;
            page.sourceId = source;
            page.sourcePage = sourcePage;
        }
        m_pages.append(std::move(page));
    }

    if (!m_pages.count())
        return false;
    m_filePath = root.value(QStringLiteral("originalFile")).toString();
    m_currentPage = qBound(0, root.value(QStringLiteral("currentPage")).toInt(), m_pages.count() - 1);
    m_undo.clear();
    markModified(true);
    emit filePathChanged();
    emit pageCountChanged();
    emit currentPageChanged();
    emit info(QStringLiteral("info.recovered"), {});
    AppLogger::write(QStringLiteral("INFO"), QStringLiteral("Recovery journal restored"));
    return true;
}

void PdfDocument::discardRecovery() {
    const QFileInfo info(recoveryPath());
    QDir dir = info.dir();
    if (dir.exists())
        dir.removeRecursively();
    refreshRecoveryState();
}

bool PdfDocument::extractPage(int page, const QString &path) {
    QString destination = normalizedPath(path);
    if (!isContentUri(destination) && !destination.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        destination += QStringLiteral(".pdf");
    const bool ok = exportToDestination(destination, page, false);
    if (ok) emit info(QStringLiteral("info.page_extracted"), {});
    return ok;
}

bool PdfDocument::protectCopy(const QString &outputPath, const QString &user, const QString &owner,
                              bool allowPrint, bool allowCopy, bool allowModify) {
    if (user.isEmpty()) {
        emit errorOccurred(QStringLiteral("error.password_required"), {});
        return false;
    }
    QString qpdf = QStandardPaths::findExecutable(QStringLiteral("qpdf"));
    if (qpdf.isEmpty()) {
        QString bundled = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("tools/qpdf/qpdf"));
#ifdef Q_OS_WIN
        bundled += QStringLiteral(".exe");
#endif
        if (QFileInfo::exists(bundled))
            qpdf = bundled;
    }
    if (qpdf.isEmpty()) {
        emit errorOccurred(QStringLiteral("error.security_provider_missing"), {});
        return false;
    }

    QTemporaryFile input(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(QStringLiteral("maenpdf-security-input-XXXXXX.pdf")));
    input.setAutoRemove(false);
    if (!input.open()) {
        emit errorOccurred(QStringLiteral("error.secure_temp"), {});
        return false;
    }
    const QString inputPath = input.fileName();
    input.close();
    QFile::remove(inputPath);
    if (!exportPdf(inputPath)) {
        QFile::remove(inputPath);
        return false;
    }

    QTemporaryFile encrypted(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                 .filePath(QStringLiteral("maenpdf-security-output-XXXXXX.pdf")));
    encrypted.setAutoRemove(false);
    if (!encrypted.open()) {
        QFile::remove(inputPath);
        emit errorOccurred(QStringLiteral("error.secure_temp"), {});
        return false;
    }
    const QString encryptedPath = encrypted.fileName();
    encrypted.close();
    QFile::remove(encryptedPath);

    const QStringList arguments = {
        QStringLiteral("--encrypt"), user, owner.isEmpty() ? user : owner, QStringLiteral("256"),
        QStringLiteral("--print=") + (allowPrint ? QStringLiteral("full") : QStringLiteral("none")),
        QStringLiteral("--extract=") + (allowCopy ? QStringLiteral("y") : QStringLiteral("n")),
        QStringLiteral("--modify=") + (allowModify ? QStringLiteral("all") : QStringLiteral("none")),
        QStringLiteral("--"), inputPath, encryptedPath
    };

    QProcess process;
    process.start(qpdf, arguments);
    if (!process.waitForStarted(5000)) {
        QFile::remove(inputPath); QFile::remove(encryptedPath);
        emit errorOccurred(QStringLiteral("error.security_start"), {});
        return false;
    }
    QElapsedTimer securityTimer;
    securityTimer.start();
    while (process.state() != QProcess::NotRunning && securityTimer.elapsed() < 60000) {
        process.waitForFinished(35);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);
    }
    if (process.state() != QProcess::NotRunning) {
        process.kill(); process.waitForFinished(3000);
        QFile::remove(inputPath); QFile::remove(encryptedPath);
        emit errorOccurred(QStringLiteral("error.security_timeout"), {});
        return false;
    }
    QFile::remove(inputPath);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QFile::remove(encryptedPath);
        emit errorOccurred(QStringLiteral("error.security_create"), {});
        return false;
    }

    QPdfDocument check;
    const auto result = check.load(encryptedPath);
    if (result != QPdfDocument::Error::IncorrectPassword && result != QPdfDocument::Error::None) {
        QFile::remove(encryptedPath);
        emit errorOccurred(QStringLiteral("error.security_verify"), {});
        return false;
    }

    QString destination = normalizedPath(outputPath);
    if (!isContentUri(destination) && !destination.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        destination += QStringLiteral(".pdf");

    bool copied = false;
    if (isContentUri(destination)) {
        copied = copyLocalFileToDestination(encryptedPath, destination);
    } else {
        QFileInfo info(destination);
        QDir dir = info.dir();
        if (dir.exists() || dir.mkpath(QStringLiteral("."))) {
            const QString temp = dir.filePath(QStringLiteral(".") + info.fileName() + QStringLiteral(".")
                                              + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".secure.tmp.pdf"));
            QFile::remove(temp);
            if (QFile::copy(encryptedPath, temp)) {
                const QString backup = destination + QStringLiteral(".maenpdf-backup");
                const bool existed = QFile::exists(destination);
                if (existed) { QFile::remove(backup); copied = QFile::rename(destination, backup); }
                else copied = true;
                if (copied) {
                    copied = QFile::rename(temp, destination);
                    if (!copied && existed) QFile::rename(backup, destination);
                    if (copied && existed) QFile::remove(backup);
                }
                QFile::remove(temp);
            }
        }
    }
    QFile::remove(encryptedPath);
    if (!copied) {
        emit errorOccurred(QStringLiteral("error.content_write"), {});
        return false;
    }
    emit info(QStringLiteral("info.protected_created"), {});
    return true;
}

void PdfDocument::addBlankPage() {
    if (m_locked) return;
    const int at = m_pages.count();
    const PageItem item = blank();
    m_undo.push(new LambdaCommand(QStringLiteral("Add blank page"),
        [this, at] { m_pages.remove(at); emit pageCountChanged(); setCurrentPage(qMax(0, at - 1)); markModified(); },
        [this, at, item] { m_pages.insert(at, item); emit pageCountChanged(); setCurrentPage(at); markModified(); }));
}

void PdfDocument::deletePage(int index) {
    if (m_locked || m_pages.count() <= 1 || !m_pages.page(index)) return;
    const PageItem copy = *m_pages.page(index);
    m_undo.push(new LambdaCommand(QStringLiteral("Delete page"),
        [this, index, copy] { m_pages.insert(index, copy); emit pageCountChanged(); markModified(); },
        [this, index] { m_pages.remove(index); emit pageCountChanged(); markModified(); }));
    setCurrentPage(qMin(index, m_pages.count() - 1));
}

void PdfDocument::duplicatePage(int index) {
    if (m_locked || !m_pages.page(index)) return;
    PageItem copy = *m_pages.page(index);
    copy.uid.clear(); copy.revision = 0;
    m_undo.push(new LambdaCommand(QStringLiteral("Duplicate page"),
        [this, index] { m_pages.remove(index + 1); emit pageCountChanged(); markModified(); },
        [this, index, copy] { m_pages.insert(index + 1, copy); emit pageCountChanged(); markModified(); }));
}

void PdfDocument::movePage(int from, int to) {
    if (m_locked || from == to || !m_pages.page(from) || to < 0 || to >= m_pages.count()) return;
    m_undo.push(new LambdaCommand(QStringLiteral("Move page"),
        [this, from, to] { m_pages.movePage(to, from); markModified(); },
        [this, from, to] { m_pages.movePage(from, to); markModified(); }));
    setCurrentPage(to);
}

void PdfDocument::rotatePage(int index, int degrees) {
    if (m_locked) return;
    auto *page = m_pages.page(index);
    if (!page) return;
    const PageItem before = *page;
    PageItem after = before;
    const int normalizedDegrees = ((degrees % 360) + 360) % 360;

    if (after.base.isNull()) {
        after.rotation = (after.rotation + normalizedDegrees) % 360;
    } else {
        QTransform transform; transform.rotate(normalizedDegrees);
        after.base = after.base.transformed(transform, Qt::SmoothTransformation);
    }
    if (!after.overlay.isNull()) {
        QTransform transform; transform.rotate(normalizedDegrees);
        after.overlay = after.overlay.transformed(transform, Qt::SmoothTransformation);
    }
    if (normalizedDegrees != 0) {
        for (TextOverlayItem &item : after.textItems) {
            const double top = qBound(0.0, item.y - item.height, 1.0);
            const double left = qBound(0.0, item.x, 1.0);
            const double right = qBound(0.0, item.x + item.width, 1.0);
            const double bottom = qBound(0.0, item.y, 1.0);
            auto rotatePoint = [normalizedDegrees](const QPointF &point) {
                if (normalizedDegrees == 90)
                    return QPointF(1.0 - point.y(), point.x());
                if (normalizedDegrees == 180)
                    return QPointF(1.0 - point.x(), 1.0 - point.y());
                if (normalizedDegrees == 270)
                    return QPointF(point.y(), 1.0 - point.x());
                return point;
            };
            const QPointF a = rotatePoint(QPointF(left, top));
            const QPointF b = rotatePoint(QPointF(right, bottom));
            const double newLeft = qMin(a.x(), b.x());
            const double newTop = qMin(a.y(), b.y());
            const double newRight = qMax(a.x(), b.x());
            const double newBottom = qMax(a.y(), b.y());
            item.x = qBound(0.0, newLeft, 1.0);
            item.y = qBound(0.0, newBottom, 1.0);
            item.width = qBound(0.01, newRight - newLeft, 1.0 - item.x);
            item.height = qBound(0.01, newBottom - newTop, item.y);
        }
        for (FormOverlayItem &item : after.formItems) {
            auto rotatePoint = [normalizedDegrees](const QPointF &point) {
                if (normalizedDegrees == 90) return QPointF(1.0 - point.y(), point.x());
                if (normalizedDegrees == 180) return QPointF(1.0 - point.x(), 1.0 - point.y());
                if (normalizedDegrees == 270) return QPointF(point.y(), 1.0 - point.x());
                return point;
            };
            const QPointF a = rotatePoint(QPointF(item.x, item.y));
            const QPointF b = rotatePoint(QPointF(item.x + item.width, item.y + item.height));
            const double newLeft = qMin(a.x(), b.x());
            const double newTop = qMin(a.y(), b.y());
            const double newRight = qMax(a.x(), b.x());
            const double newBottom = qMax(a.y(), b.y());
            item.x = qBound(0.0, newLeft, 0.99);
            item.y = qBound(0.0, newTop, 0.99);
            item.width = qBound(0.01, newRight - newLeft, 1.0 - item.x);
            item.height = qBound(0.01, newBottom - newTop, 1.0 - item.y);
        }
    }
    if (normalizedDegrees == 90 || normalizedDegrees == 270)
        after.points = QSizeF(before.points.height(), before.points.width());

    m_undo.push(new LambdaCommand(QStringLiteral("Rotate page"),
        [this, index, before] { if (auto *p = m_pages.page(index)) { *p = before; m_pages.changed(index); markModified(); } },
        [this, index, after] { if (auto *p = m_pages.page(index)) { *p = after; m_pages.changed(index); markModified(); } }));
}

void PdfDocument::copyPage(int index) {
    if (const auto *page = m_pages.page(index)) {
        m_clipboard = *page;
        m_hasClipboard = true;
        emit clipboardChanged();
        emit info(QStringLiteral("info.page_copied"), {});
    }
}

void PdfDocument::pastePage(int afterIndex) {
    if (m_locked || !m_hasClipboard) return;
    const int at = qBound(0, afterIndex + 1, m_pages.count());
    PageItem item = m_clipboard;
    item.uid.clear(); item.revision = 0;
    m_undo.push(new LambdaCommand(QStringLiteral("Paste page"),
        [this, at] { m_pages.remove(at); emit pageCountChanged(); setCurrentPage(qMax(0, at - 1)); markModified(); },
        [this, at, item] { m_pages.insert(at, item); emit pageCountChanged(); setCurrentPage(at); markModified(); }));
}

void PdfDocument::snapshotCommand(int pageIndex, const QImage &before, const QImage &after, const QString &label) {
    m_undo.push(new LambdaCommand(label,
        [this, pageIndex, before] { if (auto *p = m_pages.page(pageIndex)) { p->overlay = before; m_pages.changed(pageIndex); markModified(); } },
        [this, pageIndex, after] { if (auto *p = m_pages.page(pageIndex)) { p->overlay = after; m_pages.changed(pageIndex); markModified(); } }));
}

QString PdfDocument::addText(int pageIndex, double x, double y, const QString &text, int fontSize) {
    if (m_locked || text.trimmed().isEmpty())
        return {};
    auto *page = m_pages.page(pageIndex);
    if (!page)
        return {};

    TextOverlayItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.text = normalizeEditableUnicode(text);
    item.x = qBound(0.0, x, 0.98);
    item.y = qBound(0.02, y, 1.0);
    item.fontSize = qBound(8, fontSize, 144);
    item.color = QStringLiteral("#111827");

    QFont metricsFont;
    metricsFont.setPixelSize(item.fontSize);
    const QFontMetricsF metrics(metricsFont);
    const QSizeF measured = measuredEditableText(item.text, metrics);
    const qreal pageWidth = qMax<qreal>(1.0, page->points.width());
    const qreal pageHeight = qMax<qreal>(1.0, page->points.height());
    item.width = qBound(0.015, measured.width() / pageWidth, 0.98 - item.x);
    item.height = qBound(0.012, measured.height() / pageHeight, qMin(0.40, item.y));

    const int at = page->textItems.size();
    page->textItems.push_back(item);
    m_pages.changed(pageIndex);
    markModified();

    m_undo.push(new LambdaCommand(QStringLiteral("Add text"),
        [this, pageIndex, id = item.id] {
            if (auto *p = m_pages.page(pageIndex)) {
                for (int i = 0; i < p->textItems.size(); ++i) {
                    if (p->textItems[i].id == id) {
                        p->textItems.removeAt(i);
                        m_pages.changed(pageIndex);
                        markModified();
                        break;
                    }
                }
            }
        },
        [this, pageIndex, item, at] {
            if (auto *p = m_pages.page(pageIndex)) {
                const int insertAt = qBound(0, at, p->textItems.size());
                p->textItems.insert(insertAt, item);
                m_pages.changed(pageIndex);
                markModified();
            }
        },
        true));
    return item.id;
}

QVariantList PdfDocument::textAnnotations(int pageIndex) const {
    QVariantList result;
    const auto *page = m_pages.page(pageIndex);
    if (!page)
        return result;
    result.reserve(page->textItems.size());
    for (const TextOverlayItem &item : page->textItems) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), item.id);
        row.insert(QStringLiteral("text"), item.text);
        row.insert(QStringLiteral("x"), item.x);
        row.insert(QStringLiteral("y"), qBound(0.0, item.y - item.height, 1.0));
        row.insert(QStringLiteral("w"), item.width);
        row.insert(QStringLiteral("h"), item.height);
        row.insert(QStringLiteral("fontSize"), item.fontSize);
        row.insert(QStringLiteral("strikeStart"), item.strikeStart);
        row.insert(QStringLiteral("strikeLength"), item.strikeLength);
        result.push_back(row);
    }
    return result;
}

QVariantMap PdfDocument::textAnnotation(int pageIndex, const QString &id) const {
    const auto *page = m_pages.page(pageIndex);
    if (!page || id.isEmpty())
        return {};
    for (const TextOverlayItem &item : page->textItems) {
        if (item.id != id)
            continue;
        return {{QStringLiteral("id"), item.id},
                {QStringLiteral("text"), item.text},
                {QStringLiteral("x"), item.x},
                {QStringLiteral("y"), qBound(0.0, item.y - item.height, 1.0)},
                {QStringLiteral("baselineY"), item.y},
                {QStringLiteral("w"), item.width},
                {QStringLiteral("h"), item.height},
                {QStringLiteral("fontSize"), item.fontSize},
                {QStringLiteral("strikeStart"), item.strikeStart},
                {QStringLiteral("strikeLength"), item.strikeLength}};
    }
    return {};
}

bool PdfDocument::updateTextAnnotation(int pageIndex, const QString &id, const QString &text,
                                       double x, double y, int fontSize, int strikeStart, int strikeLength) {
    if (m_locked || id.isEmpty() || text.isEmpty())
        return false;
    auto *page = m_pages.page(pageIndex);
    if (!page)
        return false;
    int at = -1;
    for (int i = 0; i < page->textItems.size(); ++i) {
        if (page->textItems[i].id == id) { at = i; break; }
    }
    if (at < 0)
        return false;

    const TextOverlayItem before = page->textItems[at];
    TextOverlayItem after = before;
    after.text = normalizeEditableUnicode(text);
    after.x = qBound(0.0, x, 0.98);
    after.fontSize = qBound(8, fontSize, 144);
    const int textSize = int(after.text.size());
    after.strikeStart = qBound(-1, strikeStart, textSize);
    after.strikeLength = after.strikeStart < 0 ? 0 : qBound(0, strikeLength, textSize - after.strikeStart);

    QFont metricsFont;
    metricsFont.setPixelSize(after.fontSize);
    const QFontMetricsF metrics(metricsFont);
    const QSizeF measured = measuredEditableText(after.text, metrics);
    const qreal pageWidth = qMax<qreal>(1.0, page->points.width());
    const qreal pageHeight = qMax<qreal>(1.0, page->points.height());
    after.width = qBound(0.015, measured.width() / pageWidth, qMax(0.015, 0.98 - after.x));
    after.height = qBound(0.012, measured.height() / pageHeight, 0.40);
    // QML supplies the top edge while the renderer stores a baseline.
    after.y = qBound(after.height, y + after.height, 1.0);

    if (before.text == after.text && qFuzzyCompare(before.x + 1.0, after.x + 1.0)
        && qFuzzyCompare(before.y + 1.0, after.y + 1.0) && before.fontSize == after.fontSize
        && before.strikeStart == after.strikeStart && before.strikeLength == after.strikeLength)
        return true;

    page->textItems[at] = after;
    m_pages.changed(pageIndex);
    markModified();
    m_undo.push(new LambdaCommand(QStringLiteral("Edit inserted text"),
        [this, pageIndex, at, before] {
            if (auto *p = m_pages.page(pageIndex); p && at >= 0 && at < p->textItems.size()) {
                p->textItems[at] = before; m_pages.changed(pageIndex); markModified();
            }
        },
        [this, pageIndex, at, after] {
            if (auto *p = m_pages.page(pageIndex); p && at >= 0 && at < p->textItems.size()) {
                p->textItems[at] = after; m_pages.changed(pageIndex); markModified();
            }
        }, true));
    return true;
}

bool PdfDocument::moveTextAnnotation(int pageIndex, const QString &id, double x, double y) {
    const QVariantMap item = textAnnotation(pageIndex, id);
    if (item.isEmpty()) return false;
    return updateTextAnnotation(pageIndex, id, item.value(QStringLiteral("text")).toString(), x, y,
                                item.value(QStringLiteral("fontSize")).toInt(),
                                item.value(QStringLiteral("strikeStart"), -1).toInt(),
                                item.value(QStringLiteral("strikeLength"), 0).toInt());
}

bool PdfDocument::resizeTextAnnotation(int pageIndex, const QString &id, int fontSize) {
    const QVariantMap item = textAnnotation(pageIndex, id);
    if (item.isEmpty()) return false;
    return updateTextAnnotation(pageIndex, id, item.value(QStringLiteral("text")).toString(),
                                item.value(QStringLiteral("x")).toDouble(), item.value(QStringLiteral("y")).toDouble(),
                                fontSize, item.value(QStringLiteral("strikeStart"), -1).toInt(),
                                item.value(QStringLiteral("strikeLength"), 0).toInt());
}

QString PdfDocument::addFormField(int pageIndex, const QString &type, double x, double y, double w, double h) {
    if (m_locked)
        return {};
    auto *page = m_pages.page(pageIndex);
    if (!page)
        return {};
    QString kind = type.trimmed().toLower();
    if (kind != QStringLiteral("text") && kind != QStringLiteral("checkbox")
        && kind != QStringLiteral("radio") && kind != QStringLiteral("dropdown"))
        return {};

    FormOverlayItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.type = kind;
    item.x = qBound(0.0, x, 0.95);
    item.y = qBound(0.0, y, 0.95);
    item.width = qBound(0.025, w, 1.0 - item.x);
    item.height = qBound(0.025, h, 1.0 - item.y);
    if (kind == QStringLiteral("checkbox") || kind == QStringLiteral("radio")) {
        item.width = qMin(item.width, 0.045);
        item.height = qMin(item.height, 0.045);
    }
    if (kind == QStringLiteral("dropdown")) {
        item.options = {QStringLiteral("Option 1"), QStringLiteral("Option 2"), QStringLiteral("Option 3")};
        item.value = item.options.first();
    }
    const int at = page->formItems.size();
    page->formItems.push_back(item);
    m_pages.changed(pageIndex);
    markModified();
    m_undo.push(new LambdaCommand(QStringLiteral("Add form field"),
        [this, pageIndex, id = item.id] {
            if (auto *p = m_pages.page(pageIndex)) {
                for (int i = 0; i < p->formItems.size(); ++i) if (p->formItems[i].id == id) {
                    p->formItems.removeAt(i); m_pages.changed(pageIndex); markModified(); break;
                }
            }
        },
        [this, pageIndex, item, at] {
            if (auto *p = m_pages.page(pageIndex)) {
                p->formItems.insert(qBound(0, at, p->formItems.size()), item); m_pages.changed(pageIndex); markModified();
            }
        }, true));
    return item.id;
}

QVariantList PdfDocument::formAnnotations(int pageIndex) const {
    QVariantList result;
    const auto *page = m_pages.page(pageIndex);
    if (!page) return result;
    for (const FormOverlayItem &item : page->formItems) {
        QVariantMap row{{QStringLiteral("id"), item.id}, {QStringLiteral("type"), item.type},
                        {QStringLiteral("label"), item.label}, {QStringLiteral("value"), item.value},
                        {QStringLiteral("options"), item.options}, {QStringLiteral("x"), item.x},
                        {QStringLiteral("y"), item.y}, {QStringLiteral("w"), item.width},
                        {QStringLiteral("h"), item.height}, {QStringLiteral("checked"), item.checked},
                        {QStringLiteral("selectedIndex"), item.selectedIndex}};
        result.push_back(row);
    }
    return result;
}

QVariantMap PdfDocument::formAnnotation(int pageIndex, const QString &id) const {
    const QVariantList items = formAnnotations(pageIndex);
    for (const QVariant &value : items) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("id")).toString() == id) return item;
    }
    return {};
}

bool PdfDocument::updateFormField(int pageIndex, const QString &id, const QString &value, bool checked, int selectedIndex) {
    if (m_locked || id.isEmpty()) return false;
    auto *page = m_pages.page(pageIndex);
    if (!page) return false;
    int at = -1;
    for (int i = 0; i < page->formItems.size(); ++i) if (page->formItems[i].id == id) { at = i; break; }
    if (at < 0) return false;
    const FormOverlayItem before = page->formItems[at];
    FormOverlayItem after = before;
    after.value = normalizeEditableUnicode(value);
    after.checked = checked;
    if (!after.options.isEmpty()) {
        after.selectedIndex = qBound(0, selectedIndex, int(after.options.size()) - 1);
        if (after.type == QStringLiteral("dropdown")) after.value = after.options.value(after.selectedIndex);
    }
    page->formItems[at] = after;
    m_pages.changed(pageIndex); markModified();
    m_undo.push(new LambdaCommand(QStringLiteral("Edit form field"),
        [this, pageIndex, at, before] { if (auto *p = m_pages.page(pageIndex); p && at < p->formItems.size()) { p->formItems[at] = before; m_pages.changed(pageIndex); markModified(); } },
        [this, pageIndex, at, after] { if (auto *p = m_pages.page(pageIndex); p && at < p->formItems.size()) { p->formItems[at] = after; m_pages.changed(pageIndex); markModified(); } }, true));
    return true;
}

bool PdfDocument::deleteFormField(int pageIndex, const QString &id) {
    if (m_locked || id.isEmpty()) return false;
    auto *page = m_pages.page(pageIndex);
    if (!page) return false;
    int at = -1; FormOverlayItem removed;
    for (int i = 0; i < page->formItems.size(); ++i) if (page->formItems[i].id == id) { at = i; removed = page->formItems[i]; break; }
    if (at < 0) return false;
    page->formItems.removeAt(at); m_pages.changed(pageIndex); markModified();
    m_undo.push(new LambdaCommand(QStringLiteral("Delete form field"),
        [this, pageIndex, at, removed] { if (auto *p = m_pages.page(pageIndex)) { p->formItems.insert(qBound(0, at, p->formItems.size()), removed); m_pages.changed(pageIndex); markModified(); } },
        [this, pageIndex, id] { if (auto *p = m_pages.page(pageIndex)) { for (int i=0;i<p->formItems.size();++i) if (p->formItems[i].id==id) { p->formItems.removeAt(i); m_pages.changed(pageIndex); markModified(); break; } } }, true));
    return true;
}

bool PdfDocument::deleteTextAnnotation(int pageIndex, const QString &id) {
    if (m_locked || id.isEmpty())
        return false;
    auto *page = m_pages.page(pageIndex);
    if (!page)
        return false;
    int at = -1;
    TextOverlayItem removed;
    for (int i = 0; i < page->textItems.size(); ++i) {
        if (page->textItems[i].id == id) {
            at = i;
            removed = page->textItems[i];
            break;
        }
    }
    if (at < 0)
        return false;

    page->textItems.removeAt(at);
    m_pages.changed(pageIndex);
    markModified();
    m_undo.push(new LambdaCommand(QStringLiteral("Delete inserted text"),
        [this, pageIndex, removed, at] {
            if (auto *p = m_pages.page(pageIndex)) {
                p->textItems.insert(qBound(0, at, p->textItems.size()), removed);
                m_pages.changed(pageIndex);
                markModified();
            }
        },
        [this, pageIndex, id] {
            if (auto *p = m_pages.page(pageIndex)) {
                for (int i = 0; i < p->textItems.size(); ++i) {
                    if (p->textItems[i].id == id) {
                        p->textItems.removeAt(i);
                        m_pages.changed(pageIndex);
                        markModified();
                        break;
                    }
                }
            }
        },
        true));
    return true;
}

void PdfDocument::addHighlight(int pageIndex, double x, double y, double width, double height) {
    QVariantMap rect{{QStringLiteral("x"), x}, {QStringLiteral("y"), y},
                     {QStringLiteral("w"), width}, {QStringLiteral("h"), height}};
    addHighlightRects(pageIndex, QVariantList{rect}, QStringLiteral("#FFD740"), 42);
}

void PdfDocument::addHighlightRects(int pageIndex, const QVariantList &rects, const QString &color, int opacity) {
    if (m_locked || rects.isEmpty()) return;
    auto *page = m_pages.page(pageIndex); if (!page) return;
    ensureOverlay(page);

    QColor fill(color);
    if (!fill.isValid()) fill = QColor(QStringLiteral("#FFD740"));
    fill.setAlpha(qBound(5, opacity, 100) * 255 / 100);

    QVector<QRectF> normalizedRects;
    QRect pixelBounds;
    for (const QVariant &value : rects) {
        const QVariantMap r = value.toMap();
        const qreal x = qBound(0.0, r.value(QStringLiteral("x")).toDouble(), 1.0);
        const qreal y = qBound(0.0, r.value(QStringLiteral("y")).toDouble(), 1.0);
        const qreal w = qBound(0.0, r.value(QStringLiteral("w")).toDouble(), 1.0 - x);
        const qreal h = qBound(0.0, r.value(QStringLiteral("h")).toDouble(), 1.0 - y);
        if (w <= 0.0005 || h <= 0.0005) continue;
        const QRectF nr(x, y, w, h);
        normalizedRects.push_back(nr);
        const QRect px(qFloor(x * page->overlay.width()) - 2,
                       qFloor(y * page->overlay.height()) - 2,
                       qCeil(w * page->overlay.width()) + 4,
                       qCeil(h * page->overlay.height()) + 4);
        pixelBounds = pixelBounds.isNull() ? px : pixelBounds.united(px);
    }
    pixelBounds = pixelBounds.intersected(page->overlay.rect());
    if (normalizedRects.isEmpty() || pixelBounds.isEmpty()) return;

    const QImage beforePatch = page->overlay.copy(pixelBounds);
    auto paintRects = [](QImage &target, const QVector<QRectF> &items, const QColor &brush) {
        QPainter painter(&target);
        for (const QRectF &r : items)
            painter.fillRect(QRectF(r.x() * target.width(), r.y() * target.height(),
                                    r.width() * target.width(), r.height() * target.height()), brush);
    };
    paintRects(page->overlay, normalizedRects, fill);
    m_pages.changed(pageIndex);
    markModified();

    m_undo.push(new LambdaCommand(QStringLiteral("Highlight text"),
        [this, pageIndex, pixelBounds, beforePatch] {
            if (auto *p = m_pages.page(pageIndex)) {
                ensureOverlay(p);
                QPainter restore(&p->overlay);
                restore.setCompositionMode(QPainter::CompositionMode_Source);
                restore.drawImage(pixelBounds.topLeft(), beforePatch);
                restore.end();
                m_pages.changed(pageIndex); markModified();
            }
        },
        [this, pageIndex, normalizedRects, fill, paintRects] {
            if (auto *p = m_pages.page(pageIndex)) {
                ensureOverlay(p); paintRects(p->overlay, normalizedRects, fill);
                m_pages.changed(pageIndex); markModified();
            }
        }, true));
}

void PdfDocument::addRedaction(int pageIndex, double x, double y, double width, double height) {
    if (m_locked) return;
    auto *page = m_pages.page(pageIndex);
    if (!page) return;
    const double nx = qBound(0.0, qMin(x, x + width), 1.0);
    const double ny = qBound(0.0, qMin(y, y + height), 1.0);
    const double nw = qBound(0.0, qAbs(width), 1.0 - nx);
    const double nh = qBound(0.0, qAbs(height), 1.0 - ny);
    if (nw < 0.002 || nh < 0.002) return;
    ensureOverlay(page);
    QRect patchRect(qFloor(nx * page->overlay.width()) - 2,
                    qFloor(ny * page->overlay.height()) - 2,
                    qCeil(nw * page->overlay.width()) + 4,
                    qCeil(nh * page->overlay.height()) + 4);
    patchRect = patchRect.intersected(page->overlay.rect());
    if (patchRect.isEmpty()) return;
    const QImage beforePatch = page->overlay.copy(patchRect);
    auto paintRedaction = [nx, ny, nw, nh](QImage &target) {
        QPainter painter(&target);
        painter.fillRect(QRectF(nx * target.width(), ny * target.height(),
                                nw * target.width(), nh * target.height()), Qt::black);
    };
    paintRedaction(page->overlay);
    m_pages.changed(pageIndex); markModified();
    m_undo.push(new LambdaCommand(QStringLiteral("Secure flattened redaction"),
        [this, pageIndex, patchRect, beforePatch] {
            if (auto *p = m_pages.page(pageIndex)) {
                ensureOverlay(p);
                QPainter restore(&p->overlay);
                restore.setCompositionMode(QPainter::CompositionMode_Source);
                restore.drawImage(patchRect.topLeft(), beforePatch);
                restore.end(); m_pages.changed(pageIndex); markModified();
            }
        },
        [this, pageIndex, paintRedaction] {
            if (auto *p = m_pages.page(pageIndex)) {
                ensureOverlay(p); paintRedaction(p->overlay); m_pages.changed(pageIndex); markModified();
            }
        }, true));
    emit info(QStringLiteral("info.redaction_added"), {});
}

void PdfDocument::cropPage(int pageIndex, double x, double y, double width, double height) {
    if (m_locked) return;
    auto *page = m_pages.page(pageIndex);
    if (!page) return;
    const double nx = qBound(0.0, qMin(x, x + width), 1.0);
    const double ny = qBound(0.0, qMin(y, y + height), 1.0);
    const double nw = qBound(0.0, qAbs(width), 1.0 - nx);
    const double nh = qBound(0.0, qAbs(height), 1.0 - ny);
    if (nw < 0.05 || nh < 0.05) return;

    const PageItem before = *page;
    QSize renderSize = (page->points * 1.5).toSize();
    const int maxDim = MemoryPolicy::maxRenderDimension();
    renderSize.setWidth(qBound(600, renderSize.width(), maxDim));
    renderSize.setHeight(qBound(800, renderSize.height(), maxDim));
    const QImage rendered = m_pages.renderPage(pageIndex, renderSize);
    if (rendered.isNull()) return;
    QRect crop(qRound(nx * rendered.width()), qRound(ny * rendered.height()),
               qRound(nw * rendered.width()), qRound(nh * rendered.height()));
    crop = crop.intersected(rendered.rect());
    if (crop.width() < 32 || crop.height() < 32) return;

    PageItem after = before;
    after.base = rendered.copy(crop);
    after.overlay = QImage();
    after.sourceId.clear();
    after.sourcePage = -1;
    after.rotation = 0;
    after.points = QSizeF(before.points.width() * nw, before.points.height() * nh);
    after.watermarkText.clear();
    after.watermarkOpacity = 0;
    after.pageNumber = false;
    after.batesText.clear();
    after.textItems.clear(); // crop result already contains flattened inserted text
    after.formItems.clear(); // crop result already contains flattened form widgets

    m_undo.push(new LambdaCommand(QStringLiteral("Crop page"),
        [this, pageIndex, before] { if (auto *p = m_pages.page(pageIndex)) { *p = before; m_pages.changed(pageIndex); markModified(); } },
        [this, pageIndex, after] { if (auto *p = m_pages.page(pageIndex)) { *p = after; m_pages.changed(pageIndex); markModified(); } }));
    emit info(QStringLiteral("info.page_cropped"), {});
}

void PdfDocument::addInk(int pageIndex, const QVariantList &points) {
    addInkStyled(pageIndex, points, QStringLiteral("#185EB4"), 0.004, 100);
}

void PdfDocument::addHighlightInkStyled(int pageIndex, const QVariantList &points, const QString &color,
                                        double widthRatio, int opacity) {
    // Freehand highlighter deliberately reuses the lightweight touched-patch
    // stroke engine: it stays fast on 4 GB devices while supporting unlimited
    // Undo history without page-sized bitmap snapshots per stroke.
    addInkStyled(pageIndex, points, color, widthRatio, qBound(10, opacity, 80));
}

void PdfDocument::addInkStyled(int pageIndex, const QVariantList &points, const QString &color,
                               double widthRatio, int opacity) {
    if (m_locked || points.size() < 4)
        return;
    auto *page = m_pages.page(pageIndex);
    if (!page)
        return;
    ensureOverlay(page);

    // Drawing used to detach and retain a full-page overlay bitmap for every
    // stroke. On long sessions that could consume hundreds of megabytes and
    // make the UI appear frozen. Keep only the small pixel patch touched by
    // this stroke; undo restores that patch and redo replays the vector path.
    QVector<QPointF> normalizedPoints;
    normalizedPoints.reserve(points.size() / 2);
    qreal minX = 1.0, minY = 1.0, maxX = 0.0, maxY = 0.0;
    for (int i = 0; i + 1 < points.size(); i += 2) {
        const qreal nx = qBound<qreal>(0.0, points[i].toDouble(), 1.0);
        const qreal ny = qBound<qreal>(0.0, points[i + 1].toDouble(), 1.0);
        normalizedPoints.push_back(QPointF(nx, ny));
        minX = qMin(minX, nx); minY = qMin(minY, ny);
        maxX = qMax(maxX, nx); maxY = qMax(maxY, ny);
    }
    if (normalizedPoints.size() < 2)
        return;

    QColor penColor(color);
    if (!penColor.isValid())
        penColor = QColor(QStringLiteral("#185EB4"));
    penColor.setAlpha(qBound(5, opacity, 100) * 255 / 100);
    const qreal boundedWidthRatio = qBound<qreal>(0.0005, widthRatio, 0.05);
    const qreal penWidth = qMax<qreal>(1.0, boundedWidthRatio * page->overlay.width());
    const int margin = qMax(3, qCeil(penWidth * 1.5));
    QRect patchRect(QPoint(qFloor(minX * page->overlay.width()) - margin,
                           qFloor(minY * page->overlay.height()) - margin),
                    QPoint(qCeil(maxX * page->overlay.width()) + margin,
                           qCeil(maxY * page->overlay.height()) + margin));
    patchRect = patchRect.normalized().intersected(page->overlay.rect());
    if (patchRect.isEmpty())
        return;
    const QImage beforePatch = page->overlay.copy(patchRect);

    auto paintStroke = [](QImage &target, const QVector<QPointF> &stroke, const QColor &strokeColor, qreal ratio) {
        if (target.isNull() || stroke.size() < 2)
            return;
        QPainter painter(&target);
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal width = qMax<qreal>(1.0, ratio * target.width());
        painter.setPen(QPen(strokeColor, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath path;
        path.moveTo(stroke.first().x() * target.width(), stroke.first().y() * target.height());
        for (qsizetype i = 1; i < stroke.size(); ++i)
            path.lineTo(stroke[i].x() * target.width(), stroke[i].y() * target.height());
        painter.drawPath(path);
    };

    paintStroke(page->overlay, normalizedPoints, penColor, boundedWidthRatio);
    m_pages.changed(pageIndex);
    markModified();

    m_undo.push(new LambdaCommand(QStringLiteral("Draw"),
        [this, pageIndex, patchRect, beforePatch] {
            if (auto *p = m_pages.page(pageIndex)) {
                ensureOverlay(p);
                QPainter restore(&p->overlay);
                restore.setCompositionMode(QPainter::CompositionMode_Source);
                restore.drawImage(patchRect.topLeft(), beforePatch);
                restore.end();
                m_pages.changed(pageIndex);
                markModified();
            }
        },
        [this, pageIndex, normalizedPoints, penColor, boundedWidthRatio, paintStroke] {
            if (auto *p = m_pages.page(pageIndex)) {
                ensureOverlay(p);
                paintStroke(p->overlay, normalizedPoints, penColor, boundedWidthRatio);
                m_pages.changed(pageIndex);
                markModified();
            }
        }, true));
}

void PdfDocument::addImage(int pageIndex, const QString &path, double x, double y, double width, double height) {
    if (m_locked) return;
    auto *page = m_pages.page(pageIndex); if (!page) return;
    const QString readable = prepareReadableImagePath(path);
    if (readable.isEmpty()) return;
    QImage image(readable);
    if (image.isNull()) { emit errorOccurred(QStringLiteral("error.image_read"), {}); return; }
    ensureOverlay(page);
    const QImage before = page->overlay;
    QImage after = before;
    QPainter painter(&after);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRectF(x * after.width(), y * after.height(), width * after.width(), height * after.height()), image);
    painter.end();
    snapshotCommand(pageIndex, before, after, QStringLiteral("Add image"));
}

void PdfDocument::addWatermark(const QString &text, int fontSize, int opacity) {
    if (m_locked || text.isEmpty()) return;
    QVector<QString> oldText;
    QVector<int> oldSize, oldOpacity;
    oldText.reserve(m_pages.count()); oldSize.reserve(m_pages.count()); oldOpacity.reserve(m_pages.count());
    for (int i = 0; i < m_pages.count(); ++i) {
        auto *page = m_pages.page(i);
        oldText.push_back(page->watermarkText); oldSize.push_back(page->watermarkFontSize); oldOpacity.push_back(page->watermarkOpacity);
        page->watermarkText = text; page->watermarkFontSize = fontSize; page->watermarkOpacity = qBound(0, opacity, 100);
        m_pages.changed(i);
    }
    m_undo.push(new LambdaCommand(QStringLiteral("Watermark"),
        [this, oldText, oldSize, oldOpacity] {
            for (int i = 0; i < oldText.size() && i < m_pages.count(); ++i) {
                auto *p = m_pages.page(i); p->watermarkText = oldText[i]; p->watermarkFontSize = oldSize[i]; p->watermarkOpacity = oldOpacity[i]; m_pages.changed(i);
            } markModified();
        },
        [this, text, fontSize, opacity] {
            for (int i = 0; i < m_pages.count(); ++i) {
                auto *p = m_pages.page(i); p->watermarkText = text; p->watermarkFontSize = fontSize; p->watermarkOpacity = qBound(0, opacity, 100); m_pages.changed(i);
            } markModified();
        }, true));
    markModified();
    emit info(QStringLiteral("info.watermark_applied"), {});
}

void PdfDocument::addPageNumbers() {
    if (m_locked) return;
    QVector<bool> before;
    before.reserve(m_pages.count());
    for (int i = 0; i < m_pages.count(); ++i) {
        before.push_back(m_pages.page(i)->pageNumber);
        m_pages.page(i)->pageNumber = true;
        m_pages.changed(i);
    }
    m_undo.push(new LambdaCommand(QStringLiteral("Page numbers"),
        [this, before] {
            for (int i = 0; i < before.size() && i < m_pages.count(); ++i) { m_pages.page(i)->pageNumber = before[i]; m_pages.changed(i); }
            markModified();
        },
        [this] {
            for (int i = 0; i < m_pages.count(); ++i) { m_pages.page(i)->pageNumber = true; m_pages.changed(i); }
            markModified();
        }, true));
    markModified();
    emit info(QStringLiteral("info.page_numbers_added"), {});
}

void PdfDocument::addBatesNumbers(const QString &prefix, int start, int padding) {
    if (m_locked) return;
    const int safePadding = qBound(1, padding, 12);
    QVector<QString> before;
    before.reserve(m_pages.count());
    QVector<QString> after;
    after.reserve(m_pages.count());
    for (int i = 0; i < m_pages.count(); ++i) {
        before.push_back(m_pages.page(i)->batesText);
        const QString number = QString::number(qMax(0, start + i)).rightJustified(safePadding, QLatin1Char('0'));
        const QString value = prefix + number;
        after.push_back(value);
        m_pages.page(i)->batesText = value;
        m_pages.changed(i);
    }
    m_undo.push(new LambdaCommand(QStringLiteral("Bates numbering"),
        [this, before] {
            for (int i = 0; i < before.size() && i < m_pages.count(); ++i) { m_pages.page(i)->batesText = before[i]; m_pages.changed(i); }
            markModified();
        },
        [this, after] {
            for (int i = 0; i < after.size() && i < m_pages.count(); ++i) { m_pages.page(i)->batesText = after[i]; m_pages.changed(i); }
            markModified();
        }, true));
    markModified();
    emit info(QStringLiteral("info.bates_added"), {m_pages.count()});
}

void PdfDocument::setLocked(bool value) {
    if (m_locked == value) return;
    m_locked = value;
    emit lockedChanged();
}

void PdfDocument::setCurrentPage(int value) {
    value = qBound(0, value, qMax(0, m_pages.count() - 1));
    if (value == m_currentPage) return;
    m_currentPage = value;
    emit currentPageChanged();
}

QVariantMap PdfDocument::properties() const {
    QVariantMap map;
    map[QStringLiteral("title")] = title();
    map[QStringLiteral("path")] = m_filePath;
    map[QStringLiteral("pages")] = m_pages.count();
    map[QStringLiteral("modified")] = m_modified;
    map[QStringLiteral("sizeBytes")] = (!m_filePath.isEmpty() && !isContentUri(m_filePath)) ? QFileInfo(m_filePath).size() : 0;
    map[QStringLiteral("format")] = QStringLiteral("PDF");
    return map;
}
