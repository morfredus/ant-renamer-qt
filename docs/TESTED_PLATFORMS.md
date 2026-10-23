# Tested platforms

Where Ant Renamer Qt has actually been built and run. "Tested" means a person ran the program or the package on that system; a platform that is not listed has not been tested, even if it should work. ([Version française](TESTED_PLATFORMS_fr.md).)

If you try another system, please report the result in the [discussions](https://github.com/morfredus/ant-renamer-qt/discussions) (works or not, with the version and the system).

| System | Architecture | Version | What was tested |
|---|---|---|---|
| Debian GNU/Linux 13 (trixie), Raspberry Pi 4 Model B (two machines) | arm64 | 3.0.0-rc.11, rc.12, rc.13, rc.14, rc.15, rc.16, 3.0.0 | rc.11: `.deb` installed from the package (dependencies resolved by apt), menu entry present, program works, French interface. rc.12: arm64 `.deb` installed from the package, program works, Linux file names, help pages and links, icon sets and legibility. rc.13, rc.14: installed from the packages, program works; rc.14: light / dark switch, readable log in the dark theme rc.15: `.deb` installed from the package; renaming of files, pictures, sidecar files and folders, light / dark switch, readable log and tool tips in the dark theme. rc.16: the new features, on this machine too: hidden and system files, and the renaming history (Undo pass after pass, History window, Undo button option, French interface). 3.0.0: offered by the new-version check, installed from the published package, program works. |
| Linux Mint 22.3 Xfce (Ubuntu 24.04 base) | x86-64 | 3.0.0-rc.11, rc.12, rc.13, rc.14, rc.15, rc.16, 3.0.0 | rc.11: `.deb` installed from the package, program works. rc.12: `.deb` installed from the package; Linux file names (`:`, `/`, folder rename, `%folder-N%`, `%count%`), help pages and links, icon sets and legibility. rc.13, rc.14: installed from the `.deb`, program works; rc.14: AppImage runs and works, light / dark switch (AppImage included), readable log in the dark theme rc.15: `.deb` and AppImage installed and run; renaming of files, pictures, sidecar files and folders, light / dark switch (AppImage included). rc.16: the new features, on this machine too: hidden and system files, and the renaming history (Undo pass after pass, History window, Undo button option, French interface). 3.0.0: offered by the new-version check, `.deb` and AppImage installed from the published files, program works. |
| Ubuntu 24.04 (WSL2 on Windows 11) | x86-64 | 3.0.0-rc.10, rc.11 | build, unit tests, `.deb` and AppImage built; program started, help displayed; dependency list of the `.deb` checked (`ldd`: no missing library) |
| Windows 11 Home | x86-64 | 3.0.0-rc.10, rc.12, rc.13, rc.14, rc.15, rc.16, 3.0.0 | rc.10: MinGW build, unit tests, installer and portable zip. rc.12: tested from the Windows package; program, help pages and links, icon sets and legibility, `%count%`. rc.13, rc.14: installer and portable zip, program works; rc.14: light / dark switch, readable log in the dark theme rc.15: installer and portable zip; renaming of files, pictures, sidecar files and folders, light / dark switch. rc.16: the new features, on this machine too: hidden and system files, and the renaming history (Undo pass after pass, History window, Undo button option, French interface). 3.0.0: offered by the new-version check, installed from the published package, program works. |

## Not tested yet

- Debian 13 on x86-64 (the dependency fix of rc.11 is the same as on arm64, but the amd64 package has not been installed there).
- Any other distribution with the AppImage (it was tested on Linux Mint only), Windows 10, macOS (not supported).

Update this file with each release: version, system, what was checked.
