#include "AppLogger.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QtGlobal>
static QMutex gLogMutex;
static QtMessageHandler gPrevious=nullptr;
QString AppLogger::logFilePath(){QString d=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);QDir().mkpath(d+"/logs");return d+"/logs/maenpdf.log";}
void AppLogger::write(const QString& level,const QString& message){QMutexLocker lock(&gLogMutex);QFile f(logFilePath());if(f.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)){QTextStream s(&f);s<<QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)<<" ["<<level<<"] "<<message.left(4000)<<'\n';}}
static void handler(QtMsgType type,const QMessageLogContext& ctx,const QString& msg){Q_UNUSED(ctx);QString l="INFO";if(type==QtWarningMsg)l="WARN";else if(type==QtCriticalMsg)l="ERROR";else if(type==QtFatalMsg)l="FATAL";else if(type==QtDebugMsg)l="DEBUG";AppLogger::write(l,msg);if(gPrevious)gPrevious(type,ctx,msg);}
void AppLogger::install(){gPrevious=qInstallMessageHandler(handler);write("INFO","MaenPDF logger initialized");}
