#include "EngineCapabilities.h"
#include <QStandardPaths>

QVariantMap EngineCapabilities::current() {
    QVariantMap capabilities;
    capabilities[QStringLiteral("backend")] = QStringLiteral("Orbis Engine 6 / Qt PDF lazy reader backend");
    capabilities[QStringLiteral("openPdf")] = true;
    capabilities[QStringLiteral("createPdf")] = true;
    capabilities[QStringLiteral("pageOperations")] = true;
    capabilities[QStringLiteral("overlayEditing")] = true;
    capabilities[QStringLiteral("standardExport")] = true;
    capabilities[QStringLiteral("aes256Desktop")] = !QStandardPaths::findExecutable(QStringLiteral("qpdf")).isEmpty();
    capabilities[QStringLiteral("textSearch")] = true;
    capabilities[QStringLiteral("backendAbstraction")] = true;
    capabilities[QStringLiteral("memoryPolicy")] = true;
    capabilities[QStringLiteral("lazyRendering")] = true;
    capabilities[QStringLiteral("renderCache")] = true;
    capabilities[QStringLiteral("androidContentUris")] = true;
    capabilities[QStringLiteral("bilingualUi")] = true;

    // These remain deliberately false until a structural writer/provider and regression corpus exist.
    capabilities[QStringLiteral("structuralTextEditing")] = false;
    capabilities[QStringLiteral("ocr")] = false;
    capabilities[QStringLiteral("digitalSignatures")] = false;
    capabilities[QStringLiteral("acroForms")] = false;
    capabilities[QStringLiteral("trueRedaction")] = false;
    return capabilities;
}
