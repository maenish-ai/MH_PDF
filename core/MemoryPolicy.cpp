#include "MemoryPolicy.h"

#include <QProcessEnvironment>
#include <QSettings>

qint64 MemoryPolicy::renderCacheBudgetBytes() {
    const auto env = QProcessEnvironment::systemEnvironment();
    bool ok = false;
    int mb = env.value(QStringLiteral("MAENPDF_CACHE_MB")).toInt(&ok);
    if (!ok)
        mb = env.value(QStringLiteral("ORBIS_CACHE_MB")).toInt(&ok); // legacy compatibility
    if (ok && mb >= 24 && mb <= 2048)
        return qint64(mb) * 1024 * 1024;

    const bool lowMemory = QSettings().value(QStringLiteral("performance/lowMemory"), false).toBool();
#ifdef Q_OS_ANDROID
    return qint64(lowMemory ? 48 : 96) * 1024 * 1024;
#else
    return qint64(lowMemory ? 64 : 192) * 1024 * 1024;
#endif
}

int MemoryPolicy::maxRenderDimension() {
    const bool lowMemory = QSettings().value(QStringLiteral("performance/lowMemory"), false).toBool();
#ifdef Q_OS_ANDROID
    return lowMemory ? 1800 : 2200;
#else
    return lowMemory ? 2200 : 3400;
#endif
}
