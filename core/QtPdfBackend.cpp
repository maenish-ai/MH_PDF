#include "QtPdfBackend.h"
#include <QPdfSelection>

bool QtPdfBackend::open(const QString &path, const QString &password, QString *error) {
    m_doc.close();
    if (!password.isEmpty())
        m_doc.setPassword(password);
    const auto e = m_doc.load(path);
    if (e == QPdfDocument::Error::None)
        return true;
    if (error) {
        if (e == QPdfDocument::Error::IncorrectPassword)
            *error = QStringLiteral("password");
        else if (e == QPdfDocument::Error::UnsupportedSecurityScheme)
            *error = QStringLiteral("unsupported-security");
        else
            *error = QStringLiteral("open-failed");
    }
    return false;
}
QString QtPdfBackend::pageText(int page) const {
    if (page < 0 || page >= m_doc.pageCount()) return {};
    return m_doc.getAllText(page).text();
}
QVariantList QtPdfBackend::search(const QString &needle, int maxResults) const {
    QVariantList out;
    const QString q = needle.trimmed();
    if (q.isEmpty() || maxResults <= 0) return out;
    for (int p=0; p<m_doc.pageCount() && out.size()<maxResults; ++p) {
        const QString text = pageText(p);
        int from=0;
        while (out.size()<maxResults) {
            const int at=text.indexOf(q, from, Qt::CaseInsensitive);
            if (at<0) break;
            const int start=qMax(0, at-45), end=qMin(text.size(), at+q.size()+70);
            QString excerpt=text.mid(start,end-start).simplified();
            QVariantMap hit; hit["page"]=p; hit["index"]=at; hit["excerpt"]=excerpt;
            out.push_back(hit); from=at+qMax(1,q.size());
        }
    }
    return out;
}
