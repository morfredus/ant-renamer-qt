# Security policy

([Version française](SECURITY_fr.md))

## Supported versions

Only the latest published version of Ant Renamer Qt receives fixes. If you find a problem in an older one, please check that it is still there in the latest version first.

## Reporting a vulnerability

Please do **not** open a public issue or discussion for a security problem.

Use the private reporting of GitHub instead: on the [Security tab of the repository](https://github.com/morfredus/ant-renamer-qt/security), click *Report a vulnerability*. Only the maintainer sees the report.

Please include the version, the system, what you did and what happened (a file that is renamed, moved or read when it should not be, a crash on a crafted file name or `.lng` file, and so on).

You will get an answer as soon as possible; this is a spare-time project maintained by one person, so allow a few days. Once the problem is fixed and published, the report can be made public with credit to you, unless you prefer to stay anonymous.

## What is not a vulnerability

The program renames and moves files on purpose, with the rights of the user who runs it. Renaming a file you asked it to rename is not a security problem; a bug that loses data is a normal [issue](https://github.com/morfredus/ant-renamer-qt/issues).
