#include "WebPdfImporter.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUrlQuery>

WebPdfImporter::WebPdfImporter(QObject *parent): QObject(parent) {}
bool WebPdfImporter::allowed(const QUrl &url) const {
    if (!url.isValid() || (url.scheme()!="http" && url.scheme()!="https")
        || url.host().isEmpty() || !url.userInfo().isEmpty()) return false;
    const QString host=url.host().toLower();
    if(host=="localhost" || host.endsWith(".localhost") ||
       host.endsWith(".local") || host.endsWith(".internal")) return false;
    QHostAddress ip;
    if(ip.setAddress(host)) {
        if(ip.isLoopback() || ip.isLinkLocal() || ip.isMulticast()
           || ip.isInSubnet(QHostAddress("10.0.0.0"),8)
           || ip.isInSubnet(QHostAddress("172.16.0.0"),12)
           || ip.isInSubnet(QHostAddress("192.168.0.0"),16)
           || ip.isInSubnet(QHostAddress("100.64.0.0"),10)
           || ip.isInSubnet(QHostAddress("0.0.0.0"),8)
           || ip.isInSubnet(QHostAddress("fc00::"),7)
           || ip.isInSubnet(QHostAddress("fe80::"),10)) return false;
    }
    return true;
}
void WebPdfImporter::inspect(const QString &s){start(QUrl::fromUserInput(s),true);}
void WebPdfImporter::download(const QString &s){start(QUrl::fromUserInput(s),false);}
void WebPdfImporter::start(const QUrl &url,bool inspect) {
    if(m_busy) return;
    if(!allowed(url)){emit failed("Invalid or restricted URL");return;}
    m_inspect=inspect;m_redirects=0;m_pdfDetected=false;m_html.clear();
    m_progress=0;emit progressChanged();
    m_busy=true;emit busyChanged();
    request(url);
}
QString WebPdfImporter::targetPath(const QUrl &url) const {
#ifdef Q_OS_ANDROID
    // Scoped storage: app-specific storage is writable without broad storage permissions.
    QString dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString dir=QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#endif
    if(dir.isEmpty()) dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QString name=QFileInfo(url.path()).fileName();
    if(!name.endsWith(".pdf",Qt::CaseInsensitive)) name="download.pdf";
    name.replace(QRegularExpression("[^A-Za-z0-9._-]"),"_");
    QString stem=QFileInfo(name).completeBaseName();
    if(stem.isEmpty()) stem="download";
    QString path=QDir(dir).filePath(name);
    for(int i=1;QFile::exists(path);++i)
        path=QDir(dir).filePath(stem+"_"+QString::number(i)+".pdf");
    return path;
}
void WebPdfImporter::request(const QUrl &url){
    if(!allowed(url)){error("Redirect to restricted URL");return;}
    m_currentUrl=url;
    m_status=m_inspect?"Inspecting page...":"Downloading PDF...";
    emit statusChanged();
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(30000);
    req.setRawHeader("Accept","application/pdf,text/html;q=0.8");
    QNetworkReply *reply=m_network.get(req);
    m_reply=reply;
    connect(reply,&QNetworkReply::metaDataChanged,this,[this,reply](){
        if(reply!=m_reply) return;
        const qint64 length=reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if(length>(m_inspect?MaxHtml:MaxPdf)) reply->abort();
    });
    connect(reply,&QNetworkReply::downloadProgress,this,[this,reply](qint64 n,qint64 total){
        if(reply!=m_reply)return;
        if(n>(m_inspect?MaxHtml:MaxPdf)){reply->abort();return;}
        if(total>0){m_progress=int(qBound<qint64>(qint64(0), n*100/total, qint64(99)));emit progressChanged();}
    });
    connect(reply,&QIODevice::readyRead,this,[this,reply](){
        if(reply!=m_reply)return;
        // Redirect responses are handled only after completion.
        if(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
            reply->readAll();
            return;
        }
        QByteArray data=reply->readAll();
        if(m_inspect){
            if(m_html.size()+data.size()>MaxHtml){reply->abort();return;}
            m_html.append(data);
        } else {
            if(!m_output){
                if(data.size()<5 && !data.isEmpty()) { m_html.append(data); return; }
                if(!m_html.isEmpty()) { data.prepend(m_html); m_html.clear(); }
                m_pdfDetected=data.startsWith("%PDF-");
                if(!m_pdfDetected){reply->abort();return;}
                m_savedPath=targetPath(m_currentUrl);
                m_output=new QSaveFile(m_savedPath);
                if(!m_output->open(QIODevice::WriteOnly)){reply->abort();return;}
            }
            if(m_output->size()+data.size()>MaxPdf || m_output->write(data)!=data.size())
                reply->abort();
        }
    });
    connect(reply,&QNetworkReply::finished,this,[this,reply](){finish(reply);});
}
void WebPdfImporter::finish(QNetworkReply *reply){
    if(reply!=m_reply){reply->deleteLater();return;}
    m_reply=nullptr;
    const QVariant redir=reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(redir.isValid() && reply->error()==QNetworkReply::NoError){
        const QUrl next=m_currentUrl.resolved(redir.toUrl());
        reply->deleteLater();
        if(m_output){m_output->cancelWriting();delete m_output;m_output=nullptr;}
        m_html.clear();
        if(++m_redirects>8){error("Too many redirects");return;}
        request(next);
        return;
    }
    const auto networkError=reply->error();
    const QString message=reply->errorString();
    reply->deleteLater();
    if(networkError!=QNetworkReply::NoError){error(message);return;}
    if(m_inspect){
        if(m_html.startsWith("%PDF-")){
            QVariantMap entry;entry["url"]=m_currentUrl.toString();entry["title"]="PDF";
            emit candidatesFound(QVariantList{entry});
        }else parseHtml(m_currentUrl,m_html);
        m_html.clear();
        m_status="Inspection complete";emit statusChanged();
    } else {
        if(!m_output){error("No valid PDF data received");return;}
        if(!m_output->commit()){error("Unable to save PDF");return;}
        delete m_output;m_output=nullptr;
        m_progress=100;emit progressChanged();
        m_status="Saved: "+m_savedPath;emit statusChanged();
        emit downloaded(m_savedPath);
    }
    m_busy=false;emit busyChanged();
}
void WebPdfImporter::parseHtml(const QUrl &base,const QByteArray &bytes){
    QVariantList results;QSet<QString> seen;
    const QString html=QString::fromUtf8(bytes);
    QRegularExpression rx(R"((?:href|src|data|data-url)\s*=\s*["']([^"']+)["'])",
                          QRegularExpression::CaseInsensitiveOption);
    auto it=rx.globalMatch(html);
    while(it.hasNext() && results.size()<100){
        QString value=it.next().captured(1);
        value.replace("&amp;","&");
        const QUrl url=base.resolved(QUrl(value));
        if(!allowed(url))continue;
        if(!url.path().endsWith(".pdf",Qt::CaseInsensitive)
           && !url.query().contains(".pdf",Qt::CaseInsensitive))continue;
        const QString key=url.toString();
        if(seen.contains(key))continue;
        seen.insert(key);
        QVariantMap entry;entry["url"]=key;
        entry["title"]=QFileInfo(url.path()).fileName();
        results.append(entry);
    }
    emit candidatesFound(results);
}
void WebPdfImporter::error(const QString &message){
    if(m_output){m_output->cancelWriting();delete m_output;m_output=nullptr;}
    m_html.clear();m_status=message;emit statusChanged();
    m_busy=false;emit busyChanged();emit failed(message);
}
void WebPdfImporter::cancel(){
    if(m_reply)m_reply->abort();
}
