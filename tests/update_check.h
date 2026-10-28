// New-version check: version order and the choice of the release to announce.
#include "ren_update.h"

static void updateChecks() {
    using ren::compareVersions;
    auto sign = [](int v) { return v < 0 ? u"<"_s : v > 0 ? u">"_s : u"="_s; };
    CHECK_EQ(sign(compareVersions(u"3.0.0-rc.15"_s, u"3.0.0-rc.14"_s)), u">"_s);
    CHECK_EQ(sign(compareVersions(u"3.0.0-rc.9"_s, u"3.0.0-rc.10"_s)), u"<"_s);  // numbers, not text
    CHECK_EQ(sign(compareVersions(u"3.0.0"_s, u"3.0.0-rc.15"_s)), u">"_s);
    CHECK_EQ(sign(compareVersions(u"v3.0.1"_s, u"3.0.0"_s)), u">"_s);
    CHECK_EQ(sign(compareVersions(u"3.0"_s, u"3.0.0"_s)), u"="_s);
    CHECK_EQ(sign(compareVersions(u"3.10.0"_s, u"3.9.0"_s)), u">"_s);
    CHECK_EQ(sign(compareVersions(u"3.0.0-rc.1"_s, u"3.0.0-rc.1.1"_s)), u"<"_s);

    const QByteArray list = R"([
      {"tag_name":"v3.0.0-rc.16","html_url":"https://github.com/o/r/releases/tag/v3.0.0-rc.16","prerelease":true,"draft":false},
      {"tag_name":"v3.0.0-rc.15","html_url":"https://github.com/o/r/releases/tag/v3.0.0-rc.15","prerelease":true,"draft":false},
      {"tag_name":"v3.0.0-rc.99","html_url":"https://github.com/o/r/releases/tag/v3.0.0-rc.99","prerelease":true,"draft":true},
      {"tag_name":"v2.13.0","html_url":"https://github.com/o/r/releases/tag/v2.13.0","prerelease":false,"draft":false}])";
    auto pick = [&](const QString& current) {
        const auto r = ren::newerRelease(list, current);
        return r ? r->version : u"none"_s;
    };
    CHECK_EQ(pick(u"3.0.0-rc.15"_s), u"3.0.0-rc.16"_s);  // a candidate user is told about the next candidate
    CHECK_EQ(pick(u"3.0.0-rc.16"_s), u"none"_s);         // up to date (the draft rc.99 is ignored)
    CHECK_EQ(pick(u"2.13.0"_s), u"none"_s);              // a final version is not told about candidates
    CHECK_EQ(pick(u"2.12.0"_s), u"2.13.0"_s);
    CHECK_EQ(ren::newerRelease(list, u"2.13.0"_s, true) ? ren::newerRelease(list, u"2.13.0"_s, true)->version : u"none"_s,
             u"3.0.0-rc.16"_s);  // asked for candidates
    CHECK_EQ(ren::newerRelease(list, u"3.0.0-rc.16"_s, true) ? u"found"_s : u"none"_s,
             u"none"_s);  // still nothing newer than rc.16 (draft ignored)
    CHECK_EQ(ren::newerRelease("not json", u"1.0"_s) ? u"found"_s : u"none"_s, u"none"_s);
    const QByteArray foreign = R"([{"tag_name":"v9.0.0","html_url":"https://evil.example/x","prerelease":false,"draft":false}])";
    CHECK_EQ(ren::newerRelease(foreign, u"1.0"_s) ? u"found"_s : u"none"_s, u"none"_s);  // only github.com addresses

    // the real list: the candidates are published as ordinary releases (prerelease false), so the tag decides
    const QByteArray real = R"([
      {"tag_name":"v3.0.0-rc.15","html_url":"https://github.com/morfredus/ant-renamer-qt/releases/tag/v3.0.0-rc.15","prerelease":false,"draft":false},
      {"tag_name":"v3.0.0-rc.12","html_url":"https://github.com/morfredus/ant-renamer-qt/releases/tag/v3.0.0-rc.12","prerelease":false,"draft":false}])";
    auto pickReal = [&](const QString& current) {
        const auto r = ren::newerRelease(real, current);
        return r ? r->version : u"none"_s;
    };
    CHECK_EQ(pickReal(u"3.0.0-rc.12"_s), u"3.0.0-rc.15"_s);
    CHECK_EQ(pickReal(u"3.0.0-rc.15"_s), u"none"_s);
    CHECK_EQ(pickReal(u"2.13.0"_s), u"none"_s);  // a final version is not told about candidates by default
    CHECK_EQ(ren::newerRelease(real, u"2.13.0"_s, true)->version, u"3.0.0-rc.15"_s);  // ...unless it asked for them
}
