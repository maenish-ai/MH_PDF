#pragma once

#include <QObject>
#include <QImage>
#include <QSize>
#include <QVariantList>
#include <QVector>

class PdfDocument;
class AppSettings;

class DocumentManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *currentDocument READ currentDocument NOTIFY currentDocumentChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QVariantList tabs READ tabs NOTIFY tabsChanged)
    Q_PROPERTY(int count READ count NOTIFY tabsChanged)
    Q_PROPERTY(bool hasModifiedDocuments READ hasModifiedDocuments NOTIFY modifiedStateChanged)
public:
    explicit DocumentManager(AppSettings *settings, QObject *parent = nullptr);
    ~DocumentManager() override;

    QObject *currentDocument() const;
    PdfDocument *currentPdfDocument() const;
    int currentIndex() const { return m_currentIndex; }
    int count() const { return m_documents.size(); }
    QVariantList tabs() const;
    bool hasModifiedDocuments() const;

    Q_INVOKABLE int newTab();
    Q_INVOKABLE bool openDocument(const QString &path);
    Q_INVOKABLE bool openDocumentWithPassword(const QString &path, const QString &password);
    Q_INVOKABLE bool closeTab(int index);
    Q_INVOKABLE void setCurrentIndex(int index);
    Q_INVOKABLE bool closeCurrentTab() { return closeTab(m_currentIndex); }
    Q_INVOKABLE bool closeOtherTabs(int keepIndex);
    Q_INVOKABLE void refreshMemoryPolicy();

    QImage renderPage(const QString &sessionId, int page, const QSize &requestedSize) const;

signals:
    void currentDocumentChanged();
    void currentIndexChanged();
    void tabsChanged();
    void modifiedStateChanged();
    void documentPathAvailable(QString path);

private:
    PdfDocument *createDocument();
    void connectDocument(PdfDocument *document);
    PdfDocument *documentForSession(const QString &sessionId) const;

    AppSettings *m_settings{nullptr};
    QVector<PdfDocument *> m_documents;
    int m_currentIndex{-1};
    quint64 m_nextSession{1};
};
