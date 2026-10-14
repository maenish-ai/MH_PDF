#include "UpdateManager.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QVersionNumber>

namespace {
const QUrl releasesApi(QStringLiteral("https://api.github.com/repos/maenish-ai/MH_PDF/releases/latest"));
const QUrl releasesPage(QStringLiteral("https://github.com/maenish-ai/MH_PDF/releases/latest"));
bool trusted(const QUrl &u) {
 return u.isValid() && u.scheme()==QStringLiteral("https") && u.userInfo().isEmpty()
   && (u.host().compare(QStringLiteral("github.com"),Qt::CaseInsensitive)==0
       || u.host().compare(QStringLiteral("api.github.com"),Qt::CaseInsensitive)==0
       || u.host().compare(QStringLiteral("objects.githubusercontent.com"),Qt::CaseInsensitive)==0
       || u.host().compare(QStringLiteral("release-assets.githubusercontent.com"),Qt::CaseInsensitive)==0);
}
}
UpdateManager::UpdateManager(QObject *parent):QObject(parent) {
 // Notifications are remembered per installed-version and target-version.
 // A downloaded package is NOT an installed update.
 QTimer::singleShot(12000,this,&UpdateManager::check);
}
void UpdateManager::fail(const QString &message) {
 if(m_file.isOpen())m_file.close();
 if(!m_targetPath.isEmpty())QFile::remove(m_targetPath);
 m_targetPath.clear();m_busy=false;m_downloading=false;m_status=message;
 emit changed();emit failed(message);
}
void UpdateManager::check(){
 if(m_busy)return;
 m_busy=true;m_downloading=false;m_status=QStringLiteral("Checking for updates");emit changed();
 fetch(releasesApi);
}
void UpdateManager::fetch(const QUrl &url,int redirects){
 if(!trusted(url)||redirects>8){fail(QStringLiteral("Untrusted update URL or redirect limit exceeded"));return;}
 QNetworkRequest request(url);
 request.setTransferTimeout(30000);
 request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
 request.setRawHeader("User-Agent","MaenPDF-Updater/1.0");
 request.setRawHeader("Accept","application/vnd.github+json");
 QNetworkReply *reply=m_net.get(request);m_reply=reply;
 connect(reply,&QNetworkReply::downloadProgress,this,[this,reply](qint64 received,qint64 total){
  if(reply!=m_reply||!m_downloading)return;
  if(total>MaxSize||received>MaxSize){reply->abort();return;}
  if(total>0){m_progress=int(qMin<qint64>(99,received*100/total));emit changed();}
 });
 connect(reply,&QIODevice::readyRead,this,[this,reply](){
  if(reply!=m_reply||!m_downloading)return;
  if(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()){reply->readAll();return;}
  const QByteArray data=reply->readAll();
  m_bytes+=data.size();
  if(m_bytes>MaxSize||!m_file.isOpen()||m_file.write(data)!=data.size()){reply->abort();return;}
  m_hash.addData(data);
 });
 connect(reply,&QNetworkReply::finished,this,[this,reply,url,redirects](){
  if(reply!=m_reply){reply->deleteLater();return;}
  m_reply=nullptr;
  const QVariant redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
  const auto networkError=reply->error();
  const int http=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QString errorString=reply->errorString();
  const QByteArray body=m_downloading?QByteArray():reply->readAll();
  reply->deleteLater();
  if(networkError!=QNetworkReply::NoError){fail(errorString);return;}
  if(redirect.isValid()){
   if(m_downloading&&m_file.isOpen()){m_file.resize(0);m_file.seek(0);m_bytes=0;m_hash.reset();}
   fetch(url.resolved(redirect.toUrl()),redirects+1);return;
  }
  if(http<200||http>=300){fail(QStringLiteral("GitHub HTTP %1").arg(http));return;}
  if(m_downloading){
   m_file.close();
   const QString actual=QString::fromLatin1(m_hash.result().toHex());
   if(actual.compare(m_expectedHash,Qt::CaseInsensitive)!=0){fail(QStringLiteral("Update SHA-256 mismatch"));return;}
   m_localPath=m_targetPath;m_targetPath.clear();m_busy=false;m_downloading=false;m_progress=100;
   m_status=QStringLiteral("Update downloaded and verified");emit changed();return;
  }
  if(body.size()>1024*1024){fail(QStringLiteral("Release metadata too large"));return;}
  QJsonParseError parseError;
  const QJsonDocument doc=QJsonDocument::fromJson(body,&parseError);
  if(parseError.error!=QJsonParseError::NoError||!doc.isObject()){fail(QStringLiteral("Invalid release metadata"));return;}
  const QJsonObject obj=doc.object();
  const QString tag=obj.value(QStringLiteral("tag_name")).toString();
  const QString next=tag.startsWith(QLatin1Char('v'))?tag.mid(1):tag;
  const QVersionNumber current=QVersionNumber::fromString(QCoreApplication::applicationVersion());
  const QVersionNumber newer=QVersionNumber::fromString(next);
  m_available=false;m_assetUrl=QUrl();m_expectedHash.clear();m_localPath.clear();
  if(!newer.isNull()&&QVersionNumber::compare(newer,current)>0){
   const QJsonArray assets=obj.value(QStringLiteral("assets")).toArray();
   for(const QJsonValue &item:assets){
    const QJsonObject a=item.toObject();
    const QString name=a.value(QStringLiteral("name")).toString();
#ifdef Q_OS_ANDROID
    const bool compatible=name.endsWith(QStringLiteral(".apk"),Qt::CaseInsensitive)
      && (name.contains(QStringLiteral("universal"),Qt::CaseInsensitive)
       || name.contains(QStringLiteral("arm64"),Qt::CaseInsensitive));
#elif defined(Q_OS_WIN)
    const bool compatible=name.endsWith(QStringLiteral(".exe"),Qt::CaseInsensitive)
      || name.endsWith(QStringLiteral(".msi"),Qt::CaseInsensitive);
#else
    const bool compatible=false;
#endif
    const QString digest=a.value(QStringLiteral("digest")).toString();
    const QUrl asset(a.value(QStringLiteral("browser_download_url")).toString());
    if(compatible&&trusted(asset)&&digest.startsWith(QStringLiteral("sha256:"))
       && QRegularExpression(QStringLiteral("^[0-9a-fA-F]{64}$")).match(digest.mid(7)).hasMatch()){
      m_assetUrl=asset;m_expectedHash=digest.mid(7);m_version=next;m_available=true;break;
    }
   }
  }
  m_releaseUrl=QUrl(obj.value(QStringLiteral("html_url")).toString());
  if(!trusted(m_releaseUrl))m_releaseUrl=releasesPage;
  m_busy=false;m_status=m_available?QStringLiteral("Update available"):QStringLiteral("Up to date");
  emit changed();
  if(m_available){
   // Only auto-prompt once for each (installed, available) version pair.
   // Manual Help > Check for Updates continues to show the latest status.
   const QString installed=QCoreApplication::applicationVersion();
   const QString promptKey=installed+QStringLiteral(" -> ")+m_version;
   QSettings settings;
   if(settings.value(QStringLiteral("updater/lastAutoPromptPair")).toString()!=promptKey){
    settings.setValue(QStringLiteral("updater/lastAutoPromptPair"),promptKey);
    settings.sync();
    emit updateFound();
   }
  }
 });
}
void UpdateManager::download(){
 if(m_busy||!m_available||!trusted(m_assetUrl))return;
 const QString dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
 if(dir.isEmpty()){fail(QStringLiteral("No update storage available"));return;}
 if(!QDir().mkpath(dir+QStringLiteral("/updates"))){fail(QStringLiteral("Cannot create update directory"));return;}
 m_targetPath=dir+QStringLiteral("/updates/")+QFileInfo(m_assetUrl.path()).fileName()+QStringLiteral(".part");
 m_file.setFileName(m_targetPath);
 if(!m_file.open(QIODevice::WriteOnly|QIODevice::Truncate)){fail(QStringLiteral("Cannot write update file"));return;}
 m_hash.reset();m_bytes=0;m_progress=0;m_downloading=true;m_busy=true;
 m_status=QStringLiteral("Downloading update");emit changed();fetch(m_assetUrl);
}
void UpdateManager::openReleasePage(){QDesktopServices::openUrl(m_releaseUrl.isValid()?m_releaseUrl:releasesPage);}
void UpdateManager::install(){
 if(m_localPath.isEmpty()||!QFileInfo::exists(m_localPath))return;
 // Installation is deliberately handed to the OS. Android requires user approval;
 // Windows installers may request elevation. Never replace a running executable.
 const QString ext=QFileInfo(m_assetUrl.path()).suffix().toLower();
 const QString dest=m_localPath.endsWith(QStringLiteral(".part"))?m_localPath.left(m_localPath.size()-5):m_localPath;
 if(!QFile::exists(dest))QFile::rename(m_localPath,dest);
 if(ext==QStringLiteral("apk")){
  openReleasePage(); // Android package installer needs a FileProvider / Play distribution.
 }else{
  QDesktopServices::openUrl(QUrl::fromLocalFile(dest));
 }
}
