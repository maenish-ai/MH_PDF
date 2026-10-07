#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>
#include <QStringList>

class AppSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(bool lowMemoryMode READ lowMemoryMode WRITE setLowMemoryMode NOTIFY lowMemoryModeChanged)
    Q_PROPERTY(bool adaptivePerformance READ adaptivePerformance WRITE setAdaptivePerformance NOTIFY adaptivePerformanceChanged)
    Q_PROPERTY(QString performanceProfile READ performanceProfile NOTIFY performanceProfileChanged)
    Q_PROPERTY(qint64 totalMemoryMB READ totalMemoryMB CONSTANT)
    Q_PROPERTY(bool safeGraphics READ safeGraphics WRITE setSafeGraphics NOTIFY safeGraphicsChanged)
    Q_PROPERTY(bool singleKeyShortcuts READ singleKeyShortcuts WRITE setSingleKeyShortcuts NOTIFY singleKeyShortcutsChanged)
    Q_PROPERTY(QVariantList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QString supportUrl READ supportUrl NOTIFY supportUrlChanged)
    Q_PROPERTY(bool supportAvailable READ supportAvailable NOTIFY supportUrlChanged)
    Q_PROPERTY(QString projectUrl READ projectUrl CONSTANT)
    Q_PROPERTY(QString releasesUrl READ releasesUrl CONSTANT)
    Q_PROPERTY(int settingsSchemaVersion READ settingsSchemaVersion CONSTANT)
public:
    static constexpr int CurrentSettingsSchema = 4;

    explicit AppSettings(QObject *parent = nullptr);

    bool darkMode() const { return m_darkMode; }
    bool lowMemoryMode() const { return m_lowMemoryMode; }
    bool adaptivePerformance() const { return m_adaptivePerformance; }
    QString performanceProfile() const;
    qint64 totalMemoryMB() const;
    bool safeGraphics() const { return m_safeGraphics; }
    bool singleKeyShortcuts() const { return m_singleKeyShortcuts; }
    QVariantList recentFiles() const;
    QString supportUrl() const { return m_supportUrl; }
    bool supportAvailable() const { return !m_supportUrl.trimmed().isEmpty(); }
    QString projectUrl() const { return QStringLiteral("https://github.com/maenish-ai/MH_PDF"); }
    QString releasesUrl() const { return QStringLiteral("https://github.com/maenish-ai/MH_PDF/releases"); }
    int settingsSchemaVersion() const { return CurrentSettingsSchema; }

    Q_INVOKABLE void setDarkMode(bool value);
    Q_INVOKABLE void setLowMemoryMode(bool value);
    Q_INVOKABLE void setAdaptivePerformance(bool value);
    Q_INVOKABLE void setSafeGraphics(bool value);
    Q_INVOKABLE void setSingleKeyShortcuts(bool value);
    Q_INVOKABLE void addRecentFile(const QString &path);
    Q_INVOKABLE void clearRecentFiles();
    Q_INVOKABLE void setSupportUrl(const QString &url);
    Q_INVOKABLE bool openSupportPage() const;
    Q_INVOKABLE bool openProjectPage() const;
    Q_INVOKABLE bool openReleasesPage() const;
    Q_INVOKABLE QString normalizedPath(const QString &path) const;
    Q_INVOKABLE void resetApplicationSettings(bool keepLanguage = true);

signals:
    void darkModeChanged();
    void lowMemoryModeChanged();
    void adaptivePerformanceChanged();
    void performanceProfileChanged();
    void safeGraphicsChanged();
    void singleKeyShortcutsChanged();
    void recentFilesChanged();
    void supportUrlChanged();
    void settingsReset();

private:
    void migrateSettings();
    void persistRecentFiles();
    void refreshAdaptiveMemoryChoice();
    bool m_darkMode{false};
    bool m_lowMemoryMode{false};
    bool m_adaptivePerformance{true};
    bool m_safeGraphics{false};
    bool m_singleKeyShortcuts{true};
    QStringList m_recentFiles;
    QString m_supportUrl;
};
