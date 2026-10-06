#pragma once

#include <QObject>
#include <QVariantList>

class PdfDocument;

class PrintService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
public:
    explicit PrintService(QObject *parent = nullptr);

    bool available() const;

    Q_INVOKABLE bool printDocument(QObject *documentObject, bool currentPageOnly = false);
    Q_INVOKABLE bool printPreview(QObject *documentObject, bool currentPageOnly = false);

signals:
    void operationFinished(QString key, QVariantList args);
    void operationFailed(QString key, QVariantList args);

private:
#ifndef Q_OS_ANDROID
    bool paintDocument(PdfDocument *document, class QPrinter *printer, bool currentPageOnly) const;
#endif
};
