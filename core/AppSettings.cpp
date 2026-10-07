#include "AppSettings.h"
#include "MemoryPolicy.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

AppSettings::AppSettings(QObject *parent) : QObject(parent) {
    migrateSettings();

    QSettings settings;
    m_darkMode = settings.value(QStringLiteral("ui/darkMode"), false).toBool();
    m_adaptivePerformance = settings.value(QStringLiteral("performance/adaptive"), true).toBool();
    m_lowMemoryMode = m_adaptivePerformance
        ? MemoryPolicy::totalSystemMemoryMB() <= 6144
        : settings.value(QStringLiteral("performance/lowMemory"), false).toBool();
    m_safeGraphics = settings.value(QStringLiteral("performance/safeGraphics"), false).toBool();
    m_singleKeyShortcuts = settings.value(QStringLiteral("input/singleKeyShortcuts"), true).toBool();
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

    // Versioned preferences contract. Volatile window/session/cache state is
    // discarded across incompatible releases; durable user choices survive.
    settings.remove(QStringLiteral("window"));
    settings.remove(QStringLiteral("session"));
    settings.remove(QStringLiteral("cache"));
    settings.remove(QStringLiteral("print"));
    if (previous < 4) {
        settings.setValue(QStringLiteral("performance/adaptive"), true);
        settings.setValue(QStringLiteral("input/singleKeyShortcuts"), true);
    }
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

QString AppSettings::performanceProfile() const {
    if (!m_adaptivePerformance)
        return m_lowMemoryMode ? QStringLiteral("eco") : QStringLiteral("balanced");
    const qint64 mb = MemoryPolicy::totalSystemMemoryMB();
    if (mb <= 6144) return QStringLiteral("eco");
    if (mb <= 12288) return QStringLiteral("balanced");
    return QStringLiteral("performance");
}

qint64 AppSettings::totalMemoryMB() const {
    return MemoryPolicy::totalSystemMemoryMB();
}

void AppSettings::setDarkMode(bool value) {
    if (m_darkMode == value)
        return;
    m_darkMode = value;
    QSettings().setValue(QStringLiteral("ui/darkMode"), value);
    emit darkModeChanged();
}

void AppSettings::refreshAdaptiveMemoryChoice() {
    if (!m_adaptivePerformance)
        return;
    const bool recommended = MemoryPolicy::totalSystemMemoryMB() <= 6144;
    if (m_lowMemoryMode != recommended) {
        m_lowMemoryMode = recommended;
        emit lowMemoryModeChanged();
    }
}

void AppSettings::setLowMemoryMode(bool value) {
    const bool adaptiveWasEnabled = m_adaptivePerformance;
    m_adaptivePerformance = false; // a manual choice becomes an explicit override
    if (adaptiveWasEnabled) {
        QSettings().setValue(QStringLiteral("performance/adaptive"), false);
        emit adaptivePerformanceChanged();
    }
    if (m_lowMemoryMode == value) {
        emit performanceProfileChanged();
        return;
    }
    m_lowMemoryMode = value;
    QSettings().setValue(QStringLiteral("performance/lowMemory"), value);
    emit lowMemoryModeChanged();
    emit performanceProfileChanged();
}

void AppSettings::setAdaptivePerformance(bool value) {
    if (m_adaptivePerformance == value)
        return;
    m_adaptivePerformance = value;
    QSettings().setValue(QStringLiteral("performance/adaptive"), value);
    refreshAdaptiveMemoryChoice();
    emit adaptivePerformanceChanged();
    emit performanceProfileChanged();
}

void AppSettings::setSafeGraphics(bool value) {
    if (m_safeGraphics == value)
        return;
    m_safeGraphics = value;
    QSettings().setValue(QStringLiteral("performance/safeGraphics"), value);
    emit safeGraphicsChanged();
}

void AppSettings::setSingleKeyShortcuts(bool value) {
    if (m_singleKeyShortcuts == value)
        return;
    m_singleKeyShortcuts = value;
    QSettings().setValue(QStringLiteral("input/singleKeyShortcuts"), value);
    emit singleKeyShortcutsChanged();
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
    settings.setValue(QStringLiteral("performance/adaptive"), true);
    settings.setValue(QStringLiteral("input/singleKeyShortcuts"), true);
    if (keepLanguage)
        settings.setValue(QStringLiteral("ui/language"), language == QStringLiteral("ar") ? QStringLiteral("ar") : QStringLiteral("en"));
    if (!support.trimmed().isEmpty())
        settings.setValue(QStringLiteral("community/supportUrl"), support.trimmed());
    settings.sync();

    m_darkMode = false;
    m_adaptivePerformance = true;
    m_lowMemoryMode = MemoryPolicy::totalSystemMemoryMB() <= 6144;
    m_safeGraphics = false;
    m_singleKeyShortcuts = true;
    m_recentFiles.clear();
    m_supportUrl = support.trimmed();
    emit darkModeChanged();
    emit adaptivePerformanceChanged();
    emit lowMemoryModeChanged();
    emit performanceProfileChanged();
    emit safeGraphicsChanged();
    emit singleKeyShortcutsChanged();
    emit recentFilesChanged();
    emit supportUrlChanged();
    emit settingsReset();
}
