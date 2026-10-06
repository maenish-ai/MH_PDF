#include "AppSettings.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

AppSettings::AppSettings(QObject *parent) : QObject(parent) {
    migrateSettings();

    QSettings settings;
    m_darkMode = settings.value(QStringLiteral("ui/darkMode"), false).toBool();
    m_lowMemoryMode = settings.value(QStringLiteral("performance/lowMemory"), false).toBool();
    m_safeGraphics = settings.value(QStringLiteral("performance/safeGraphics"), false).toBool();
    m_recentFiles = settings.value(QStringLiteral("recent/files")).toStringList();
    m_supportUrl = settings.value(QStringLiteral("community/supportUrl"),
                                  qEnvironmentVariable("MAENPDF_SUPPORT_URL")).toString().trimmed();
    while (m_recentFiles.size() > 12)
        m_recentFiles.removeLast();
}

void AppSettings::migrateSettings() {
    QSettings settings;
    const int previous = settings.value(QStringLiteral("meta/settingsSchemaVersion"), 0).toInt();
    if (previous >= CurrentSettingsSchema)
        return;

    // v7.1 establishes a small, versioned preferences contract. Old volatile
    // UI/session/cache keys are discarded so stale values cannot break a new
    // installation, while durable user choices remain compatible.
    settings.remove(QStringLiteral("window"));
    settings.remove(QStringLiteral("session"));
    settings.remove(QStringLiteral("cache"));
    settings.remove(QStringLiteral("print"));
    settings.setValue(QStringLiteral("meta/settingsSchemaVersion"), CurrentSettingsSchema);
    settings.sync();
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

void AppSettings::setSafeGraphics(bool value) {
    if (m_safeGraphics == value)
        return;
    m_safeGraphics = value;
    QSettings().setValue(QStringLiteral("performance/safeGraphics"), value);
    emit safeGraphicsChanged();
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

bool AppSettings::openReleasesPage() const {
    return QDesktopServices::openUrl(QUrl(releasesUrl()));
}

void AppSettings::resetApplicationSettings(bool keepLanguage) {
    QSettings settings;
    const QString language = settings.value(QStringLiteral("ui/language"), QStringLiteral("en")).toString();
    const QString support = settings.value(QStringLiteral("community/supportUrl"), m_supportUrl).toString();
    settings.clear();
    settings.setValue(QStringLiteral("meta/settingsSchemaVersion"), CurrentSettingsSchema);
    if (keepLanguage)
        settings.setValue(QStringLiteral("ui/language"), language == QStringLiteral("ar") ? QStringLiteral("ar") : QStringLiteral("en"));
    if (!support.trimmed().isEmpty())
        settings.setValue(QStringLiteral("community/supportUrl"), support.trimmed());
    settings.sync();

    m_darkMode = false;
    m_lowMemoryMode = false;
    m_safeGraphics = false;
    m_recentFiles.clear();
    m_supportUrl = support.trimmed();
    emit darkModeChanged();
    emit lowMemoryModeChanged();
    emit safeGraphicsChanged();
    emit recentFilesChanged();
    emit supportUrlChanged();
    emit settingsReset();
}
