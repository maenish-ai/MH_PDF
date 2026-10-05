#include "AppSettings.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

AppSettings::AppSettings(QObject *parent) : QObject(parent) {
    QSettings settings;
    m_darkMode = settings.value(QStringLiteral("ui/darkMode"), false).toBool();
    m_lowMemoryMode = settings.value(QStringLiteral("performance/lowMemory"), false).toBool();
    m_recentFiles = settings.value(QStringLiteral("recent/files")).toStringList();
    m_supportUrl = settings.value(QStringLiteral("community/supportUrl"),
                                  qEnvironmentVariable("MAENPDF_SUPPORT_URL")).toString().trimmed();
    while (m_recentFiles.size() > 12)
        m_recentFiles.removeLast();
}

QString AppSettings::normalizedPath(const QString &path) const {
    const QUrl url(path);
    if (url.isLocalFile())
        return QFileInfo(url.toLocalFile()).absoluteFilePath();
    return path;
}

QVariantList AppSettings::recentFiles() const {
    QVariantList result;
    for (const QString &stored : m_recentFiles) {
        const QUrl url(stored);
        const bool local = url.isLocalFile() || url.scheme().isEmpty();
        const QString localPath = url.isLocalFile() ? url.toLocalFile() : stored;
        const QFileInfo info(localPath);
        QVariantMap item;
        item.insert(QStringLiteral("path"), stored);
        item.insert(QStringLiteral("name"), local ? info.fileName() : url.fileName());
        item.insert(QStringLiteral("exists"), local ? info.exists() : true);
        result.push_back(item);
    }
    return result;
}

void AppSettings::setDarkMode(bool value) {
    if (m_darkMode == value)
        return;
    m_darkMode = value;
    QSettings().setValue(QStringLiteral("ui/darkMode"), value);
    emit darkModeChanged();
}

void AppSettings::setLowMemoryMode(bool value) {
    if (m_lowMemoryMode == value)
        return;
    m_lowMemoryMode = value;
    QSettings().setValue(QStringLiteral("performance/lowMemory"), value);
    emit lowMemoryModeChanged();
}

void AppSettings::persistRecentFiles() {
    QSettings().setValue(QStringLiteral("recent/files"), m_recentFiles);
    emit recentFilesChanged();
}

void AppSettings::addRecentFile(const QString &path) {
    const QString normalized = normalizedPath(path).trimmed();
    if (normalized.isEmpty())
        return;
    m_recentFiles.removeAll(normalized);
    m_recentFiles.prepend(normalized);
    while (m_recentFiles.size() > 12)
        m_recentFiles.removeLast();
    persistRecentFiles();
}

void AppSettings::clearRecentFiles() {
    if (m_recentFiles.isEmpty())
        return;
    m_recentFiles.clear();
    persistRecentFiles();
}

void AppSettings::setSupportUrl(const QString &url) {
    const QString clean = url.trimmed();
    if (clean == m_supportUrl)
        return;
    m_supportUrl = clean;
    QSettings().setValue(QStringLiteral("community/supportUrl"), m_supportUrl);
    emit supportUrlChanged();
}

bool AppSettings::openSupportPage() const {
    if (!supportAvailable())
        return false;
    const QUrl url(m_supportUrl);
    return url.isValid() && (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"))
        && QDesktopServices::openUrl(url);
}

bool AppSettings::openProjectPage() const {
    return QDesktopServices::openUrl(QUrl(projectUrl()));
}
