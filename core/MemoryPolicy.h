#pragma once
#include <QtGlobal>
class MemoryPolicy {
public:
    static qint64 renderCacheBudgetBytes();
    static int maxRenderDimension();
    static int overlayMaxDimension();
    static int undoLimit();
};
