# Code signing policy

French version: [CODE_SIGNING_POLICY_fr.md](CODE_SIGNING_POLICY_fr.md).

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

This page describes how the Windows files of Ant Renamer Qt are signed, who can approve a signature, and what the program does with your data. It applies to the Windows files published once signing is in place. The files published before that are not signed; the release notes of the first signed release say so.

## What is signed

- the Windows installer (`AntRenamerQt-<version>-win64-setup.exe`);
- the program (`antrenamer-qt.exe`) inside the installer and the portable zip.

Only files built from this repository are signed. The Qt and MinGW libraries that come with the program are those of the upstream projects and are not signed by this project. The Linux files (`.deb`, AppImage) are not signed with this certificate: they come with a GPG signature, see the [README](../README.md).

## How a file gets signed

1. The files are built from the source code of a tagged version by the GitHub Actions workflow of the public repository ([`.github/workflows/build.yml`](../.github/workflows/build.yml)). Nothing is built on a personal computer for signing.
2. The workflow sends the build to SignPath, which signs it only if it comes from that workflow and repository.
3. A maintainer approves each signature request by hand, after checking that the version, the tag and the build are the expected ones.

The product name in the files is "Ant Renamer Qt" and the product version is the version of the release, the same for every file of that release.

## Roles

The project has one maintainer, who holds the three roles. Two-factor authentication is required on GitHub and on SignPath.

| Role | Who | What it means |
|---|---|---|
| Author | [morfredus](https://github.com/morfredus) (Frédéric Biron) | Writes and commits the code. |
| Reviewer | [morfredus](https://github.com/morfredus) | Reads every change that comes from someone else (pull request) before it is merged. |
| Approver | [morfredus](https://github.com/morfredus) | Approves each signature request. |

## Privacy

Ant Renamer Qt does not send information to other systems, except when the user asks for it. The only network function is the optional check for a new version, off by default: when you turn it on, the program asks the GitHub API once a day for the latest release and sends nothing but its own version number. No file name, no path and no personal data leaves your computer. The renaming history and the settings stay in files on your computer.

## Checking a signature

On Windows, right-click the file, choose Properties, then the Digital Signatures tab. The signer must be "SignPath Foundation".

## Questions

[Issues](https://github.com/morfredus/ant-renamer-qt/issues) on GitHub, or by e-mail: [ant-renamer-qt@morfredus.fr](mailto:ant-renamer-qt@morfredus.fr).
