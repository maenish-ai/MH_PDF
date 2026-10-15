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
    QString dir=QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
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
    req.setRawHeader("User-Agent","MaenPDF-WebPdfImporter/1.0");
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
        if(total>0){const qint64 percent = n * 100 / total; m_progress = int(percent < 0 ? 0 : (percent > 99 ? 99 : percent));emit progressChanged();}
    });
    connect(reply,&QIODevice::readyRead,this,[this,reply](){
        if(reply!=m_reply)return;
        // Redirect responses are not PDF content.
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
                // A network chunk may contain fewer than five bytes.
                if(!m_pdfDetected) {
                    m_html.append(data);
                    if(m_html.size() < 5) return;
                    if(!m_html.startsWith("%PDF-")) {
                        // The URL is a webpage: collect it and offer only links that
                        // the page itself exposes as PDF resources. Do not attempt to
                        // bypass sign-in, viewer controls, DRM or site access rules.
                        m_inspect = true;
                        m_status = "Webpage detected. Finding directly exposed PDFs...";
                        emit statusChanged();
                        if(m_html.size() > MaxHtml) { reply->abort(); return; }
                        return;
                    }
                    data = m_html;
                    m_html.clear();
                    m_pdfDetected = true;
                }
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
            m_status="Direct PDF detected. Select it to download.";
        }else{
            parseHtml(m_currentUrl,m_html);
        }
        m_html.clear();
        emit statusChanged();
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

    auto addCandidate=[&](QString value,bool explicitlyPdf){
        value=value.trimmed();
        value.replace("&amp;","&");
        value.replace("&quot;","\"");
        value.replace("&#39;","'");
        if(value.isEmpty())return;
        const QUrl url=base.resolved(QUrl(value));
        if(!allowed(url))return;
        const bool looksPdf=url.path().endsWith(".pdf",Qt::CaseInsensitive)
            || url.query().contains(".pdf",Qt::CaseInsensitive);
        if(!explicitlyPdf && !looksPdf)return;
        const QString key=url.toString();
        if(seen.contains(key))return;
        seen.insert(key);
        QVariantMap entry;
        entry["url"]=key;
        QString title=QFileInfo(url.path()).fileName();
        if(title.isEmpty())title="PDF";
        entry["title"]=title;
        results.append(entry);
    };

    // Normal links/resources whose URL itself identifies a PDF.
    QRegularExpression rx(R"((?:href|src|data|data-url)\s*=\s*["']([^"']+)["'])",
                          QRegularExpression::CaseInsensitiveOption);
    auto it=rx.globalMatch(html);
    while(it.hasNext() && results.size()<100)
        addCandidate(it.next().captured(1),false);

    // Also accept standards-based embedded PDF resources even when the URL has
    // no .pdf suffix (for example /download?id=123 with type=application/pdf).
    QRegularExpression pdfTagRx(R"(<[^>]+\btype\s*=\s*["']application/pdf(?:\s*;[^"']*)?["'][^>]*>)",
                                QRegularExpression::CaseInsensitiveOption);
    QRegularExpression tagUrlRx(R"((?:href|src|data|data-url)\s*=\s*["']([^"']+)["'])",
                                QRegularExpression::CaseInsensitiveOption);
    auto tagIt=pdfTagRx.globalMatch(html);
    while(tagIt.hasNext() && results.size()<100){
        const QString tag=tagIt.next().captured(0);
        const auto urlMatch=tagUrlRx.match(tag);
        if(urlMatch.hasMatch())addCandidate(urlMatch.captured(1),true);
    }

    emit candidatesFound(results);
    if(results.isEmpty()){
        m_status="No direct PDF link was found on this page. The site may use a JavaScript/embedded viewer, require sign-in, or not expose a downloadable PDF. Use a direct PDF URL, or download the file in your browser and open it in MaenPDF.";
    }else{
        m_status=QString("Found %1 PDF link(s). Select one or download all.").arg(results.size());
    }
}
void WebPdfImporter::error(const QString &message){
    if(m_output){m_output->cancelWriting();delete m_output;m_output=nullptr;}
    m_html.clear();m_status=message;emit statusChanged();
    m_busy=false;emit busyChanged();emit failed(message);
}
void WebPdfImporter::cancel(){
    if(m_reply)m_reply->abort();
}

void WebPdfImporter::resetUi(){
    // Detach before aborting: finished/readyRead handlers reject stale replies.
    QNetworkReply *reply=m_reply;
    m_reply=nullptr;
    if(reply){reply->abort();reply->deleteLater();}
    if(m_output){m_output->cancelWriting();delete m_output;m_output=nullptr;}
    m_html.clear();m_savedPath.clear();m_currentUrl=QUrl();
    m_inspect=false;m_pdfDetected=false;m_redirects=0;
    m_busy=false;m_progress=0;m_status.clear();
    emit busyChanged();emit progressChanged();emit statusChanged();
}
