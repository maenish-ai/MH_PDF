#include "LanguageManager.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSettings>

LanguageManager::LanguageManager(QObject *parent) : QObject(parent) {
    loadCatalog(QStringLiteral("en"), m_english);
    loadCatalog(QStringLiteral("ar"), m_arabic);

    QSettings settings;
    const QString stored = settings.value(QStringLiteral("ui/language"), QStringLiteral("en")).toString();
    m_language = stored == QStringLiteral("ar") ? QStringLiteral("ar") : QStringLiteral("en");
    QGuiApplication::setLayoutDirection(rtl() ? Qt::RightToLeft : Qt::LeftToRight);
}

bool LanguageManager::loadCatalog(const QString &language, QHash<QString, QString> &target) {
    QFile file(QStringLiteral(":/i18n/%1.json").arg(language));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return false;

    const QJsonObject object = document.object();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it)
        target.insert(it.key(), it.value().toString());
    return true;
}

QString LanguageManager::t(const QString &key) const {
    const auto &catalog = rtl() ? m_arabic : m_english;
    const auto it = catalog.constFind(key);
    if (it != catalog.constEnd())
        return it.value();

    const auto fallback = m_english.constFind(key);
    return fallback != m_english.constEnd() ? fallback.value() : key;
}

QString LanguageManager::number(qint64 value) const {
    QLocale locale(rtl() ? QLocale::Arabic : QLocale::English);
    return locale.toString(value);
}

QString LanguageManager::decimal(double value, int precision) const {
    QLocale locale(rtl() ? QLocale::Arabic : QLocale::English);
    return locale.toString(value, 'f', precision);
}

void LanguageManager::setLanguage(const QString &language) {
    const QString normalized = language == QStringLiteral("ar") ? QStringLiteral("ar") : QStringLiteral("en");
    if (normalized == m_language)
        return;

    m_language = normalized;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/language"), m_language);
    QGuiApplication::setLayoutDirection(rtl() ? Qt::RightToLeft : Qt::LeftToRight);
    ++m_revision;
    emit languageChanged();
}
