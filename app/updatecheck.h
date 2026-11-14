#pragma once
#include <QFile>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QWidget;

// Asks GitHub for the published releases and tells whether a newer one exists. Asynchronous, times out after 10 s, silent
// on failure at start-up (retried next start). Sends nothing about the user or files: a plain HTTPS request.
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QObject* parent = nullptr);
    void check(bool includePrereleases = false);  // true: also announce beta and rc
signals:
    // ok = the answer was read; version empty = nothing newer. error is set when ok is false.
    void finished(bool ok, const QString& version, const QString& url, const QString& notes, const QString& error);

private:
    QNetworkAccessManager* net_;
};

// Tells the user what the check found, with the release text. `manual`: also report "up to date" and errors.
void reportUpdate(QWidget* parent, bool ok, const QString& version, const QString& url, const QString& notes, const QString& error,
                  bool manual);

// Flatpak: the store updates the program, so the check is neither run nor offered.
inline bool runsInFlatpak() {
    return QFile::exists(QStringLiteral("/.flatpak-info"));
}

// Repository the check reads.
inline const char* kReleasesRepo = "morfredus/ant-renamer-qt";
