#include "EngineCapabilities.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {
bool providerAvailable(const QString &systemName, const QString &relativePath) {
    if (!QStandardPaths::findExecutable(systemName).isEmpty())
        return true;
    QString bundled = QDir(QCoreApplication::applicationDirPath()).filePath(relativePath);
#ifdef Q_OS_WIN
    if (!bundled.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive))
        bundled += QStringLiteral(".exe");
#endif
    return QFileInfo::exists(bundled);
}
}

QVariantMap EngineCapabilities::current() {
    const bool qpdf = providerAvailable(QStringLiteral("qpdf"), QStringLiteral("tools/qpdf/qpdf"));
    const bool tesseract = providerAvailable(QStringLiteral("tesseract"), QStringLiteral("tools/tesseract/tesseract"));
    const bool libreOffice = providerAvailable(QStringLiteral("soffice"), QStringLiteral("tools/libreoffice/program/soffice"));

    QVariantMap capabilities;
    capabilities[QStringLiteral("backend")] = QStringLiteral("MaenPDF Engine 7 / Qt PDF lazy renderer + local tool providers");
    capabilities[QStringLiteral("openPdf")] = true;
    capabilities[QStringLiteral("passwordProtectedOpen")] = true;
    capabilities[QStringLiteral("createPdf")] = true;
    capabilities[QStringLiteral("pageOperations")] = true;
    capabilities[QStringLiteral("overlayEditing")] = true;
    capabilities[QStringLiteral("standardExport")] = true;
    capabilities[QStringLiteral("aes256Desktop")] = qpdf;
    capabilities[QStringLiteral("textSearch")] = true;
    capabilities[QStringLiteral("textSelection")] = true;
    capabilities[QStringLiteral("continuousPageViewer")] = true;
    capabilities[QStringLiteral("liveInkPreview")] = true;
    capabilities[QStringLiteral("stagedCropAndRedaction")] = true;
    capabilities[QStringLiteral("backendAbstraction")] = true;
    capabilities[QStringLiteral("memoryPolicy")] = true;
    capabilities[QStringLiteral("lowMemoryMode")] = true;
    capabilities[QStringLiteral("adaptivePerformance")] = true;
    capabilities[QStringLiteral("removableInsertedText")] = true;
    capabilities[QStringLiteral("movableResizableInsertedText")] = true;
    capabilities[QStringLiteral("partialTextStrikeout")] = true;
    capabilities[QStringLiteral("localFormFields")] = true;
    capabilities[QStringLiteral("pdfFullScreenMode")] = true;
    capabilities[QStringLiteral("keyboardShortcutProfile")] = QStringLiteral("Acrobat-familiar");
    capabilities[QStringLiteral("lazyRendering")] = true;
    capabilities[QStringLiteral("renderCache")] = true;
    capabilities[QStringLiteral("androidContentUris")] = true;
    capabilities[QStringLiteral("bilingualUi")] = true;
    capabilities[QStringLiteral("multiDocumentTabs")] = true;
    capabilities[QStringLiteral("recentFiles")] = true;
    capabilities[QStringLiteral("pdfCompare")] = true;
    capabilities[QStringLiteral("exportImages")] = true;
    capabilities[QStringLiteral("safeFlatten")] = true;
    capabilities[QStringLiteral("secureFlattenedRedaction")] = true;
    capabilities[QStringLiteral("batesNumbering")] = true;
    capabilities[QStringLiteral("cropPage")] = true;
    capabilities[QStringLiteral("qpdfTools")] = qpdf;
    capabilities[QStringLiteral("ocrSearchablePdf")] = qpdf && tesseract;
    capabilities[QStringLiteral("officeBridge")] = libreOffice;
    capabilities[QStringLiteral("localOnlyProcessing")] = true;
    capabilities[QStringLiteral("cloudUpload")] = false;

    // Deliberately false until a verified structural writer/provider is integrated.
    capabilities[QStringLiteral("structuralTextEditing")] = false;
    capabilities[QStringLiteral("digitalSignatures")] = false;
    capabilities[QStringLiteral("acroFormAuthoring")] = false;
    capabilities[QStringLiteral("trueObjectRedaction")] = false;
    return capabilities;
}
