#include "updatecheck.h"

#include "ren_update.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

using namespace Qt::StringLiterals;

void reportUpdate(QWidget* parent, bool ok, const QString& version, const QString& url, const QString& error, bool manual) {
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
    } else if (QMessageBox::question(parent, title,
                                     tr("A new version is available: %1 (you have %2). Open the download page?")
                                         .arg(version, QCoreApplication::applicationVersion())) == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl(url));
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
            emit finished(false, {}, {}, reply->errorString());
            return;
        }
        const auto r = ren::newerRelease(reply->readAll(), QCoreApplication::applicationVersion(), includePrereleases);
        emit finished(true, r ? r->version : QString(), r ? r->url : QString(), {});
    });
}
