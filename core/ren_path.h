#pragma once
// Delphi-style path helpers. On Windows '\\', '/' and ':' all delimit, so behaviour matches the original
// WideExtractFileName & co. whatever separator a caller uses. Elsewhere only '/' delimits: a backslash or a colon is an
// ordinary character of a Linux file name.
#include <QString>

namespace ren {

#ifdef Q_OS_WIN
constexpr QChar kSep = u'\\';  // the separator the helpers add
inline bool isDelim(QChar c) {
    return c == u'\\' || c == u'/' || c == u':';
}
#else
constexpr QChar kSep = u'/';
inline bool isDelim(QChar c) {
    return c == u'/';
}
#endif

inline int lastDelim(const QString& p) {
    for (int i = int(p.size()) - 1; i >= 0; --i) {
        if (isDelim(p[i])) {
            return i;
        }
    }
    return -1;
}
inline QString extractFileName(const QString& p) {
    return p.mid(lastDelim(p) + 1);
}
inline QString extractFilePath(const QString& p) {
    return p.left(lastDelim(p) + 1);
}
inline QString extractFileExt(const QString& p) {
    int dot = int(p.lastIndexOf(u'.'));
    return dot > lastDelim(p) ? p.mid(dot) : QString();
}
inline QString changeFileExt(const QString& p, const QString& ext) {
    int dot = int(p.lastIndexOf(u'.'));
    return (dot > lastDelim(p) ? p.left(dot) : p) + ext;
}
inline QString excludeTrailingBackslash(const QString& p) {
    return (p.endsWith(kSep) || p.endsWith(u'/')) ? p.chopped(1) : p;
}
inline QString includeTrailingBackslash(const QString& p) {
    return (p.endsWith(kSep) || p.endsWith(u'/')) ? p : p + kSep;
}

}  // namespace ren
