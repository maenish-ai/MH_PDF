#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QSize>
#include <QStringList>
#include <atomic>
#include <functional>

class QPdfDocument;

class PdfToolsService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap providers READ providers NOTIFY providersChanged)
    Q_PROPERTY(QString defaultOcrLanguages READ defaultOcrLanguages CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString currentOperation READ currentOperation NOTIFY busyChanged)
public:
    explicit PdfToolsService(QObject *parent = nullptr);

    QVariantMap providers() const;
    QString defaultOcrLanguages() const { return QStringLiteral("eng+ara"); }
    bool busy() const { return m_busy.load(); }
    QString currentOperation() const { return m_currentOperation; }

    Q_INVOKABLE void refreshProviders();
    Q_INVOKABLE QString normalizePath(const QString &path) const;
    Q_INVOKABLE bool optimizePdf(const QString &input, const QString &output);
    Q_INVOKABLE bool linearizePdf(const QString &input, const QString &output);
    Q_INVOKABLE bool repairPdf(const QString &input, const QString &output);
    Q_INVOKABLE bool decryptPdf(const QString &input, const QString &output, const QString &password);
    Q_INVOKABLE QString checkPdf(const QString &input);
    Q_INVOKABLE bool splitPdf(const QString &input, const QString &outputDirectory, int pagesPerFile = 1);
    Q_INVOKABLE bool exportImages(const QString &input, const QString &outputDirectory, int dpi = 144,
                                  const QString &password = QString());
    Q_INVOKABLE bool imageToPdf(const QString &image, const QString &output);
    Q_INVOKABLE bool safeFlattenPdf(const QString &input, const QString &output, int dpi = 150,
                                    const QString &password = QString());
    Q_INVOKABLE QVariantList comparePdf(const QString &first, const QString &second, int maxPages = 0,
                                        const QString &firstPassword = QString(),
                                        const QString &secondPassword = QString());
    Q_INVOKABLE bool ocrToSearchablePdf(const QString &input, const QString &output,
                                        const QString &languages = QStringLiteral("eng+ara"), int dpi = 150,
                                        const QString &password = QString());
    Q_INVOKABLE bool officeToPdf(const QString &input, const QString &outputDirectory);
    Q_INVOKABLE QStringList ocrLanguages();

    Q_INVOKABLE bool startOptimizePdf(const QString &input, const QString &output);
    Q_INVOKABLE bool startLinearizePdf(const QString &input, const QString &output);
    Q_INVOKABLE bool startRepairPdf(const QString &input, const QString &output);
    Q_INVOKABLE bool startDecryptPdf(const QString &input, const QString &output, const QString &password);
    Q_INVOKABLE bool startSplitPdf(const QString &input, const QString &outputDirectory, int pagesPerFile = 1);
    Q_INVOKABLE bool startExportImages(const QString &input, const QString &outputDirectory, int dpi = 144, const QString &password = QString());
    Q_INVOKABLE bool startImageToPdf(const QString &image, const QString &output);
    Q_INVOKABLE bool startSafeFlattenPdf(const QString &input, const QString &output, int dpi = 150, const QString &password = QString());
    Q_INVOKABLE bool startComparePdf(const QString &first, const QString &second, int maxPages = 0);
    Q_INVOKABLE bool startOcrToSearchablePdf(const QString &input, const QString &output, const QString &languages = QStringLiteral("eng+ara"), int dpi = 150, const QString &password = QString());
    Q_INVOKABLE bool startOfficeToPdf(const QString &input, const QString &outputDirectory);
    Q_INVOKABLE bool startCheckPdf(const QString &input);
    Q_INVOKABLE bool startBatchOptimize(const QStringList &inputs, const QString &outputDirectory);

signals:
    void busyChanged();
    void compareReady(QVariantList results);
    void reportReady(QString report);
    void providersChanged();
    void operationFinished(QString key, QVariantList args);
    void operationFailed(QString key, QVariantList args);

private:
    bool runProcess(const QString &program, const QStringList &arguments, int timeoutMs,
                    QByteArray *standardOutput = nullptr, QByteArray *standardError = nullptr) const;
    bool loadPdf(QPdfDocument &document, const QString &path, const QString &password, QString *errorKey) const;
    bool writeFlattened(QPdfDocument &document, const QString &output, int dpi) const;
    bool mergePdfs(const QStringList &files, const QString &output, const QString &temporaryDirectory) const;
    QSize renderSize(QPdfDocument &document, int page, int dpi) const;
    void fail(const QString &key, const QVariantList &args = {});
    void done(const QString &key, const QVariantList &args = {});

    QString m_qpdf;
    QString m_tesseract;
    QString m_soffice;
    std::atomic_bool m_busy{false};
    QString m_currentOperation;
    bool beginAsync(const QString &operation, std::function<void()> task);
    void finishAsync();
    bool batchOptimize(const QStringList &inputs, const QString &outputDirectory);
};
