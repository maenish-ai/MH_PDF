#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QGuiApplication>
#ifndef Q_OS_ANDROID
#include <QApplication>
#endif
#include <QIcon>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QtGlobal>
#include <cstdlib>

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
#ifdef Q_OS_WIN
    // Force the most compatible Qt Quick path before any window is created.
    // This avoids silent startup failures caused by broken GPU/D3D drivers.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND"))
        qputenv("QT_QUICK_BACKEND", QByteArrayLiteral("software"));
#endif

#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

#ifdef Q_OS_WIN
    // These calls are made after QGuiApplication exists but before the first
    // QQuickWindow/QML control is created. Pin both rendering and controls to
    // the compatibility-oriented paths used by MaenPDF on Windows.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
#endif

    // Set application identity before the logger resolves AppLocalDataLocation
    // so every startup message is written to one deterministic path.
    QGuiApplication::setOrganizationName(QStringLiteral("MaenPDF"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("maenpdf.local"));
    QGuiApplication::setApplicationName(QStringLiteral("MaenPDF"));
    QGuiApplication::setApplicationVersion(QStringLiteral("7.2.1"));
    AppLogger::install();

    // Make startup independent of how the process was launched (desktop
    // shortcut, Start menu, Explorer, terminal, file association, etc.).
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    AppLogger::write(QStringLiteral("INFO"),
                     QStringLiteral("MaenPDF 7.2.1 startup; Qt %1; appDir=%2; cwd=%3; QT_QUICK_BACKEND=%4")
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

    if (argc > 1) {
        const QString firstArgument = QString::fromLocal8Bit(argv[1]);
        if (firstArgument == QStringLiteral("--interaction-smoke")) {
            if (argc < 3 || !documentManager.openDocument(QString::fromLocal8Bit(argv[2]))) {
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
            document->addInkStyled(0, QVariantList{0.15, 0.20, 0.30, 0.24, 0.45, 0.21},
                                   QStringLiteral("#2563EB"), 0.004, 100);
            document->addRedaction(1, 0.12, 0.12, 0.22, 0.08);
            document->cropPage(2, 0.05, 0.05, 0.90, 0.90);
            document->setCurrentPage(2);
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
