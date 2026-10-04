#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QUndoStack>
#include <QVariantMap>
#include <QTimer>
#include <memory>

#include "PageModel.h"

class IPdfBackend;

class PdfDocument final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QString title READ title NOTIFY filePathChanged)
    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QObject *pages READ pages CONSTANT)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(bool hasPageClipboard READ hasPageClipboard NOTIFY clipboardChanged)
    Q_PROPERTY(bool recoveryAvailable READ recoveryAvailable NOTIFY recoveryAvailableChanged)
    Q_PROPERTY(QString engineName READ engineName CONSTANT)
public:
    explicit PdfDocument(QObject *parent = nullptr);
    ~PdfDocument() override;

    QString filePath() const { return m_filePath; }
    QString title() const;
    bool locked() const { return m_locked; }
    bool modified() const { return m_modified; }
    int pageCount() const { return m_pages.count(); }
    int currentPage() const { return m_currentPage; }
    QObject *pages() { return &m_pages; }
    PageModel *pageModel() { return &m_pages; }
    bool canUndo() const { return m_undo.canUndo(); }
    bool canRedo() const { return m_undo.canRedo(); }
    bool hasPageClipboard() const { return m_hasClipboard; }
    bool recoveryAvailable() const { return m_recoveryAvailable; }
    QString engineName() const { return QStringLiteral("Orbis Engine 6"); }

    Q_INVOKABLE bool newDocument();
    Q_INVOKABLE bool openDocument(const QString &path);
    Q_INVOKABLE bool appendPdf(const QString &path);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QString &path);
    Q_INVOKABLE bool extractPage(int page, const QString &path);
    Q_INVOKABLE bool protectCopy(const QString &output, const QString &userPassword, const QString &ownerPassword,
                                 bool allowPrint, bool allowCopy, bool allowModify);

    Q_INVOKABLE void addBlankPage();
    Q_INVOKABLE void deletePage(int page);
    Q_INVOKABLE void duplicatePage(int page);
    Q_INVOKABLE void movePage(int from, int to);
    Q_INVOKABLE void rotatePage(int page, int degrees = 90);
    Q_INVOKABLE void copyPage(int page);
    Q_INVOKABLE void pastePage(int afterIndex);
    Q_INVOKABLE void addImage(int page, const QString &path, double x = .15, double y = .15, double w = .5, double h = .5);
    Q_INVOKABLE void addText(int page, double x, double y, const QString &text, int fontSize = 18);
    Q_INVOKABLE void addHighlight(int page, double x, double y, double w, double h);
    Q_INVOKABLE void addInk(int page, const QVariantList &points);
    Q_INVOKABLE void addWatermark(const QString &text, int fontSize = 42, int opacity = 45);
    Q_INVOKABLE void addPageNumbers();

    Q_INVOKABLE void undo() { m_undo.undo(); }
    Q_INVOKABLE void redo() { m_undo.redo(); }
    Q_INVOKABLE bool validateCurrentDocument() const;
    Q_INVOKABLE QVariantList searchText(const QString &query, int maxResults = 200) const;
    Q_INVOKABLE QVariantMap engineCapabilities() const;
    Q_INVOKABLE QVariantMap cacheStats() const { return m_pages.cacheStats(); }
    Q_INVOKABLE bool recoverAutosave();
    Q_INVOKABLE void discardRecovery();
    Q_INVOKABLE QString recoveryPath() const;
    Q_INVOKABLE QString diagnosticLogPath() const;
    Q_INVOKABLE void setLocked(bool locked);
    Q_INVOKABLE void setCurrentPage(int page);
    Q_INVOKABLE QString normalizedPath(const QString &path) const;
    Q_INVOKABLE QVariantMap properties() const;

signals:
    void filePathChanged();
    void recoveryAvailableChanged();
    void lockedChanged();
    void modifiedChanged();
    void pageCountChanged();
    void currentPageChanged();
    void historyChanged();
    void clipboardChanged();
    void errorOccurred(QString key, QVariantList args);
    void info(QString key, QVariantList args);

private:
    QString m_filePath;
    bool m_locked{false};
    bool m_modified{false};
    bool m_hasClipboard{false};
    bool m_recoveryAvailable{false};
    int m_currentPage{0};
    PageModel m_pages;
    QUndoStack m_undo;
    PageItem m_clipboard;
    QTimer m_autosaveTimer;
    QHash<QString, std::shared_ptr<IPdfBackend>> m_backends;
    QStringList m_tempInputs;

    void markModified(bool value = true);
    PageItem blank() const;
    bool loadIntoModel(const QString &path, bool append);
    QImage renderBase(const PageItem &page, const QSize &requestedSize) const;
    void ensureOverlay(PageItem *page);
    bool exportPdf(const QString &localPath, int onlyPage = -1);
    bool atomicExportPdf(const QString &localTarget);
    bool exportToDestination(const QString &destination, int onlyPage = -1, bool atomicFullDocument = false);
    bool copyLocalFileToDestination(const QString &localPath, const QString &destination) const;
    QString prepareReadablePath(const QString &path);
    QString prepareReadableImagePath(const QString &path);
    bool isContentUri(const QString &path) const;
    void cleanupTemporaryInputs();
    void snapshotCommand(int page, const QImage &before, const QImage &after, const QString &label);
    void autosave();
    void refreshRecoveryState();
};
