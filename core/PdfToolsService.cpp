#include "PdfToolsService.h"
#include "MemoryPolicy.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPageSize>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>
#include <QtMath>
#include <utility>
#include <QtConcurrent>
#include <QPointer>
#include <QMetaObject>

namespace {

QString findProviderExecutable(const QStringList &relativeCandidates, const QString &systemName) {
    const QDir appDir(QCoreApplication::applicationDirPath());
    for (const QString &relative : relativeCandidates) {
        const QString candidate = appDir.filePath(relative);
        if (QFileInfo::exists(candidate))
            return QFileInfo(candidate).absoluteFilePath();
#ifdef Q_OS_WIN
        if (!candidate.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive) && QFileInfo::exists(candidate + QStringLiteral(".exe")))
            return QFileInfo(candidate + QStringLiteral(".exe")).absoluteFilePath();
#endif
    }
    return QStandardPaths::findExecutable(systemName);
}

QString ensurePdfSuffix(QString path) {
    if (!path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        path += QStringLiteral(".pdf");
    return path;
}

bool ensureParent(const QString &path) {
    QFileInfo info(path);
    QDir dir = info.dir();
    return dir.exists() || dir.mkpath(QStringLiteral("."));
}
}

PdfToolsService::PdfToolsService(QObject *parent) : QObject(parent) {
    refreshProviders();
}

void PdfToolsService::refreshProviders() {
    const QString oldQpdf = m_qpdf;
    const QString oldTesseract = m_tesseract;
    const QString oldSoffice = m_soffice;
    m_qpdf = findProviderExecutable({QStringLiteral("tools/qpdf/qpdf"), QStringLiteral("providers/qpdf/qpdf")}, QStringLiteral("qpdf"));
    m_tesseract = findProviderExecutable({QStringLiteral("tools/tesseract/tesseract"), QStringLiteral("providers/tesseract/tesseract")}, QStringLiteral("tesseract"));
    m_soffice = findProviderExecutable({QStringLiteral("tools/libreoffice/program/soffice"), QStringLiteral("providers/libreoffice/program/soffice")}, QStringLiteral("soffice"));
    if (m_qpdf != oldQpdf || m_tesseract != oldTesseract || m_soffice != oldSoffice)
        emit providersChanged();
}

QVariantMap PdfToolsService::providers() const {
    return {
        {QStringLiteral("qpdf"), !m_qpdf.isEmpty()},
        {QStringLiteral("tesseract"), !m_tesseract.isEmpty()},
        {QStringLiteral("libreOffice"), !m_soffice.isEmpty()},
        {QStringLiteral("localOnly"), true},
        {QStringLiteral("cloud"), false}
    };
}

QString PdfToolsService::normalizePath(const QString &path) const {
    const QUrl url(path);
    if (url.isLocalFile())
        return QFileInfo(url.toLocalFile()).absoluteFilePath();
    if (url.scheme().isEmpty())
        return QFileInfo(path).absoluteFilePath();
    return {};
}

void PdfToolsService::fail(const QString &key, const QVariantList &args) {
    emit operationFailed(key, args);
}

void PdfToolsService::done(const QString &key, const QVariantList &args) {
    emit operationFinished(key, args);
}

bool PdfToolsService::runProcess(const QString &program, const QStringList &arguments, int timeoutMs,
                                 QByteArray *standardOutput, QByteArray *standardError) const {
    if (program.isEmpty())
        return false;
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    if (program == m_tesseract) {
        const QString tessdata = QFileInfo(program).dir().filePath(QStringLiteral("tessdata"));
        if (QDir(tessdata).exists()) {
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            environment.insert(QStringLiteral("TESSDATA_PREFIX"), tessdata);
            process.setProcessEnvironment(environment);
        }
    }
    process.start(program, arguments);
    if (!process.waitForStarted(5000))
        return false;
    QElapsedTimer timer;
    timer.start();
    while (process.state() != QProcess::NotRunning && timer.elapsed() < timeoutMs) {
        process.waitForFinished(35);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);
    }
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(3000);
        return false;
    }
    if (standardOutput)
        *standardOutput = process.readAllStandardOutput();
    if (standardError)
        *standardError = process.readAllStandardError();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool PdfToolsService::loadPdf(QPdfDocument &document, const QString &path, const QString &password,
                              QString *errorKey) const {
    const QString local = normalizePath(path);
    if (local.isEmpty() || !QFileInfo::exists(local)) {
        if (errorKey) *errorKey = QStringLiteral("tools.error.input_missing");
        return false;
    }
    if (!password.isEmpty())
        document.setPassword(password);
    const auto result = document.load(local);
    if (result == QPdfDocument::Error::None)
        return true;
    if (errorKey) {
        if (result == QPdfDocument::Error::IncorrectPassword)
            *errorKey = QStringLiteral("tools.error.password");
        else if (result == QPdfDocument::Error::UnsupportedSecurityScheme)
            *errorKey = QStringLiteral("tools.error.security_scheme");
        else
            *errorKey = QStringLiteral("tools.error.invalid_pdf");
    }
    return false;
}

QSize PdfToolsService::renderSize(QPdfDocument &document, int page, int dpi) const {
    const QSizeF points = document.pagePointSize(page);
    const qreal scale = qBound(0.5, dpi / 72.0, 6.0);
    QSize size(qMax(64, qRound(points.width() * scale)), qMax(64, qRound(points.height() * scale)));
    const int maxDim = qMax(1600, MemoryPolicy::maxRenderDimension());
    if (size.width() > maxDim || size.height() > maxDim)
        size.scale(maxDim, maxDim, Qt::KeepAspectRatio);
    return size;
}

bool PdfToolsService::writeFlattened(QPdfDocument &document, const QString &outputPath, int dpi) const {
    const QString output = ensurePdfSuffix(normalizePath(outputPath));
    if (output.isEmpty() || document.pageCount() <= 0 || !ensureParent(output))
        return false;

    QPdfWriter writer(output);
    writer.setResolution(qBound(96, dpi, 300));
    writer.setCreator(QStringLiteral("MaenPDF 7"));
    writer.setTitle(QStringLiteral("MaenPDF flattened PDF"));
    writer.setPageSize(QPageSize(document.pagePointSize(0), QPageSize::Point));
    QPainter painter(&writer);
    if (!painter.isActive())
        return false;

    for (int page = 0; page < document.pageCount(); ++page) {
        if (page > 0) {
            writer.setPageSize(QPageSize(document.pagePointSize(page), QPageSize::Point));
            writer.newPage();
        }
        const QImage image = document.render(page, renderSize(document, page, dpi));
        if (image.isNull()) {
            painter.end();
            QFile::remove(output);
            return false;
        }
        painter.drawImage(QRect(0, 0, writer.width(), writer.height()), image);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 8);
    }
    painter.end();
    return QFileInfo(output).exists() && QFileInfo(output).size() > 0;
}

bool PdfToolsService::optimizePdf(const QString &inputPath, const QString &outputPath) {
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return false; }
    const QString input = normalizePath(inputPath);
    const QString output = ensurePdfSuffix(normalizePath(outputPath));
    if (input.isEmpty() || output.isEmpty() || !ensureParent(output)) { fail(QStringLiteral("tools.error.path")); return false; }
    const bool ok = runProcess(m_qpdf, {QStringLiteral("--object-streams=generate"), QStringLiteral("--compress-streams=y"),
                                        QStringLiteral("--recompress-flate"), QStringLiteral("--linearize"), input, output}, 180000);
    if (ok) done(QStringLiteral("tools.done.optimize")); else fail(QStringLiteral("tools.error.process"));
    return ok;
}

bool PdfToolsService::linearizePdf(const QString &inputPath, const QString &outputPath) {
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return false; }
    const QString input = normalizePath(inputPath);
    const QString output = ensurePdfSuffix(normalizePath(outputPath));
    if (input.isEmpty() || output.isEmpty() || !ensureParent(output)) { fail(QStringLiteral("tools.error.path")); return false; }
    const bool ok = runProcess(m_qpdf, {QStringLiteral("--linearize"), input, output}, 180000);
    if (ok) done(QStringLiteral("tools.done.linearize")); else fail(QStringLiteral("tools.error.process"));
    return ok;
}

bool PdfToolsService::repairPdf(const QString &inputPath, const QString &outputPath) {
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return false; }
    const QString input = normalizePath(inputPath);
    const QString output = ensurePdfSuffix(normalizePath(outputPath));
    if (input.isEmpty() || output.isEmpty() || !ensureParent(output)) { fail(QStringLiteral("tools.error.path")); return false; }
    const bool ok = runProcess(m_qpdf, {input, output}, 180000);
    if (ok) done(QStringLiteral("tools.done.repair")); else fail(QStringLiteral("tools.error.process"));
    return ok;
}

bool PdfToolsService::decryptPdf(const QString &inputPath, const QString &outputPath, const QString &password) {
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return false; }
    const QString input = normalizePath(inputPath);
    const QString output = ensurePdfSuffix(normalizePath(outputPath));
    if (input.isEmpty() || output.isEmpty() || !ensureParent(output)) { fail(QStringLiteral("tools.error.path")); return false; }
    const bool ok = runProcess(m_qpdf, {QStringLiteral("--password=") + password, QStringLiteral("--decrypt"), input, output}, 180000);
    if (ok) done(QStringLiteral("tools.done.decrypt")); else fail(QStringLiteral("tools.error.password_or_process"));
    return ok;
}

QString PdfToolsService::checkPdf(const QString &inputPath) {
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return {}; }
    const QString input = normalizePath(inputPath);
    if (input.isEmpty()) { fail(QStringLiteral("tools.error.path")); return {}; }
    QByteArray out, err;
    const bool ok = runProcess(m_qpdf, {QStringLiteral("--check"), input}, 120000, &out, &err);
    const QString report = QString::fromUtf8(out + err).trimmed();
    if (ok) done(QStringLiteral("tools.done.check")); else fail(QStringLiteral("tools.error.check_failed"));
    return report;
}

bool PdfToolsService::splitPdf(const QString &inputPath, const QString &outputDirectory, int pagesPerFile) {
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return false; }
    const QString input = normalizePath(inputPath);
    const QString dirPath = normalizePath(outputDirectory);
    if (input.isEmpty() || dirPath.isEmpty()) { fail(QStringLiteral("tools.error.path")); return false; }
    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) { fail(QStringLiteral("tools.error.path")); return false; }
    const QString pattern = dir.filePath(QStringLiteral("page-%d.pdf"));
    const bool ok = runProcess(m_qpdf, {QStringLiteral("--split-pages=") + QString::number(qMax(1, pagesPerFile)), input, pattern}, 180000);
    if (ok) done(QStringLiteral("tools.done.split")); else fail(QStringLiteral("tools.error.process"));
    return ok;
}

bool PdfToolsService::exportImages(const QString &inputPath, const QString &outputDirectory, int dpi,
                                   const QString &password) {
    QPdfDocument document;
    QString errorKey;
    if (!loadPdf(document, inputPath, password, &errorKey)) { fail(errorKey); return false; }
    const QString dirPath = normalizePath(outputDirectory);
    QDir dir(dirPath);
    if (dirPath.isEmpty() || (!dir.exists() && !dir.mkpath(QStringLiteral(".")))) { fail(QStringLiteral("tools.error.path")); return false; }
    for (int page = 0; page < document.pageCount(); ++page) {
        const QImage image = document.render(page, renderSize(document, page, qBound(72, dpi, 300)));
        const QString name = dir.filePath(QStringLiteral("page-%1.png").arg(page + 1, 4, 10, QLatin1Char('0')));
        if (image.isNull() || !image.save(name, "PNG")) { fail(QStringLiteral("tools.error.image_write")); return false; }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 8);
    }
    done(QStringLiteral("tools.done.images"), {document.pageCount()});
    return true;
}

bool PdfToolsService::imageToPdf(const QString &imagePath, const QString &outputPath) {
    const QString imageLocal = normalizePath(imagePath);
    const QString output = ensurePdfSuffix(normalizePath(outputPath));
    QImageReader reader(imageLocal);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull() || output.isEmpty() || !ensureParent(output)) { fail(QStringLiteral("tools.error.image_read")); return false; }
    QPdfWriter writer(output);
    writer.setResolution(144);
    const QSizeF points(image.width() * 72.0 / 144.0, image.height() * 72.0 / 144.0);
    writer.setPageSize(QPageSize(points, QPageSize::Point));
    writer.setCreator(QStringLiteral("MaenPDF 7"));
    QPainter painter(&writer);
    if (!painter.isActive()) { fail(QStringLiteral("tools.error.image_write")); return false; }
    painter.drawImage(QRect(0, 0, writer.width(), writer.height()), image);
    painter.end();
    done(QStringLiteral("tools.done.image_pdf"));
    return true;
}

bool PdfToolsService::safeFlattenPdf(const QString &inputPath, const QString &outputPath, int dpi,
                                     const QString &password) {
    QPdfDocument document;
    QString errorKey;
    if (!loadPdf(document, inputPath, password, &errorKey)) { fail(errorKey); return false; }
    const bool ok = writeFlattened(document, outputPath, dpi);
    if (ok) done(QStringLiteral("tools.done.safe_flatten")); else fail(QStringLiteral("tools.error.write_pdf"));
    return ok;
}

QVariantList PdfToolsService::comparePdf(const QString &firstPath, const QString &secondPath, int maxPages,
                                         const QString &firstPassword, const QString &secondPassword) {
    QVariantList results;
    QPdfDocument first, second;
    QString errorKey;
    if (!loadPdf(first, firstPath, firstPassword, &errorKey)) { fail(errorKey); return results; }
    if (!loadPdf(second, secondPath, secondPassword, &errorKey)) { fail(errorKey); return results; }

    int pages = qMax(first.pageCount(), second.pageCount());
    if (maxPages > 0)
        pages = qMin(pages, maxPages);

    for (int page = 0; page < pages; ++page) {
        QVariantMap row;
        row.insert(QStringLiteral("page"), page);
        if (page >= first.pageCount() || page >= second.pageCount()) {
            row.insert(QStringLiteral("differencePercent"), 100.0);
            row.insert(QStringLiteral("same"), false);
            row.insert(QStringLiteral("missingPage"), true);
            results.push_back(row);
            continue;
        }

        QSize size(900, 1200);
        const QSizeF aPoints = first.pagePointSize(page);
        if (aPoints.width() > 0 && aPoints.height() > 0)
            size = QSize(qRound(900.0), qRound(900.0 * aPoints.height() / aPoints.width()));
        size.setHeight(qBound(400, size.height(), 1400));
        QImage a = first.render(page, size).convertToFormat(QImage::Format_ARGB32);
        QImage b = second.render(page, size).convertToFormat(QImage::Format_ARGB32);
        if (a.isNull() || b.isNull()) {
            row.insert(QStringLiteral("differencePercent"), 100.0);
            row.insert(QStringLiteral("same"), false);
            row.insert(QStringLiteral("renderError"), true);
            results.push_back(row);
            continue;
        }
        if (b.size() != a.size())
            b = b.scaled(a.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        quint64 totalDifference = 0;
        quint64 samples = 0;
        for (int y = 0; y < a.height(); y += 4) {
            const QRgb *arow = reinterpret_cast<const QRgb *>(a.constScanLine(y));
            const QRgb *brow = reinterpret_cast<const QRgb *>(b.constScanLine(y));
            for (int x = 0; x < a.width(); x += 4) {
                totalDifference += qAbs(qRed(arow[x]) - qRed(brow[x]));
                totalDifference += qAbs(qGreen(arow[x]) - qGreen(brow[x]));
                totalDifference += qAbs(qBlue(arow[x]) - qBlue(brow[x]));
                samples += 3;
            }
            if ((y & 63) == 0)
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 4);
        }
        const double percent = samples ? (100.0 * totalDifference / (255.0 * samples)) : 0.0;
        row.insert(QStringLiteral("differencePercent"), percent);
        row.insert(QStringLiteral("same"), percent < 0.35);
        row.insert(QStringLiteral("missingPage"), false);
        results.push_back(row);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 8);
    }
    done(QStringLiteral("tools.done.compare"), {results.size()});
    return results;
}

bool PdfToolsService::mergePdfs(const QStringList &files, const QString &output, const QString &temporaryDirectory) const {
    if (files.isEmpty() || m_qpdf.isEmpty())
        return false;
    if (files.size() == 1) {
        QFile::remove(output);
        return QFile::copy(files.first(), output);
    }

    QStringList current = files;
    int round = 0;
    while (current.size() > 1) {
        QStringList next;
        for (int start = 0; start < current.size(); start += 30) {
            const QStringList group = current.mid(start, 30);
            const QString groupOut = QDir(temporaryDirectory).filePath(
                QStringLiteral("merge-%1-%2.pdf").arg(round).arg(start / 30));
            QStringList args{QStringLiteral("--empty"), QStringLiteral("--pages")};
            for (const QString &file : group) {
                args << file << QStringLiteral("1-z");
            }
            args << QStringLiteral("--") << groupOut;
            if (!runProcess(m_qpdf, args, 240000))
                return false;
            next << groupOut;
        }
        current = next;
        ++round;
    }
    QFile::remove(output);
    return QFile::copy(current.first(), output);
}

bool PdfToolsService::ocrToSearchablePdf(const QString &inputPath, const QString &outputPath,
                                         const QString &languages, int dpi, const QString &password) {
    if (m_tesseract.isEmpty()) { fail(QStringLiteral("tools.error.tesseract_missing")); return false; }
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return false; }
    static const QRegularExpression languagePattern(QStringLiteral("^[A-Za-z0-9_+.-]+$"));
    if (!languagePattern.match(languages).hasMatch()) { fail(QStringLiteral("tools.error.ocr_language")); return false; }

    QPdfDocument document;
    QString errorKey;
    if (!loadPdf(document, inputPath, password, &errorKey)) { fail(errorKey); return false; }
    const QString output = ensurePdfSuffix(normalizePath(outputPath));
    if (output.isEmpty() || !ensureParent(output)) { fail(QStringLiteral("tools.error.path")); return false; }

    QTemporaryDir temporary;
    if (!temporary.isValid()) { fail(QStringLiteral("tools.error.temp")); return false; }
    QStringList pagePdfs;
    for (int page = 0; page < document.pageCount(); ++page) {
        const QString imagePath = QDir(temporary.path()).filePath(QStringLiteral("ocr-%1.png").arg(page, 5, 10, QLatin1Char('0')));
        const QString base = QDir(temporary.path()).filePath(QStringLiteral("ocr-%1").arg(page, 5, 10, QLatin1Char('0')));
        const QImage image = document.render(page, renderSize(document, page, qBound(100, dpi, 300)));
        if (image.isNull() || !image.save(imagePath, "PNG")) { fail(QStringLiteral("tools.error.image_write")); return false; }
        if (!runProcess(m_tesseract, {imagePath, base, QStringLiteral("-l"), languages,
                                      QStringLiteral("--dpi"), QString::number(qBound(100, dpi, 300)), QStringLiteral("pdf")}, 240000)) {
            fail(QStringLiteral("tools.error.ocr_process"), {page + 1});
            return false;
        }
        const QString pagePdf = base + QStringLiteral(".pdf");
        if (!QFileInfo::exists(pagePdf)) { fail(QStringLiteral("tools.error.ocr_process"), {page + 1}); return false; }
        pagePdfs << pagePdf;
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 8);
    }
    const bool ok = mergePdfs(pagePdfs, output, temporary.path());
    if (ok) done(QStringLiteral("tools.done.ocr"), {document.pageCount()}); else fail(QStringLiteral("tools.error.merge"));
    return ok;
}

bool PdfToolsService::officeToPdf(const QString &inputPath, const QString &outputDirectory) {
    if (m_soffice.isEmpty()) { fail(QStringLiteral("tools.error.libreoffice_missing")); return false; }
    const QString input = normalizePath(inputPath);
    const QString dirPath = normalizePath(outputDirectory);
    QDir dir(dirPath);
    if (input.isEmpty() || dirPath.isEmpty() || (!dir.exists() && !dir.mkpath(QStringLiteral(".")))) {
        fail(QStringLiteral("tools.error.path")); return false;
    }
    const bool ok = runProcess(m_soffice, {QStringLiteral("--headless"), QStringLiteral("--convert-to"), QStringLiteral("pdf"),
                                           QStringLiteral("--outdir"), dirPath, input}, 240000);
    if (ok) done(QStringLiteral("tools.done.office")); else fail(QStringLiteral("tools.error.process"));
    return ok;
}

QStringList PdfToolsService::ocrLanguages() {
    QStringList languages;
    if (m_tesseract.isEmpty())
        return languages;
    QByteArray out;
    if (!runProcess(m_tesseract, {QStringLiteral("--list-langs")}, 15000, &out, nullptr))
        return languages;
    const QStringList lines = QString::fromUtf8(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.startsWith(QStringLiteral("List of available languages")) && !trimmed.isEmpty())
            languages << trimmed;
    }
    return languages;
}


bool PdfToolsService::beginAsync(const QString &operation, std::function<void()> task) {
    bool expected = false;
    if (!m_busy.compare_exchange_strong(expected, true))
        return false;
    m_currentOperation = operation;
    emit busyChanged();
    QPointer<PdfToolsService> guard(this);
    QtConcurrent::run([guard, task = std::move(task)]() mutable {
        if (!guard)
            return;
        task();
        if (guard)
            QMetaObject::invokeMethod(guard.data(), [guard] { if (guard) guard->finishAsync(); }, Qt::QueuedConnection);
    });
    return true;
}

void PdfToolsService::finishAsync() {
    m_currentOperation.clear();
    m_busy.store(false);
    emit busyChanged();
}

bool PdfToolsService::startOptimizePdf(const QString &input, const QString &output) {
    return beginAsync(QStringLiteral("optimize"), [this, input, output] { optimizePdf(input, output); });
}
bool PdfToolsService::startLinearizePdf(const QString &input, const QString &output) {
    return beginAsync(QStringLiteral("linearize"), [this, input, output] { linearizePdf(input, output); });
}
bool PdfToolsService::startRepairPdf(const QString &input, const QString &output) {
    return beginAsync(QStringLiteral("repair"), [this, input, output] { repairPdf(input, output); });
}
bool PdfToolsService::startDecryptPdf(const QString &input, const QString &output, const QString &password) {
    return beginAsync(QStringLiteral("decrypt"), [this, input, output, password] { decryptPdf(input, output, password); });
}
bool PdfToolsService::startSplitPdf(const QString &input, const QString &outputDirectory, int pagesPerFile) {
    return beginAsync(QStringLiteral("split"), [this, input, outputDirectory, pagesPerFile] { splitPdf(input, outputDirectory, pagesPerFile); });
}
bool PdfToolsService::startExportImages(const QString &input, const QString &outputDirectory, int dpi, const QString &password) {
    return beginAsync(QStringLiteral("exportImages"), [this, input, outputDirectory, dpi, password] { exportImages(input, outputDirectory, dpi, password); });
}
bool PdfToolsService::startImageToPdf(const QString &image, const QString &output) {
    return beginAsync(QStringLiteral("imageToPdf"), [this, image, output] { imageToPdf(image, output); });
}
bool PdfToolsService::startSafeFlattenPdf(const QString &input, const QString &output, int dpi, const QString &password) {
    return beginAsync(QStringLiteral("safeFlatten"), [this, input, output, dpi, password] { safeFlattenPdf(input, output, dpi, password); });
}
bool PdfToolsService::startComparePdf(const QString &first, const QString &second, int maxPages) {
    return beginAsync(QStringLiteral("compare"), [this, first, second, maxPages] {
        const QVariantList result = comparePdf(first, second, maxPages);
        QPointer<PdfToolsService> guard(this);
        QMetaObject::invokeMethod(this, [guard, result] { if (guard) emit guard->compareReady(result); }, Qt::QueuedConnection);
    });
}
bool PdfToolsService::startOcrToSearchablePdf(const QString &input, const QString &output, const QString &languages, int dpi, const QString &password) {
    return beginAsync(QStringLiteral("ocr"), [this, input, output, languages, dpi, password] { ocrToSearchablePdf(input, output, languages, dpi, password); });
}
bool PdfToolsService::startOfficeToPdf(const QString &input, const QString &outputDirectory) {
    return beginAsync(QStringLiteral("office"), [this, input, outputDirectory] { officeToPdf(input, outputDirectory); });
}
bool PdfToolsService::startCheckPdf(const QString &input) {
    return beginAsync(QStringLiteral("check"), [this, input] {
        const QString report = checkPdf(input);
        QPointer<PdfToolsService> guard(this);
        QMetaObject::invokeMethod(this, [guard, report] { if (guard) emit guard->reportReady(report); }, Qt::QueuedConnection);
    });
}


bool PdfToolsService::batchOptimize(const QStringList &inputs, const QString &outputDirectory) {
    if (m_qpdf.isEmpty()) { fail(QStringLiteral("tools.error.qpdf_missing")); return false; }
    const QString dirPath = normalizePath(outputDirectory);
    QDir dir(dirPath);
    if (inputs.isEmpty() || dirPath.isEmpty() || (!dir.exists() && !dir.mkpath(QStringLiteral(".")))) {
        fail(QStringLiteral("tools.error.path"));
        return false;
    }
    int completed = 0;
    for (const QString &entry : inputs) {
        const QString input = normalizePath(entry);
        if (input.isEmpty() || !QFileInfo::exists(input))
            continue;
        const QFileInfo info(input);
        QString outName = info.completeBaseName() + QStringLiteral("-optimized.pdf");
        const QString output = dir.filePath(outName);
        const bool ok = runProcess(m_qpdf, {QStringLiteral("--object-streams=generate"), QStringLiteral("--compress-streams=y"),
                                            QStringLiteral("--recompress-flate"), QStringLiteral("--linearize"), input, output}, 180000);
        if (!ok) {
            fail(QStringLiteral("tools.error.process"));
            return false;
        }
        ++completed;
    }
    if (completed == 0) {
        fail(QStringLiteral("tools.error.input_missing"));
        return false;
    }
    done(QStringLiteral("tools.done.batch_optimize"), {completed});
    return true;
}

bool PdfToolsService::startBatchOptimize(const QStringList &inputs, const QString &outputDirectory) {
    return beginAsync(QStringLiteral("batchOptimize"), [this, inputs, outputDirectory] { batchOptimize(inputs, outputDirectory); });
}
