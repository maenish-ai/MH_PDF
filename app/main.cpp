#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QGuiApplication>
#include <QIcon>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlApplicationEngine>
#include <QQmlContext>
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
#include "../core/LanguageManager.h"
#include "../core/PageImageProvider.h"
#include "../core/PdfDocument.h"

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

    QGuiApplication app(argc, argv);

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
    QGuiApplication::setApplicationVersion(QStringLiteral("6.0.11"));
    AppLogger::install();

    // Make startup independent of how the process was launched (desktop
    // shortcut, Start menu, Explorer, terminal, file association, etc.).
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    AppLogger::write(QStringLiteral("INFO"),
                     QStringLiteral("MaenPDF 6.0.11 startup; Qt %1; appDir=%2; cwd=%3; QT_QUICK_BACKEND=%4")
                         .arg(QString::fromLatin1(qVersion()),
                              QCoreApplication::applicationDirPath(),
                              QDir::currentPath(),
                              qEnvironmentVariable("QT_QUICK_BACKEND")));

    QGuiApplication::setWindowIcon(
        QIcon(QStringLiteral(":/qt/qml/PDFStudio/assets/maenpdf-logo.svg")));

    LanguageManager languageManager;
    PdfDocument document;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("pdfDocument"), &document);
    engine.rootContext()->setContextProperty(QStringLiteral("i18n"), &languageManager);
    engine.addImageProvider(QStringLiteral("maenpdf"),
                            new PageImageProvider(document.pageModel()));

    engine.loadFromModule(QStringLiteral("PDFStudio"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) {
        showStartupFailure(QStringLiteral(
            "The main interface could not be created. This is usually caused by "
            "a missing Qt runtime/plugin or a QML startup error."));
        return EXIT_FAILURE;
    }

    AppLogger::write(QStringLiteral("INFO"), QStringLiteral("Main window created successfully"));

    if (argc > 1)
        document.openDocument(QString::fromLocal8Bit(argv[1]));

    return app.exec();
}
