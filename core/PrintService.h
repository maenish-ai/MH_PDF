#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class PdfDocument;

class PrintService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
public:
    explicit PrintService(QObject *parent = nullptr);

    bool available() const;

    Q_INVOKABLE QVariantList printers() const;
    Q_INVOKABLE QVariantList paperSizes(const QString &printerName) const;
    Q_INVOKABLE bool printDocument(QObject *documentObject, const QVariantMap &settings);
    Q_INVOKABLE bool printPreview(QObject *documentObject, const QVariantMap &settings);

signals:
    void operationFinished(QString key, QVariantList args);
    void operationFailed(QString key, QVariantList args);

private:
    bool runJob(QObject *documentObject, const QVariantMap &settings, bool preview);
#ifndef Q_OS_ANDROID
    bool paintDocument(PdfDocument *document, class QPrinter *printer, int first, int last,
                       int copies, int scalePercent) const;
#endif
};
