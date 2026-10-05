#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "../core/AppLogger.h"
#include "../core/LanguageManager.h"
#include "../core/PageImageProvider.h"
#include "../core/PdfDocument.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    AppLogger::install();

    QGuiApplication::setOrganizationName(QStringLiteral("MaenPDF"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("maenpdf.local"));
    QGuiApplication::setApplicationName(QStringLiteral("MaenPDF"));
    QGuiApplication::setApplicationVersion(QStringLiteral("6.0.8"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/PDFStudio/assets/maenpdf-logo.svg")));

    LanguageManager languageManager;
    PdfDocument document;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("pdfDocument"), &document);
    engine.rootContext()->setContextProperty(QStringLiteral("i18n"), &languageManager);
    engine.addImageProvider(QStringLiteral("maenpdf"), new PageImageProvider(document.pageModel()));
    engine.loadFromModule(QStringLiteral("PDFStudio"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return -1;

    if (argc > 1)
        document.openDocument(QString::fromLocal8Bit(argv[1]));
    return app.exec();
}
