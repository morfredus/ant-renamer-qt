#pragma once
// New-version check, the part that needs no network: comparing versions and reading the list of GitHub releases.
#include <QByteArray>
#include <QString>

#include <optional>

namespace ren {

// "3.0.0-rc.15", "v3.0.0": negative, 0 or positive like a comparison. A final release is newer than its candidates,
// and rc.9 is older than rc.10.
int compareVersions(const QString& a, const QString& b);

struct Release {
    QString version;  // as tagged, without the leading "v"
    QString url;      // page of the release
    bool prerelease = false;
};

// `json` is the answer of GET /repos/<owner>/<repo>/releases. Returns the newest published release that is newer than
// `current`. Candidates (beta, rc) are only considered when `current` is one itself or when `includePrereleases` is set
// (the user asked for them): by default a user of a final version is not told about a candidate. Drafts and addresses
// outside https://github.com/ are ignored.
std::optional<Release> newerRelease(const QByteArray& json, const QString& current, bool includePrereleases = false);

}  // namespace ren
