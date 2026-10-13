param(
    [ValidateSet("configure", "build", "run", "clean")]
    [string]$Action = "build"
)

$ErrorActionPreference = "Stop"

# Retrouve MSYS2 sans chemin en dur : variable MSYS2_ROOT, outils MinGW deja dans le PATH, registre
# (desinstallation de MSYS2), puis dossier "msys64"/"msys2" a la racine de chaque disque fixe.
function Find-MsysBash {
    $roots = @()
    if ($env:MSYS2_ROOT) { $roots += $env:MSYS2_ROOT }
    foreach ($tool in "g++.exe", "ninja.exe", "cmake.exe") {
        $cmd = Get-Command $tool -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($cmd -and $cmd.Source -match '^(.*)\\(mingw64|ucrt64|clang64|usr)\\bin\\[^\\]+$') { $roots += $Matches[1] }
    }
    foreach ($hive in "HKLM:", "HKCU:") {
        foreach ($sub in "Software\Microsoft\Windows\CurrentVersion\Uninstall", "Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall") {
            $key = Join-Path $hive $sub
            if (Test-Path $key) {
                Get-ChildItem $key -ErrorAction SilentlyContinue | ForEach-Object {
                    $p = Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue
                    if ($p.DisplayName -like "MSYS2*" -and $p.InstallLocation) { $roots += $p.InstallLocation }
                }
            }
        }
    }
    foreach ($drive in [IO.DriveInfo]::GetDrives() | Where-Object { $_.DriveType -eq "Fixed" }) {
        $roots += (Join-Path $drive.RootDirectory.FullName "msys64"), (Join-Path $drive.RootDirectory.FullName "msys2")
    }
    foreach ($root in $roots) {
        $bash = Join-Path $root "usr\bin\bash.exe"
        if (Test-Path $bash) { return $bash }
    }
    throw "MSYS2 introuvable. Installe MSYS2 ou definis MSYS2_ROOT vers son dossier d'installation."
}

function Convert-ToMsysPath {
    param([string]$Path)
    $fullPath = (Resolve-Path $Path).Path
    if ($fullPath -match "^([A-Za-z]):\\(.*)$") {
        $drive = $matches[1].ToLowerInvariant()
        $rest = $matches[2] -replace "\\", "/"
        return "/$drive/$rest"
    }
    return ($fullPath -replace "\\", "/")
}

function Quote-Bash { param([string]$Value) return "'" + ($Value -replace "'", "'\''") + "'" }

function Normalize-PathText { param([string]$Path) return (($Path -replace "\\", "/").TrimEnd("/")).ToLowerInvariant() }

function Reset-StaleBuildDir {
    param([string]$BuildDir, [string]$SourceDir)
    $cachePath = Join-Path $BuildDir "CMakeCache.txt"
    if (-not (Test-Path $cachePath)) { return }
    $homeLine = Select-String -Path $cachePath -Pattern "^CMAKE_HOME_DIRECTORY:INTERNAL=(.*)$" | Select-Object -First 1
    if (-not $homeLine) { return }
    $cachedSource = $homeLine.Matches[0].Groups[1].Value
    if ((Normalize-PathText $cachedSource) -eq (Normalize-PathText $SourceDir)) { return }
    $resolvedBuild = (Resolve-Path $BuildDir).Path
    $resolvedSource = (Resolve-Path $SourceDir).Path
    if (-not ((Normalize-PathText $resolvedBuild).StartsWith((Normalize-PathText $resolvedSource) + "/"))) {
        throw "Refus de supprimer un dossier de build hors du projet: $resolvedBuild"
    }
    Write-Host "Le cache CMake pointe vers '$cachedSource'. Reinitialisation de '$resolvedBuild'."
    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

$bash = Find-MsysBash
# Ce script vit dans scripts/windows/ : la racine du projet est deux niveaux plus haut.
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$buildDir = Join-Path $root "build-mingw"
Reset-StaleBuildDir -BuildDir $buildDir -SourceDir $root

$rootMsys = Convert-ToMsysPath $root
$quotedRoot = Quote-Bash $rootMsys

$env:MSYSTEM = if ($env:MSYSTEM) { $env:MSYSTEM } else { "MINGW64" }
$env:CHERE_INVOKING = "1"
$env:MSYS2_PATH_TYPE = "inherit"

$command = switch ($Action) {
    "configure" { "cd $quotedRoot && cmake --preset mingw" }
    "build" { "cd $quotedRoot && { test -f build-mingw/build.ninja || cmake --preset mingw; } && cmake --build --preset mingw" }
    "run" { "cd $quotedRoot && ./build-mingw/app/antrenamer-qt.exe" }
    "clean" { "cd $quotedRoot && cmake --build --preset mingw --target clean" }
}

& $bash -lc $command
exit $LASTEXITCODE
