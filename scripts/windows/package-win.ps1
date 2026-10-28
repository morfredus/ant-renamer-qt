# Builds the Windows packages in dist/:
#   AntRenamerQt-<version>-win64-portable.zip  (self-contained folder, empty antrenamer-qt.ini = portable mode)
#   AntRenamerQt-<version>-win64-setup.exe     (Inno Setup 6 installer, if ISCC.exe is found)
#
# Usage: powershell -ExecutionPolicy Bypass -File scripts\windows\package-win.ps1 [-SkipBuild]
# Requirements: MSYS2 (see vscode-mingw.ps1); Inno Setup 6 for the installer (https://jrsoftware.org/isinfo.php).
param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$version = (Get-Content (Join-Path $root "VERSION") -TotalCount 1).Trim()
if ($version -notmatch '^(\d+\.\d+\.\d+)') { throw "cannot read VERSION: '$version'" }
$versionNum = "$($Matches[1]).0"   # Inno wants 4 numbers: 3.0.0-rc.1 -> 3.0.0.0

if (-not $SkipBuild) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "vscode-mingw.ps1") build
    if ($LASTEXITCODE -ne 0) { throw "The build failed." }
}

$app = Join-Path $root "build-mingw\app"
if (-not (Test-Path (Join-Path $app "antrenamer-qt.exe"))) { throw "antrenamer-qt.exe not found in $app (build first)." }

$dist = Join-Path $root "dist"
$stage = Join-Path $dist "AntRenamerQt-$version-win64"
if (Test-Path $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null

# Only what is needed at run time: exe, DLLs, Qt plugins, languages, help (not the CMake files nor the *_autogen folders, test programs included).
Copy-Item (Join-Path $app "antrenamer-qt.exe") $stage
Get-ChildItem $app -Filter *.dll -File | Copy-Item -Destination $stage
Get-ChildItem $app -Directory | Where-Object { $_.Name -notmatch '^(CMakeFiles|.*_autogen)$' } |
    ForEach-Object { Copy-Item $_.FullName (Join-Path $stage $_.Name) -Recurse }
Copy-Item (Join-Path $root "LICENSE") (Join-Path $stage "license.txt")
Copy-Item (Join-Path $root "README.md") $stage
Copy-Item (Join-Path $root "README_fr.md") $stage

# --- installer (without antrenamer-qt.ini: an installation is not portable) ---
$iscc = @(
    (Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source,
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if ($iscc) {
    & $iscc "/DAppVersion=$version" "/DAppVersionNum=$versionNum" "/DStageDir=$stage" "/O$dist" `
        "/FAntRenamerQt-$version-win64-setup" (Join-Path $root "installer\antrenamer-qt.iss")
    if ($LASTEXITCODE -ne 0) { throw "ISCC failed." }
} else {
    Write-Warning "Inno Setup 6 not found: installer not built (the portable zip still is)."
}

# --- portable zip: an empty antrenamer-qt.ini next to the exe turns portable mode on ---
New-Item -ItemType File -Path (Join-Path $stage "antrenamer-qt.ini") -Force | Out-Null
$zip = Join-Path $dist "AntRenamerQt-$version-win64-portable.zip"
if (Test-Path $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path $stage -DestinationPath $zip
Write-Host "Packages in ${dist}:"
Get-ChildItem $dist -File | ForEach-Object { Write-Host ("  {0}  ({1:N1} MB)" -f $_.Name, ($_.Length / 1MB)) }
