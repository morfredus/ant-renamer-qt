cmake_minimum_required(VERSION 3.21)

set(AR_VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${AR_VERSION_FILE}")
file(STRINGS "${AR_VERSION_FILE}" AR_VERSION LIMIT_COUNT 1)
string(STRIP "${AR_VERSION}" AR_VERSION)

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" AR_VERSION_NUM "${AR_VERSION}")
project(AntRenamerQt VERSION "${AR_VERSION_NUM}" LANGUAGES CXX)
include(GNUInstallDirs)
set(ANTRENAMER_DESKTOP_ID "antrenamer-qt" CACHE STRING
    "Desktop file and icon identifier")

option(ANTREN_INSTALL_DESKTOP_ID
    "Use a custom desktop file and icon name"
    "antrenamer-qt"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wshadow)
    # The CI sets this so that a warning fails the build (-DANTREN_WERROR=ON); off by default for people with a newer compiler.
    option(ANTREN_WERROR "Treat compiler warnings as errors" OFF)
    if(ANTREN_WERROR)
        add_compile_options(-Werror)
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(rencore STATIC
    core/ren_actions.cpp core/ren_exif.cpp core/ren_files.cpp core/ren_format.cpp core/ren_id3.cpp core/ren_update.cpp)
target_include_directories(rencore PUBLIC core)
target_link_libraries(rencore PUBLIC Qt6::Core)
if(MSVC)
    target_compile_options(rencore PRIVATE /utf-8)
endif()

enable_testing()
add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE rencore)
add_test(NAME core COMMAND test_core)

add_subdirectory(app)
