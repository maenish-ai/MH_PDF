#include "DocumentManager.h"
#include "AppSettings.h"
#include "PdfDocument.h"

DocumentManager::DocumentManager(AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {
    newTab();
    if (m_settings) {
        connect(m_settings, &AppSettings::lowMemoryModeChanged,
                this, &DocumentManager::refreshMemoryPolicy);
    }
}

DocumentManager::~DocumentManager() = default;

PdfDocument *DocumentManager::createDocument() {
    auto *document = new PdfDocument(this);
    document->setProviderNamespace(QString::number(m_nextSession++));
    connectDocument(document);
    return document;
}

void DocumentManager::connectDocument(PdfDocument *document) {
    connect(document, &PdfDocument::filePathChanged, this, [this, document] {
        emit tabsChanged();
        const QString path = document->filePath();
        if (!path.isEmpty()) {
            if (m_settings)
                m_settings->addRecentFile(path);
            emit documentPathAvailable(path);
        }
    });
    connect(document, &PdfDocument::modifiedChanged, this, [this] {
        emit tabsChanged();
        emit modifiedStateChanged();
    });
    connect(document, &PdfDocument::pageCountChanged, this, &DocumentManager::tabsChanged);
}

QObject *DocumentManager::currentDocument() const {
    return currentPdfDocument();
}

PdfDocument *DocumentManager::currentPdfDocument() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_documents.size())
        return nullptr;
    return m_documents.at(m_currentIndex);
}

QVariantList DocumentManager::tabs() const {
    QVariantList result;
    for (int i = 0; i < m_documents.size(); ++i) {
        const PdfDocument *document = m_documents.at(i);
        QVariantMap item;
        item.insert(QStringLiteral("index"), i);
        item.insert(QStringLiteral("title"), document->title());
        item.insert(QStringLiteral("path"), document->filePath());
        item.insert(QStringLiteral("modified"), document->modified());
        item.insert(QStringLiteral("pages"), document->pageCount());
        result.push_back(item);
    }
    return result;
}

bool DocumentManager::hasModifiedDocuments() const {
    for (const PdfDocument *document : m_documents) {
        if (document->modified())
            return true;
    }
    return false;
}

int DocumentManager::newTab() {
    auto *document = createDocument();
    m_documents.push_back(document);
    m_currentIndex = m_documents.size() - 1;
    emit tabsChanged();
    emit currentIndexChanged();
    emit currentDocumentChanged();
    emit modifiedStateChanged();
    return m_currentIndex;
}

bool DocumentManager::openDocument(const QString &path) {
    PdfDocument *target = currentPdfDocument();
    const bool reuse = target && target->filePath().isEmpty() && !target->modified()
                       && target->pageCount() == 1;
    if (!reuse) {
        newTab();
        target = currentPdfDocument();
    }
    const bool ok = target && target->openDocument(path);
    emit tabsChanged();
    return ok;
}

bool DocumentManager::openDocumentWithPassword(const QString &path, const QString &password) {
    PdfDocument *target = currentPdfDocument();
    if (!target)
        return false;
    const bool ok = target->openDocumentWithPassword(path, password);
    emit tabsChanged();
    return ok;
}

bool DocumentManager::closeTab(int index) {
    if (index < 0 || index >= m_documents.size())
        return false;
    if (m_documents.at(index)->modified())
        return false;

    if (m_documents.size() == 1) {
        m_documents.first()->newDocument();
        m_currentIndex = 0;
        emit tabsChanged();
        emit currentDocumentChanged();
        emit modifiedStateChanged();
        return true;
    }

    PdfDocument *victim = m_documents.takeAt(index);
    victim->deleteLater();
    if (m_currentIndex >= m_documents.size())
        m_currentIndex = m_documents.size() - 1;
    else if (index < m_currentIndex)
        --m_currentIndex;
    emit tabsChanged();
    emit currentIndexChanged();
    emit currentDocumentChanged();
    emit modifiedStateChanged();
    return true;
}

bool DocumentManager::closeOtherTabs(int keepIndex) {
    if (keepIndex < 0 || keepIndex >= m_documents.size())
        return false;
    for (int i = 0; i < m_documents.size(); ++i) {
        if (i != keepIndex && m_documents.at(i)->modified())
            return false;
    }
    PdfDocument *keep = m_documents.at(keepIndex);
    for (int i = m_documents.size() - 1; i >= 0; --i) {
        if (i == keepIndex)
            continue;
        PdfDocument *victim = m_documents.takeAt(i);
        victim->deleteLater();
    }
    m_currentIndex = m_documents.indexOf(keep);
    emit tabsChanged();
    emit currentIndexChanged();
    emit currentDocumentChanged();
    emit modifiedStateChanged();
    return true;
}

void DocumentManager::setCurrentIndex(int index) {
    if (index < 0 || index >= m_documents.size() || index == m_currentIndex)
        return;
    m_currentIndex = index;
    emit currentIndexChanged();
    emit currentDocumentChanged();
}

void DocumentManager::refreshMemoryPolicy() {
    for (PdfDocument *document : m_documents)
        document->refreshMemoryPolicy();
}

PdfDocument *DocumentManager::documentForSession(const QString &sessionId) const {
    for (PdfDocument *document : m_documents) {
        if (document->providerNamespace() == sessionId)
            return document;
    }
    return nullptr;
}

QImage DocumentManager::renderPage(const QString &sessionId, int page, const QSize &requestedSize) const {
    PdfDocument *document = documentForSession(sessionId);
    if (!document || !document->pageModel())
        return {};
    return document->pageModel()->renderPage(page, requestedSize);
}
