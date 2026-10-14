#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSaveFile>
#include <QUrl>
#include <QVariantList>
class QNetworkReply;

class WebPdfImporter final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
public:
    explicit WebPdfImporter(QObject *parent = nullptr);
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    int progress() const { return m_progress; }
    Q_INVOKABLE void inspect(const QString &url);
    Q_INVOKABLE void download(const QString &url);
    Q_INVOKABLE void cancel();
signals:
    void busyChanged();
    void statusChanged();
    void progressChanged();
    void candidatesFound(const QVariantList &items);
    void downloaded(const QString &path);
    void failed(const QString &message);
private:
    bool allowed(const QUrl &url) const;
    void start(const QUrl &url, bool inspect);
    void request(const QUrl &url);
    void finish(QNetworkReply *reply);
    void error(const QString &message);
    void parseHtml(const QUrl &base, const QByteArray &html);
    QString targetPath(const QUrl &url) const;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QSaveFile *m_output = nullptr;
    QByteArray m_html;
    QUrl m_currentUrl;
    QString m_savedPath;
    QString m_status;
    bool m_busy = false;
    bool m_inspect = false;
    bool m_pdfDetected = false;
    int m_progress = 0;
    int m_redirects = 0;
    static constexpr qint64 MaxHtml = 4 * 1024 * 1024;
    static constexpr qint64 MaxPdf = 300 * 1024 * 1024;
};
