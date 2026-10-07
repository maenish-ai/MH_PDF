#pragma once
#include <QtGlobal>
#include <QString>

class MemoryPolicy {
public:
    static qint64 renderCacheBudgetBytes();
    static int maxRenderDimension();
    static int overlayMaxDimension();
    static int undoLimit();
    static qint64 totalSystemMemoryMB();
    static bool adaptivePerformanceEnabled();
    static bool effectiveLowMemoryMode();
    static QString performanceProfile();
};
