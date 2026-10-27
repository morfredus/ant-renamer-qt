#include "ren_update.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

using namespace Qt::StringLiterals;

namespace ren {

namespace {

QString clean(QString v) {
    v = v.trimmed();
    if (v.startsWith(u'v') || v.startsWith(u'V')) {
        v.remove(0, 1);
    }
    return v;
}

bool isPre(const QString& v) {
    return clean(v).contains(u'-');
}

int cmpInt(qlonglong a, qlonglong b) {
    return a < b ? -1 : a > b ? 1 : 0;
}

}  // namespace

int compareVersions(const QString& a, const QString& b) {
    const QString x = clean(a), y = clean(b);
    const QStringList xp = x.section(u'-', 0, 0).split(u'.'), yp = y.section(u'-', 0, 0).split(u'.');
    for (qsizetype i = 0; i < qMax(xp.size(), yp.size()); ++i) {
        const int c = cmpInt(i < xp.size() ? xp[i].toLongLong() : 0, i < yp.size() ? yp[i].toLongLong() : 0);
        if (c) {
            return c;
        }
    }
    const bool xpre = x.contains(u'-'), ypre = y.contains(u'-');
    if (xpre != ypre) {
        return xpre ? -1 : 1;  // 3.0.0 is newer than 3.0.0-rc.15
    }
    if (!xpre) {
        return 0;
    }
    const QStringList xi = x.section(u'-', 1).split(u'.'), yi = y.section(u'-', 1).split(u'.');
    for (qsizetype i = 0; i < qMax(xi.size(), yi.size()); ++i) {
        if (i >= xi.size()) {
            return -1;  // fewer identifiers: older
        }
        if (i >= yi.size()) {
            return 1;
        }
        bool xn = false, yn = false;
        const qlonglong xv = xi[i].toLongLong(&xn), yv = yi[i].toLongLong(&yn);
        int c = 0;
        if (xn && yn) {
            c = cmpInt(xv, yv);
        } else if (xn != yn) {
            c = xn ? -1 : 1;  // a number is older than a word
        } else {
            c = xi[i].compare(yi[i]);
        }
        if (c) {
            return c < 0 ? -1 : 1;
        }
    }
    return 0;
}

std::optional<Release> newerRelease(const QByteArray& json, const QString& current, bool includePrereleases) {
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) {
        return std::nullopt;
    }
    const bool useCandidates = includePrereleases || isPre(current);
    std::optional<Release> best;
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        if (o.value(u"draft"_s).toBool()) {
            continue;
        }
        const bool pre = o.value(u"prerelease"_s).toBool();
        const QString tag = clean(o.value(u"tag_name"_s).toString());
        const QString url = o.value(u"html_url"_s).toString();
        if (tag.isEmpty() || !url.startsWith(u"https://github.com/")) {
            continue;
        }
        if ((pre || isPre(tag)) && !useCandidates) {
            continue;
        }
        if (compareVersions(tag, current) <= 0) {
            continue;
        }
        if (!best || compareVersions(tag, best->version) > 0) {
            best = Release{tag, url, pre || isPre(tag)};
        }
    }
    return best;
}

}  // namespace ren
