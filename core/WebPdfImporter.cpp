#include "WebPdfImporter.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUrlQuery>

#ifndef Q_OS_ANDROID
#include <QApplication>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWindow>
#endif

WebPdfImporter::WebPdfImporter(QObject *parent): QObject(parent)
{
#ifndef Q_OS_ANDROID
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (app) {
        // Keep the process alive when the main window is closed during an
        // active web download. The process exits automatically after the
        // background queue becomes idle.
        app->setQuitOnLastWindowClosed(false);
        app->installEventFilter(this);

        m_backgroundExitTimer = new QTimer(this);
        m_backgroundExitTimer->setSingleShot(true);
        // Batch downloads are queued from QML with Qt.callLater(). A short
        // grace period prevents the app from exiting in the tiny idle gap
        // between two queued files.
        m_backgroundExitTimer->setInterval(900);

        m_tray = new QSystemTrayIcon(QGuiApplication::windowIcon(), this);
        m_tray->setToolTip(QStringLiteral("MaenPDF — background downloads"));

        connect(m_tray, &QSystemTrayIcon::activated, this,
                [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger
                || reason == QSystemTrayIcon::DoubleClick) {
                restoreMainWindow();
            }
        });

        connect(m_backgroundExitTimer, &QTimer::timeout, this, [this, app]() {
            if (m_mainWindow && m_mainWindow->isVisible())
                return;
            if (m_busy)
                return;
            if (m_tray)
                m_tray->hide();
            app->quit();
        });

        connect(this, &WebPdfImporter::busyChanged,
                this, &WebPdfImporter::evaluateDesktopLifetime);
    }
#endif
}

bool WebPdfImporter::eventFilter(QObject *watched, QEvent *event)
{
#ifndef Q_OS_ANDROID
    auto *window = qobject_cast<QWindow *>(watched);
    if (window) {
        if (event->type() == QEvent::Show && m_mainWindow.isNull()) {
            // Main.qml's title always contains the product name. The Import
            // window has a different title, so it will not be mistaken for
            // the main application window.
            if (window->title().contains(QStringLiteral("MaenPDF"), Qt::CaseInsensitive))
                m_mainWindow = window;
        }

        if (event->type() == QEvent::Close) {
            if (m_mainWindow.isNull()
                && window->title().contains(QStringLiteral("MaenPDF"), Qt::CaseInsensitive)) {
                m_mainWindow = window;
            }

            if (window == m_mainWindow) {
                m_mainCloseRequested = true;
                // QML may reject the close because of unsaved documents or a
                // running PDF operation. Wait until the close event has fully
                // resolved before deciding whether the application is hidden.
                QTimer::singleShot(0, this, [this]() {
                    if (m_mainWindow && m_mainWindow->isVisible()) {
                        m_mainCloseRequested = false;
                        return;
                    }

                    // Hide secondary tool windows as well, so clicking X on
                    // the main window really looks like closing the program.
                    for (QWindow *w : QGuiApplication::topLevelWindows()) {
                        if (w && w != m_mainWindow)
                            w->hide();
                    }
                    evaluateDesktopLifetime();
                });
            }
        }
    }
#else
    Q_UNUSED(watched)
    Q_UNUSED(event)
#endif
    return QObject::eventFilter(watched, event);
}

#ifndef Q_OS_ANDROID
void WebPdfImporter::evaluateDesktopLifetime()
{
    if (!m_mainCloseRequested)
        return;

    if (m_mainWindow && m_mainWindow->isVisible()) {
        m_mainCloseRequested = false;
        if (m_tray)
            m_tray->hide();
        if (m_backgroundExitTimer)
            m_backgroundExitTimer->stop();
        return;
    }

    if (m_busy) {
        if (m_backgroundExitTimer)
            m_backgroundExitTimer->stop();

        if (m_tray && QSystemTrayIcon::isSystemTrayAvailable()) {
            const bool wasHidden = !m_tray->isVisible();
            m_tray->show();
            if (wasHidden) {
                m_tray->showMessage(
                    QStringLiteral("MaenPDF"),
                    QStringLiteral("PDF download is continuing in the background. "
                                   "Click the MaenPDF tray icon to reopen the app."),
                    QSystemTrayIcon::Information,
                    5000);
            }
        }
        return;
    }

    if (m_backgroundExitTimer && !m_backgroundExitTimer->isActive())
        m_backgroundExitTimer->start();
}

void WebPdfImporter::restoreMainWindow()
{
    if (m_mainWindow.isNull()) {
        for (QWindow *w : QGuiApplication::topLevelWindows()) {
            if (w && w->title().contains(QStringLiteral("MaenPDF"), Qt::CaseInsensitive)) {
                m_mainWindow = w;
                break;
            }
        }
    }

    if (!m_mainWindow)
        return;

    m_mainCloseRequested = false;
    if (m_backgroundExitTimer)
        m_backgroundExitTimer->stop();
    if (m_tray)
        m_tray->hide();

    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->requestActivate();
}
#endif

bool WebPdfImporter::allowed(const QUrl &url) const
{
    if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https")
        || url.host().isEmpty() || !url.userInfo().isEmpty())
        return false;

    const QString host = url.host().toLower();
    if (host == "localhost" || host.endsWith(".localhost")
        || host.endsWith(".local") || host.endsWith(".internal"))
        return false;

    QHostAddress ip;
    if (ip.setAddress(host)) {
        if (ip.isLoopback() || ip.isLinkLocal() || ip.isMulticast()
            || ip.isInSubnet(QHostAddress("10.0.0.0"), 8)
            || ip.isInSubnet(QHostAddress("172.16.0.0"), 12)
            || ip.isInSubnet(QHostAddress("192.168.0.0"), 16)
            || ip.isInSubnet(QHostAddress("100.64.0.0"), 10)
            || ip.isInSubnet(QHostAddress("0.0.0.0"), 8)
            || ip.isInSubnet(QHostAddress("fc00::"), 7)
            || ip.isInSubnet(QHostAddress("fe80::"), 10))
            return false;
    }
    return true;
}

void WebPdfImporter::inspect(const QString &s)
{
    start(QUrl::fromUserInput(s), true);
}

void WebPdfImporter::download(const QString &s)
{
    start(QUrl::fromUserInput(s), false);
}

void WebPdfImporter::start(const QUrl &url, bool inspect)
{
    if (m_busy)
        return;
    if (!allowed(url)) {
        emit failed("Invalid or restricted URL");
        return;
    }

    m_inspect = inspect;
    m_redirects = 0;
    m_pdfDetected = false;
    m_html.clear();
    m_progress = 0;
    emit progressChanged();
    m_busy = true;
    emit busyChanged();
    request(url);
}

QString WebPdfImporter::targetPath(const QUrl &url) const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    QString name = QFileInfo(url.path()).fileName();
    if (!name.endsWith(".pdf", Qt::CaseInsensitive))
        name = "download.pdf";
    name.replace(QRegularExpression("[^A-Za-z0-9._-]"), "_");

    QString stem = QFileInfo(name).completeBaseName();
    if (stem.isEmpty())
        stem = "download";

    QString path = QDir(dir).filePath(name);
    for (int i = 1; QFile::exists(path); ++i)
        path = QDir(dir).filePath(stem + "_" + QString::number(i) + ".pdf");
    return path;
}

void WebPdfImporter::request(const QUrl &url)
{
    if (!allowed(url)) {
        error("Redirect to restricted URL");
        return;
    }

    m_currentUrl = url;
    m_status = m_inspect ? "Inspecting page..." : "Downloading PDF...";
    emit statusChanged();

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(30000);
    req.setRawHeader("Accept", "application/pdf,text/html;q=0.9,*/*;q=0.5");
    req.setRawHeader("User-Agent", "MaenPDF-WebPdfImporter/1.0");

    QNetworkReply *reply = m_network.get(req);
    m_reply = reply;

    connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply]() {
        if (reply != m_reply)
            return;
        const qint64 length =
            reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if (length > (m_inspect ? MaxHtml : MaxPdf))
            reply->abort();
    });

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, reply](qint64 n, qint64 total) {
        if (reply != m_reply)
            return;
        if (n > (m_inspect ? MaxHtml : MaxPdf)) {
            reply->abort();
            return;
        }
        if (total > 0) {
            const qint64 percent = n * 100 / total;
            m_progress = int(percent < 0 ? 0 : (percent > 99 ? 99 : percent));
            emit progressChanged();
        }
    });

    connect(reply, &QIODevice::readyRead, this, [this, reply]() {
        if (reply != m_reply)
            return;

        if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
            reply->readAll();
            return;
        }

        QByteArray data = reply->readAll();

        if (m_inspect) {
            if (m_html.size() + data.size() > MaxHtml) {
                reply->abort();
                return;
            }
            m_html.append(data);
            return;
        }

        if (!m_output) {
            if (!m_pdfDetected) {
                m_html.append(data);
                if (m_html.size() < 5)
                    return;

                if (!m_html.startsWith("%PDF-")) {
                    // The URL is a webpage. Inspect only resources that the page
                    // itself exposes; never attempt to bypass sign-in, DRM or
                    // access controls.
                    m_inspect = true;
                    m_status = "Webpage detected. Finding directly exposed PDFs...";
                    emit statusChanged();
                    if (m_html.size() > MaxHtml)
                        reply->abort();
                    return;
                }

                data = m_html;
                m_html.clear();
                m_pdfDetected = true;
            }

            m_savedPath = targetPath(m_currentUrl);
            m_output = new QSaveFile(m_savedPath);
            if (!m_output->open(QIODevice::WriteOnly)) {
                reply->abort();
                return;
            }
        }

        if (m_output->size() + data.size() > MaxPdf
            || m_output->write(data) != data.size()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { finish(reply); });
}

void WebPdfImporter::finish(QNetworkReply *reply)
{
    if (reply != m_reply) {
        reply->deleteLater();
        return;
    }

    m_reply = nullptr;
    const QVariant redir =
        reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (redir.isValid() && reply->error() == QNetworkReply::NoError) {
        const QUrl next = m_currentUrl.resolved(redir.toUrl());
        reply->deleteLater();

        if (m_output) {
            m_output->cancelWriting();
            delete m_output;
            m_output = nullptr;
        }

        m_html.clear();
        if (++m_redirects > 8) {
            error("Too many redirects");
            return;
        }
        request(next);
        return;
    }

    const auto networkError = reply->error();
    const QString message = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        error(message);
        return;
    }

    if (m_inspect) {
        if (m_html.startsWith("%PDF-")) {
            QVariantMap entry;
            entry["url"] = m_currentUrl.toString();
            entry["title"] = "PDF";
            entry["host"] = m_currentUrl.host();
            emit candidatesFound(QVariantList{entry});
            m_status = "Direct PDF detected. Select it to download.";
            emit statusChanged();
        } else {
            parseHtml(m_currentUrl, m_html);
        }

        m_html.clear();
    } else {
        if (!m_output) {
            error("No valid PDF data received");
            return;
        }

        if (!m_output->commit()) {
            error("Unable to save PDF");
            return;
        }

        delete m_output;
        m_output = nullptr;
        m_progress = 100;
        emit progressChanged();
        m_status = "Saved: " + m_savedPath;
        emit statusChanged();
        emit downloaded(m_savedPath);
    }

    m_busy = false;
    emit busyChanged();
}

void WebPdfImporter::parseHtml(const QUrl &base, const QByteArray &bytes)
{
    QVariantList results;
    QSet<QString> seen;
    const QString html = QString::fromUtf8(bytes);

    auto cleanLabel = [](QString value) {
        value.remove(QRegularExpression("<[^>]*>"));
        value.replace("&amp;", "&");
        value.replace("&quot;", "\"");
        value.replace("&#39;", "'");
        value.replace("&nbsp;", " ");
        value = value.simplified();
        if (value.size() > 180)
            value = value.left(177) + "...";
        return value;
    };

    auto prettyTitle = [&](const QUrl &url, QString label) {
        label = cleanLabel(label);
        if (!label.isEmpty())
            return label;

        const QUrlQuery query(url);
        const QStringList preferredKeys{
            QStringLiteral("title"),
            QStringLiteral("filename"),
            QStringLiteral("file"),
            QStringLiteral("name")
        };
        for (const QString &key : preferredKeys) {
            QString value = query.queryItemValue(key, QUrl::FullyDecoded).simplified();
            if (!value.isEmpty()) {
                if (value.endsWith(".pdf", Qt::CaseInsensitive))
                    value.chop(4);
                return value;
            }
        }

        QString fileName = QFileInfo(url.path()).fileName();
        fileName = QUrl::fromPercentEncoding(fileName.toUtf8()).simplified();
        if (fileName.endsWith(".pdf", Qt::CaseInsensitive))
            fileName.chop(4);
        if (!fileName.isEmpty())
            return fileName;

        return url.host().isEmpty() ? QStringLiteral("PDF") : url.host();
    };

    auto addCandidate = [&](QString value, QString label, bool explicitPdf) {
        if (results.size() >= 100)
            return;

        value = value.trimmed();
        value.replace("&amp;", "&");
        value.replace("&quot;", "\"");
        if (value.isEmpty())
            return;

        const QUrl url = base.resolved(QUrl(value));
        if (!allowed(url))
            return;

        const bool looksPdf =
            url.path().endsWith(".pdf", Qt::CaseInsensitive)
            || url.query().contains(".pdf", Qt::CaseInsensitive);

        if (!explicitPdf && !looksPdf)
            return;

        const QString key = url.toString();
        if (seen.contains(key))
            return;

        seen.insert(key);
        QVariantMap entry;
        entry["url"] = key;
        entry["title"] = prettyTitle(url, label);
        entry["host"] = url.host();
        results.append(entry);
    };

    // Prefer visible anchor text as the book/document title.
    QRegularExpression anchorRx(
        R"(<a\b([^>]*)>(.*?)</a>)",
        QRegularExpression::CaseInsensitiveOption
            | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression attrUrlRx(
        R"((?:href|src|data|data-url)\s*=\s*["']([^"']+)["'])",
        QRegularExpression::CaseInsensitiveOption);

    auto anchorIt = anchorRx.globalMatch(html);
    while (anchorIt.hasNext() && results.size() < 100) {
        const auto match = anchorIt.next();
        const QString attrs = match.captured(1);
        const auto urlMatch = attrUrlRx.match(attrs);
        if (urlMatch.hasMatch())
            addCandidate(urlMatch.captured(1), match.captured(2), false);
    }

    // Normal resource attributes whose URL explicitly looks like a PDF.
    auto attrIt = attrUrlRx.globalMatch(html);
    while (attrIt.hasNext() && results.size() < 100)
        addCandidate(attrIt.next().captured(1), QString(), false);

    // Standards-based embedded PDFs may use a URL such as /download?id=123
    // with type="application/pdf" instead of a .pdf suffix.
    QRegularExpression pdfTagRx(
        R"(<[^>]+\btype\s*=\s*["']application/pdf(?:\s*;[^"']*)?["'][^>]*>)",
        QRegularExpression::CaseInsensitiveOption);
    auto tagIt = pdfTagRx.globalMatch(html);
    while (tagIt.hasNext() && results.size() < 100) {
        const QString tag = tagIt.next().captured(0);
        const auto urlMatch = attrUrlRx.match(tag);
        if (urlMatch.hasMatch())
            addCandidate(urlMatch.captured(1), QStringLiteral("PDF"), true);
    }

    emit candidatesFound(results);

    if (results.isEmpty()) {
        m_status = QStringLiteral(
            "No direct PDF link was found on this page. The site may use a "
            "JavaScript/embedded viewer, require sign-in, or not expose a "
            "downloadable PDF. Use a direct PDF URL, or download the file in "
            "your browser and open it in MaenPDF.");
    } else {
        m_status = QStringLiteral("Found %1 PDF link(s). Select the files you want.")
                       .arg(results.size());
    }
    emit statusChanged();
}

void WebPdfImporter::error(const QString &message)
{
    if (m_output) {
        m_output->cancelWriting();
        delete m_output;
        m_output = nullptr;
    }
    m_html.clear();
    m_status = message;
    emit statusChanged();
    m_busy = false;
    emit busyChanged();
    emit failed(message);
}

void WebPdfImporter::cancel()
{
    if (m_reply)
        m_reply->abort();
}

void WebPdfImporter::resetUi()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    if (reply) {
        reply->abort();
        reply->deleteLater();
    }

    if (m_output) {
        m_output->cancelWriting();
        delete m_output;
        m_output = nullptr;
    }

    m_html.clear();
    m_savedPath.clear();
    m_currentUrl = QUrl();
    m_inspect = false;
    m_pdfDetected = false;
    m_redirects = 0;
    m_busy = false;
    m_progress = 0;
    m_status.clear();
    emit busyChanged();
    emit progressChanged();
    emit statusChanged();
}
