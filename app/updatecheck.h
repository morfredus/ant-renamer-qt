#pragma once
#include <QFile>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QWidget;

// The program must never depend on this: the request is asynchronous (the interface keeps running), stops by itself after
// 10 seconds, and a failure at start-up is silent and tried again at the next start. If GitHub cannot be reached, the
// program works exactly as without the check.
//
// Asks GitHub for the list of published releases and tells whether a newer one exists. Read only: nothing about the
// user or the files is sent (an ordinary HTTPS request with the program name and version as user agent).
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QObject* parent = nullptr);
    void check();
signals:
    // ok = the answer was read; version empty = nothing newer. error is set when ok is false.
    void finished(bool ok, const QString& version, const QString& url, const QString& error);

private:
    QNetworkAccessManager* net_;
};

// Tells the user what the check found. `manual`: also say "up to date" and report errors (a check that nobody asked for
// only speaks when there is something new). Offers to open the page of the new release.
void reportUpdate(QWidget* parent, bool ok, const QString& version, const QString& url, const QString& error, bool manual);

// Under Flatpak the store updates the program: the check is neither run nor offered (its message would mislead).
inline bool runsInFlatpak() { return QFile::exists(QStringLiteral("/.flatpak-info")); }

// Address of the project page for the releases (also the repository the check reads).
inline const char* kReleasesRepo = "morfredus/ant-renamer-qt";
