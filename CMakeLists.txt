cmake_minimum_required(VERSION 3.21)

# The version is read from the VERSION file at the root (single source).
set(AR_VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${AR_VERSION_FILE}")
file(STRINGS "${AR_VERSION_FILE}" AR_VERSION LIMIT_COUNT 1)
string(STRIP "${AR_VERSION}" AR_VERSION)

# "3.0.0-rc.1": project(VERSION) only accepts numbers, so the full string stays in AR_VERSION.
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" AR_VERSION_NUM "${AR_VERSION}")
project(AntRenamerQt VERSION "${AR_VERSION_NUM}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Compiler warnings are on for the whole project: the code must build without any.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wshadow)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

# Core: renaming, actions, formats. Depends on QtCore only (no user interface).
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
