#include "MemoryPolicy.h"
#include <QProcessEnvironment>
qint64 MemoryPolicy::renderCacheBudgetBytes(){
    bool ok=false; const int mb=QProcessEnvironment::systemEnvironment().value("ORBIS_CACHE_MB").toInt(&ok);
    if(ok && mb>=32 && mb<=2048) return qint64(mb)*1024*1024;
#ifdef Q_OS_ANDROID
    return qint64(96)*1024*1024;
#else
    return qint64(256)*1024*1024;
#endif
}
int MemoryPolicy::maxRenderDimension(){
#ifdef Q_OS_ANDROID
    return 2200;
#else
    return 3400;
#endif
}
