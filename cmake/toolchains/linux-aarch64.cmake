# Toolchain CMake : cross-compilation Linux ARM64 (aarch64) depuis un hote x86_64
# (preset "linux-arm64-cross" de CMakePresets.json). Fichier propre au projet.
#
# Il faut un sysroot ARM64 contenant Qt6 et ses dependances (par exemple un Debian arm64
# monte avec debootstrap/qemu). Le build natif "linux-arm64" (directement sur la machine
# ARM64, ex. Raspberry Pi) reste la voie la plus simple.
#
# Variables d'environnement :
#   ARM64_SYSROOT      (OBLIGATOIRE) chemin du sysroot ARM64.
#   ARM64_CROSS_PREFIX prefixe des outils croises. Defaut : "aarch64-linux-gnu-".
#   ARM64_QT_HOST_PATH Qt6 HOTE x86_64 de MEME version que la cible (moc/rcc/uic natifs).
#                      Sans lui, les outils Qt arm64 du sysroot sont executes sous qemu-user
#                      (le preset exporte QEMU_LD_PREFIX=$ARM64_SYSROOT).

if(NOT DEFINED ENV{ARM64_SYSROOT})
    message(FATAL_ERROR
        "ARM64_SYSROOT n'est pas defini. Le cross-build ARM64 exige un sysroot arm64 avec Qt6 ; "
        "exporter ARM64_SYSROOT=<chemin> avant `cmake --preset linux-arm64-cross`.")
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(DEFINED ENV{ARM64_CROSS_PREFIX})
    set(_prefix "$ENV{ARM64_CROSS_PREFIX}")
else()
    set(_prefix "aarch64-linux-gnu-")
endif()
set(CMAKE_C_COMPILER   "${_prefix}gcc")
set(CMAKE_CXX_COMPILER "${_prefix}g++")

set(CMAKE_SYSROOT        "$ENV{ARM64_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH "$ENV{ARM64_SYSROOT}")
set(CMAKE_LIBRARY_ARCHITECTURE aarch64-linux-gnu)  # multiarch Debian

# Programmes (compilateur, moc...) cherches sur l'hote ; bibliotheques, en-tetes et paquets
# uniquement dans le sysroot.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

if(DEFINED ENV{ARM64_QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{ARM64_QT_HOST_PATH}" CACHE PATH "Qt6 hote (outils moc/rcc/uic)")
endif()

set(ENV{PKG_CONFIG_SYSROOT_DIR} "$ENV{ARM64_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
    "$ENV{ARM64_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:$ENV{ARM64_SYSROOT}/usr/share/pkgconfig")
