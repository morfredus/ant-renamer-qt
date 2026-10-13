#include "shellmenu.h"

#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <string>

namespace {

const wchar_t* const kTargets[] = {L"Software\\Classes\\*\\shell\\AntRenamerQt",
                                   L"Software\\Classes\\Directory\\shell\\AntRenamerQt"};

bool setString(HKEY key, const wchar_t* name, const QString& value) {
    const std::wstring v = value.toStdWString();
    return RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(v.c_str()),
                          DWORD((v.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

}  // namespace

bool shellMenuInstalled() {
    HKEY k;
    const bool ok = RegOpenKeyExW(HKEY_CURRENT_USER, kTargets[0], 0, KEY_READ, &k) == ERROR_SUCCESS;
    if (ok) RegCloseKey(k);
    return ok;
}

bool setShellMenu(bool on, const QString& label) {
    bool ok = true;
    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    for (const wchar_t* path : kTargets) {
        if (!on) {
            const LONG r = RegDeleteTreeW(HKEY_CURRENT_USER, path);
            ok = ok && (r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND);
            continue;
        }
        HKEY k = nullptr;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) {
            ok = false;
            continue;
        }
        ok = setString(k, nullptr, label) && setString(k, L"Icon", QStringLiteral("\"") + exe + QStringLiteral("\",0")) && ok;
        RegCloseKey(k);
        const std::wstring cmdPath = std::wstring(path) + L"\\command";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, cmdPath.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) {
            ok = false;
            continue;
        }
        ok = setString(k, nullptr, QStringLiteral("\"") + exe + QStringLiteral("\" --shell \"%1\"")) && ok;
        RegCloseKey(k);
    }
    return ok;
}

#else

bool shellMenuInstalled() { return false; }
bool setShellMenu(bool, const QString&) { return false; }

#endif
