#include "MemoryPolicy.h"

#include <QProcessEnvironment>
#include <QSettings>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <unistd.h>
#endif

namespace {
qint64 detectPhysicalMemoryMB() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX state{};
    state.dwLength = sizeof(state);
    if (GlobalMemoryStatusEx(&state))
        return qint64(state.ullTotalPhys / (1024ull * 1024ull));
#elif defined(Q_OS_UNIX)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && pageSize > 0)
        return qint64((quint64(pages) * quint64(pageSize)) / (1024ull * 1024ull));
#endif
    return 8192; // conservative fallback: balanced profile
}

qint64 ramMB() {
    static const qint64 detected = qMax<qint64>(1024, detectPhysicalMemoryMB());
    return detected;
}
}

qint64 MemoryPolicy::totalSystemMemoryMB() {
    return ramMB();
}

bool MemoryPolicy::adaptivePerformanceEnabled() {
    return QSettings().value(QStringLiteral("performance/adaptive"), true).toBool();
}

bool MemoryPolicy::effectiveLowMemoryMode() {
    const QSettings settings;
    if (!settings.value(QStringLiteral("performance/adaptive"), true).toBool())
        return settings.value(QStringLiteral("performance/lowMemory"), false).toBool();
#ifdef Q_OS_ANDROID
    return ramMB() <= 6144;
#else
    return ramMB() <= 6144;
#endif
}

QString MemoryPolicy::performanceProfile() {
    if (!adaptivePerformanceEnabled())
        return effectiveLowMemoryMode() ? QStringLiteral("eco") : QStringLiteral("balanced");
    const qint64 mb = ramMB();
    if (mb <= 6144)
        return QStringLiteral("eco");
    if (mb <= 12288)
        return QStringLiteral("balanced");
    return QStringLiteral("performance");
}

qint64 MemoryPolicy::renderCacheBudgetBytes() {
    const auto env = QProcessEnvironment::systemEnvironment();
    bool ok = false;
    int mb = env.value(QStringLiteral("MAENPDF_CACHE_MB")).toInt(&ok);
    if (!ok)
        mb = env.value(QStringLiteral("ORBIS_CACHE_MB")).toInt(&ok); // legacy compatibility
    if (ok && mb >= 16 && mb <= 2048)
        return qint64(mb) * 1024 * 1024;

    const bool lowMemory = effectiveLowMemoryMode();
    const qint64 memory = ramMB();
#ifdef Q_OS_ANDROID
    if (lowMemory || memory <= 4096) return qint64(28) * 1024 * 1024;
    if (memory <= 8192) return qint64(48) * 1024 * 1024;
    return qint64(72) * 1024 * 1024;
#else
    if (lowMemory || memory <= 4096) return qint64(32) * 1024 * 1024;
    if (memory <= 8192) return qint64(64) * 1024 * 1024;
    if (memory <= 16384) return qint64(128) * 1024 * 1024;
    return qint64(192) * 1024 * 1024;
#endif
}

int MemoryPolicy::maxRenderDimension() {
    const bool lowMemory = effectiveLowMemoryMode();
    const qint64 memory = ramMB();
#ifdef Q_OS_ANDROID
    if (lowMemory || memory <= 4096) return 1400;
    if (memory <= 8192) return 1900;
    return 2300;
#else
    if (lowMemory || memory <= 4096) return 1600;
    if (memory <= 8192) return 2200;
    if (memory <= 16384) return 3000;
    return 3600;
#endif
}

int MemoryPolicy::overlayMaxDimension() {
    const bool lowMemory = effectiveLowMemoryMode();
    const qint64 memory = ramMB();
#ifdef Q_OS_ANDROID
    if (lowMemory || memory <= 4096) return 800;
    if (memory <= 8192) return 1000;
    return 1200;
#else
    if (lowMemory || memory <= 4096) return 850;
    if (memory <= 8192) return 1100;
    if (memory <= 16384) return 1400;
    return 1600;
#endif
}

int MemoryPolicy::undoLimit() {
    const bool lowMemory = effectiveLowMemoryMode();
    const qint64 memory = ramMB();
#ifdef Q_OS_ANDROID
    if (lowMemory || memory <= 4096) return 5;
    if (memory <= 8192) return 8;
    return 10;
#else
    if (lowMemory || memory <= 4096) return 6;
    if (memory <= 8192) return 10;
    if (memory <= 16384) return 16;
    return 20;
#endif
}
