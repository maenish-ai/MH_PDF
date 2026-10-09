#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QGuiApplication>
#ifndef Q_OS_ANDROID
#include <QApplication>
#endif
#include <QIcon>
#include <QImageReader>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QtGlobal>
#include <cstdlib>
#include <QtMath>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "../core/AppLogger.h"
#include "../core/AppSettings.h"
#include "../core/DocumentManager.h"
#include "../core/LanguageManager.h"
#include "../core/MemoryPolicy.h"
#include "../core/PdfToolsService.h"
#include "../core/PdfDocument.h"
#include "../core/PrintService.h"
#include "../core/SessionImageProvider.h"

namespace {
void showStartupFailure(const QString &details)
{
    AppLogger::write(QStringLiteral("FATAL"), details);
#ifdef Q_OS_WIN
    const QString message = QStringLiteral(
        "MaenPDF could not start.\n\n"
        "%1\n\n"
        "Diagnostic log:\n%2\n\n"
        "Please send the log file when requesting support.")
        .arg(details, QDir::toNativeSeparators(AppLogger::logFilePath()));
    MessageBoxW(nullptr,
                reinterpret_cast<LPCWSTR>(message.utf16()),
                L"MaenPDF Startup Error",
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#else
    qCritical().noquote() << details;
#endif
}
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    // Set application identity before settings/logger paths are resolved.
    // so every startup message is written to one deterministic path.
    QGuiApplication::setOrganizationName(QStringLiteral("MaenPDF"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("maenpdf.local"));
    QGuiApplication::setApplicationName(QStringLiteral("MaenPDF"));
    QGuiApplication::setApplicationVersion(QStringLiteral("7.5.0"));
    AppLogger::install();

    // Bound image decoder allocations before any user-controlled image is
    // decoded. The adaptive profile uses a stricter cap on 4–6 GB machines.
    const int imageAllocationLimitMB = MemoryPolicy::performanceProfile() == QStringLiteral("eco") ? 96 : 256;
    QImageReader::setAllocationLimit(imageAllocationLimitMB);
    AppLogger::write(QStringLiteral("INFO"),
                     QStringLiteral("Adaptive profile=%1; detectedRamMB=%2; imageAllocationLimitMB=%3")
                         .arg(MemoryPolicy::performanceProfile())
                         .arg(MemoryPolicy::totalSystemMemoryMB())
                         .arg(imageAllocationLimitMB));

#ifdef Q_OS_WIN
    // Use Qt's accelerated renderer by default. Software rendering is kept as a
    // deterministic safe mode for old/broken GPU drivers. A startup marker lets
    // the next launch recover automatically if the previous graphics startup
    // terminated before the first window was created.
    const QString graphicsMarker = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                                       .filePath(QStringLiteral("graphics-startup.pending"));
    QDir().mkpath(QFileInfo(graphicsMarker).dir().absolutePath());
    const bool commandSafe = QCoreApplication::arguments().contains(QStringLiteral("--safe-graphics"));
    const bool previousGraphicsStartupFailed = QFileInfo::exists(graphicsMarker);
    const bool safeGraphics = commandSafe || previousGraphicsStartupFailed
        || QSettings().value(QStringLiteral("performance/safeGraphics"), false).toBool();
    if (safeGraphics) {
        qputenv("QT_QUICK_BACKEND", QByteArrayLiteral("software"));
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    }
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QFile marker(graphicsMarker);
    if (marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        marker.write("pending\n");
        marker.close();
    }
#else
    const QString graphicsMarker;
#endif

    // Make startup independent of how the process was launched (desktop
    // shortcut, Start menu, Explorer, terminal, file association, etc.).
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    AppLogger::write(QStringLiteral("INFO"),
                     QStringLiteral("MaenPDF 7.5.0 startup; Qt %1; appDir=%2; cwd=%3; QT_QUICK_BACKEND=%4")
                         .arg(QString::fromLatin1(qVersion()),
                              QCoreApplication::applicationDirPath(),
                              QDir::currentPath(),
                              qEnvironmentVariable("QT_QUICK_BACKEND")));

    QGuiApplication::setWindowIcon(
        QIcon(QStringLiteral(":/qt/qml/PDFStudio/assets/maenpdf-logo.svg")));

    LanguageManager languageManager;
    AppSettings appSettings;
    DocumentManager documentManager(&appSettings);
    PdfToolsService pdfTools;
    PrintService printService;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("documentManager"), &documentManager);
    engine.rootContext()->setContextProperty(QStringLiteral("appSettings"), &appSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("pdfTools"), &pdfTools);
    engine.rootContext()->setContextProperty(QStringLiteral("printService"), &printService);
    engine.rootContext()->setContextProperty(QStringLiteral("i18n"), &languageManager);
    engine.addImageProvider(QStringLiteral("maenpdf"),
                            new SessionImageProvider(&documentManager));

    engine.loadFromModule(QStringLiteral("PDFStudio"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) {
        showStartupFailure(QStringLiteral(
            "The main interface could not be created. This is usually caused by "
            "a missing Qt runtime/plugin or a QML startup error."));
        return EXIT_FAILURE;
    }

    AppLogger::write(QStringLiteral("INFO"), QStringLiteral("Main window created successfully"));
#ifdef Q_OS_WIN
    QFile::remove(graphicsMarker);
#endif

    const QStringList launchArguments = QCoreApplication::arguments();
    if (launchArguments.size() > 1) {
        const QString firstArgument = launchArguments.at(1);
        if (firstArgument == QStringLiteral("--interaction-smoke")) {
            if (launchArguments.size() < 3 || !documentManager.openDocument(launchArguments.at(2))) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_OPEN_FAILED"));
                return EXIT_FAILURE;
            }
            if (!engine.rootObjects().isEmpty())
                engine.rootObjects().first()->setProperty("homeVisible", false);

            PdfDocument *document = documentManager.currentPdfDocument();
            if (!document || document->pageCount() < 3) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_PAGE_COUNT_FAILED"));
                return EXIT_FAILURE;
            }

            const QVariantMap selection = document->textSelection(0, 0.02, 0.02, 0.98, 0.98);
            const QVariantList selectionRects = selection.value(QStringLiteral("rects")).toList();
            if (!selection.value(QStringLiteral("valid")).toBool() || selectionRects.isEmpty()) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_TEXT_SELECTION_FAILED"));
                return EXIT_FAILURE;
            }

            document->addHighlightRects(0, selectionRects, QStringLiteral("#FFD54F"), 42);
            const QString insertedTextId = document->addText(0, 0.20, 0.30, QStringLiteral("interaction text"), 18);
            if (insertedTextId.isEmpty() || document->textAnnotations(0).isEmpty()
                || !document->deleteTextAnnotation(0, insertedTextId) || !document->textAnnotations(0).isEmpty()) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_TEXT_DELETE_FAILED"));
                return EXIT_FAILURE;
            }
            const QString editableTextId = document->addText(0, 0.18, 0.38, QStringLiteral("movable text"), 18);
            if (editableTextId.isEmpty()
                || !document->updateTextAnnotation(0, editableTextId, QStringLiteral("movable text"), 0.22, 0.40, 24, 0, 7)
                || !document->moveTextAnnotation(0, editableTextId, 0.28, 0.44)
                || !document->resizeTextAnnotation(0, editableTextId, 28)) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_TEXT_EDIT_FAILED"));
                return EXIT_FAILURE;
            }
            const QVariantMap editedText = document->textAnnotation(0, editableTextId);
            if (editedText.value(QStringLiteral("fontSize")).toInt() != 28
                || editedText.value(QStringLiteral("strikeLength")).toInt() != 7) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_TEXT_STATE_FAILED"));
                return EXIT_FAILURE;
            }

            const QString formText = document->addFormField(0, QStringLiteral("text"), 0.10, 0.62, 0.28, 0.05);
            const QString formCheck = document->addFormField(0, QStringLiteral("checkbox"), 0.10, 0.70, 0.045, 0.045);
            const QString formRadio = document->addFormField(0, QStringLiteral("radio"), 0.18, 0.70, 0.045, 0.045);
            const QString formDrop = document->addFormField(0, QStringLiteral("dropdown"), 0.28, 0.70, 0.28, 0.05);
            if (formText.isEmpty() || formCheck.isEmpty() || formRadio.isEmpty() || formDrop.isEmpty()
                || !document->updateFormField(0, formText, QStringLiteral("John Doe"), false, 0)
                || !document->updateFormField(0, formCheck, QString(), true, 0)
                || !document->updateFormField(0, formRadio, QString(), true, 0)
                || !document->updateFormField(0, formDrop, QString(), false, 1)
                || document->formAnnotations(0).size() < 4) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_FORMS_FAILED"));
                return EXIT_FAILURE;
            }

            if (!document->canUndo()) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_UNDO_MISSING"));
                return EXIT_FAILURE;
            }
            document->undo();
            if (!document->canRedo()) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_REDO_MISSING"));
                return EXIT_FAILURE;
            }
            document->redo();

            document->addInkStyled(0, QVariantList{0.15, 0.20, 0.30, 0.24, 0.45, 0.21},
                                   QStringLiteral("#2563EB"), 0.004, 100);
            document->addHighlightInkStyled(0, QVariantList{0.18, 0.27, 0.32, 0.28, 0.48, 0.27},
                                            QStringLiteral("#81C784"), 0.018, 34);
            document->addRedaction(1, 0.12, 0.12, 0.22, 0.08);
            document->cropPage(2, 0.05, 0.05, 0.90, 0.90);
            document->setCurrentPage(2);

            // Stress the committed ink path with a realistic long stroke. The
            // live QML path is incremental; this catches regressions in the
            // backend/undo overlay commit without requiring GUI automation.
            QVariantList longStroke;
            longStroke.reserve(4000);
            for (int i = 0; i < 2000; ++i) {
                const double t = i / 1999.0;
                longStroke << (0.08 + 0.84 * t) << (0.50 + 0.08 * qSin(t * 24.0));
            }
            QElapsedTimer inkTimer;
            inkTimer.start();
            document->addInkStyled(0, longStroke, QStringLiteral("#2563EB"), 0.003, 100);
            const qint64 inkMs = inkTimer.elapsed();
            AppLogger::write(QStringLiteral("INFO"), QStringLiteral("INTERACTION_SMOKE_LONG_INK_MS=%1").arg(inkMs));
            if (inkMs > 5000) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_LONG_INK_TOO_SLOW"));
                return EXIT_FAILURE;
            }

            if (!document->modified() || document->currentPage() != 2 || !document->canUndo()) {
                AppLogger::write(QStringLiteral("FATAL"), QStringLiteral("INTERACTION_SMOKE_EDIT_STATE_FAILED"));
                return EXIT_FAILURE;
            }

            AppLogger::write(QStringLiteral("INFO"), QStringLiteral("INTERACTION_SMOKE_PASS"));
            QTimer::singleShot(250, &app, [&app] { app.quit(); });
        } else {
            const bool opened = documentManager.openDocument(firstArgument);
            if (opened && !engine.rootObjects().isEmpty())
                engine.rootObjects().first()->setProperty("homeVisible", false);
        }
    }

    return app.exec();
}
