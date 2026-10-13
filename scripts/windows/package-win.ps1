# Cree les livrables Windows dans dist/ :
#   AntRenamerQt-<version>-win64-portable.zip  (dossier autonome, antrenamer-qt.ini vide = mode portable)
#   AntRenamerQt-<version>-win64-setup.exe     (installeur Inno Setup 6, si ISCC.exe est trouve)
#
# Usage : powershell -ExecutionPolicy Bypass -File scripts\windows\package-win.ps1 [-SkipBuild]
# Prerequis : MSYS2 (voir vscode-mingw.ps1) ; Inno Setup 6 pour l'installeur (https://jrsoftware.org/isinfo.php).
param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$version = (Get-Content (Join-Path $root "VERSION") -TotalCount 1).Trim()
if ($version -notmatch '^(\d+\.\d+\.\d+)') { throw "VERSION illisible : '$version'" }
$versionNum = "$($Matches[1]).0"   # Inno veut 4 nombres : 3.0.0-rc.1 -> 3.0.0.0

if (-not $SkipBuild) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "vscode-mingw.ps1") build
    if ($LASTEXITCODE -ne 0) { throw "La compilation a echoue." }
}

$app = Join-Path $root "build-mingw\app"
if (-not (Test-Path (Join-Path $app "antrenamer-qt.exe"))) { throw "antrenamer-qt.exe introuvable dans $app (compiler d'abord)." }

$dist = Join-Path $root "dist"
$stage = Join-Path $dist "AntRenamerQt-$version-win64"
if (Test-Path $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null

# Uniquement ce qui sert a l'execution : exe, DLL, plugins Qt, langues, aide (pas les fichiers CMake).
Copy-Item (Join-Path $app "antrenamer-qt.exe") $stage
Get-ChildItem $app -Filter *.dll -File | Copy-Item -Destination $stage
Get-ChildItem $app -Directory | Where-Object { $_.Name -notmatch '^(CMakeFiles|antrenamer-qt_autogen)$' } |
    ForEach-Object { Copy-Item $_.FullName (Join-Path $stage $_.Name) -Recurse }
Copy-Item (Join-Path $root "LICENSE") (Join-Path $stage "license.txt")
Copy-Item (Join-Path $root "README.md") $stage
Copy-Item (Join-Path $root "README_fr.md") $stage

# --- installeur (sans antrenamer-qt.ini : l'installation n'est pas portable) ---
$iscc = @(
    (Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source,
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if ($iscc) {
    & $iscc "/DAppVersion=$version" "/DAppVersionNum=$versionNum" "/DStageDir=$stage" "/O$dist" `
        "/FAntRenamerQt-$version-win64-setup" (Join-Path $root "installer\antrenamer-qt.iss")
    if ($LASTEXITCODE -ne 0) { throw "ISCC a echoue." }
} else {
    Write-Warning "Inno Setup 6 introuvable : installeur non genere (le zip portable l'est quand meme)."
}

# --- zip portable : un antrenamer-qt.ini vide a cote de l'exe active le mode portable ---
New-Item -ItemType File -Path (Join-Path $stage "antrenamer-qt.ini") -Force | Out-Null
$zip = Join-Path $dist "AntRenamerQt-$version-win64-portable.zip"
if (Test-Path $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path $stage -DestinationPath $zip
Write-Host "Livrables dans $dist :"
Get-ChildItem $dist -File | ForEach-Object { Write-Host ("  {0}  ({1:N1} Mo)" -f $_.Name, ($_.Length / 1MB)) }
