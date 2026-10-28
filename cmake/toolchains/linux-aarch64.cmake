# CMake toolchain: cross-compiling for Linux ARM64 (aarch64) from an x86_64 host
# (preset "linux-arm64-cross" of CMakePresets.json). Specific to this project.
#
# It needs an ARM64 sysroot with Qt6 and its dependencies (for example a Debian arm64
# mounted with debootstrap/qemu). The native "linux-arm64" build (directly on the ARM64
# machine, e.g. a Raspberry Pi) remains the simplest way.
#
# Environment variables:
#   ARM64_SYSROOT      (REQUIRED) path of the ARM64 sysroot.
#   ARM64_CROSS_PREFIX prefix of the cross tools. Default: "aarch64-linux-gnu-".
#   ARM64_QT_HOST_PATH x86_64 HOST Qt6 of the SAME version as the target (native moc/rcc/uic).
#                      Without it, the arm64 Qt tools of the sysroot run under qemu-user
#                      (the preset exports QEMU_LD_PREFIX=$ARM64_SYSROOT).

if(NOT DEFINED ENV{ARM64_SYSROOT})
    message(FATAL_ERROR
        "ARM64_SYSROOT is not set. The ARM64 cross build needs an arm64 sysroot with Qt6; "
        "export ARM64_SYSROOT=<path> before `cmake --preset linux-arm64-cross`.")
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
set(CMAKE_LIBRARY_ARCHITECTURE aarch64-linux-gnu)  # Debian multiarch

# Programs (compiler, moc...) are looked up on the host; libraries, headers and packages
# only in the sysroot.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

if(DEFINED ENV{ARM64_QT_HOST_PATH})
    set(QT_HOST_PATH "$ENV{ARM64_QT_HOST_PATH}" CACHE PATH "Host Qt6 (moc/rcc/uic tools)")
endif()

set(ENV{PKG_CONFIG_SYSROOT_DIR} "$ENV{ARM64_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
    "$ENV{ARM64_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:$ENV{ARM64_SYSROOT}/usr/share/pkgconfig")
