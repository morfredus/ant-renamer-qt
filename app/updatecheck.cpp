#include "updatecheck.h"

#include "ren_update.h"

#include "lngtranslator.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

using namespace Qt::StringLiterals;

// The release text holds the English notes, then "---", then the French ones: keeps the half of the interface language
// and stops before the list of downloads (the page opened by "Yes" has it).
static QString releaseNotesText(QString body) {
    body.replace(u"\r\n"_s, u"\n"_s);
    const QStringList halves = body.split(u"\n---"_s);
    const QString t = (halves.size() > 1 && helpLanguageCode() == u"fr"_s ? halves[1] : halves[0]).trimmed();
    static const QRegularExpression downloads(uR"(\n###\s+(Downloads|Téléchargements)\b)"_s);
    const auto m = downloads.match(t);
    return m.hasMatch() ? t.left(m.capturedStart()).trimmed() : t;
}

void reportUpdate(QWidget* parent, bool ok, const QString& version, const QString& url, const QString& notes, const QString& error,
                  bool manual) {
    auto tr = [](const char* s) { return QCoreApplication::translate("UpdateCheck", s); };
    const QString title = tr("Update check");
    if (!ok) {
        if (manual) {
            QMessageBox::warning(parent, title, tr("Could not check for updates: %1").arg(error));
        }
    } else if (version.isEmpty()) {
        if (manual) {
            QMessageBox::information(parent, title, tr("This version is up to date."));
        }
    } else {
        QDialog dlg(parent);
        dlg.setWindowTitle(title);
        dlg.resize(560, 460);
        auto* v = new QVBoxLayout(&dlg);
        v->addWidget(new QLabel(tr("A new version is available: %1 (you have %2). Open the download page?")
                                    .arg(version, QCoreApplication::applicationVersion())));
        const QString text = releaseNotesText(notes);
        if (!text.isEmpty()) {
            v->addWidget(new QLabel(tr("What changes:")));
            auto* box = new QTextBrowser;
            box->setOpenExternalLinks(true);
            box->setMarkdown(text);
            v->addWidget(box, 1);
        }
        auto* bb = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);
        QObject::connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        QObject::connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        v->addWidget(bb);
        if (dlg.exec() == QDialog::Accepted) {
            QDesktopServices::openUrl(QUrl(url));
        }
    }
}

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent), net_(new QNetworkAccessManager(this)) {}

void UpdateChecker::check(bool includePrereleases) {
    QNetworkRequest req(QUrl(u"https://api.github.com/repos/%1/releases?per_page=20"_s.arg(QString::fromLatin1(kReleasesRepo))));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setHeader(QNetworkRequest::UserAgentHeader, u"AntRenamerQt/%1"_s.arg(QCoreApplication::applicationVersion()));
    req.setTransferTimeout(10000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = net_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, includePrereleases] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit finished(false, {}, {}, {}, reply->errorString());
            return;
        }
        const auto r = ren::newerRelease(reply->readAll(), QCoreApplication::applicationVersion(), includePrereleases);
        emit finished(true, r ? r->version : QString(), r ? r->url : QString(), r ? r->notes : QString(), {});
    });
}
