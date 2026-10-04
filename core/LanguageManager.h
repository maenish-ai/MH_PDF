#pragma once

#include <QObject>
#include <QHash>
#include <QString>

class LanguageManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool rtl READ rtl NOTIFY languageChanged)
    Q_PROPERTY(int revision READ revision NOTIFY languageChanged)
public:
    explicit LanguageManager(QObject *parent = nullptr);

    QString language() const { return m_language; }
    bool rtl() const { return m_language == QStringLiteral("ar"); }
    int revision() const { return m_revision; }

    Q_INVOKABLE QString t(const QString &key) const;
    Q_INVOKABLE QString number(qint64 value) const;
    Q_INVOKABLE QString decimal(double value, int precision = 0) const;
    Q_INVOKABLE void setLanguage(const QString &language);

signals:
    void languageChanged();

private:
    bool loadCatalog(const QString &language, QHash<QString, QString> &target);
    QString m_language{QStringLiteral("en")};
    int m_revision{0};
    QHash<QString, QString> m_english;
    QHash<QString, QString> m_arabic;
};
