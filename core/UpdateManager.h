#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrl>
#include <QCryptographicHash>
#include <QFile>
class QNetworkReply;
class UpdateManager final : public QObject {
 Q_OBJECT
 Q_PROPERTY(bool busy READ busy NOTIFY changed)
 Q_PROPERTY(bool available READ available NOTIFY changed)
 Q_PROPERTY(QString version READ version NOTIFY changed)
 Q_PROPERTY(QString status READ status NOTIFY changed)
 Q_PROPERTY(int progress READ progress NOTIFY changed)
 Q_PROPERTY(bool ready READ ready NOTIFY changed)
public:
 explicit UpdateManager(QObject *parent=nullptr);
 bool busy() const {return m_busy;}
 bool available() const {return m_available;}
 QString version() const {return m_version;}
 QString status() const {return m_status;}
 int progress() const {return m_progress;}
 bool ready() const {return !m_localPath.isEmpty();}
 Q_INVOKABLE void check();
 Q_INVOKABLE void download();
 Q_INVOKABLE void install();
 Q_INVOKABLE void openReleasePage();
signals:
 void changed();
 void updateFound();
 void failed(const QString &message);
private:
 void fail(const QString &message);
 void fetch(const QUrl &url,int redirects=0);
 QNetworkAccessManager m_net;
 QPointer<QNetworkReply> m_reply;
 QFile m_file;
 QCryptographicHash m_hash{QCryptographicHash::Sha256};
 QUrl m_assetUrl,m_releaseUrl;
 QString m_expectedHash,m_version,m_status,m_localPath,m_targetPath;
 bool m_busy=false,m_available=false,m_downloading=false;
 int m_progress=0;
 qint64 m_bytes=0;
 static constexpr qint64 MaxSize=350LL*1024*1024;
};
