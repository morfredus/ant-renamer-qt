find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
set(CMAKE_AUTORCC ON)

set(SRC main.cpp mainwindow.cpp mainwindow_files.cpp mainwindow_batch.cpp mainwindow_run.cpp devaids.cpp actionpanel.cpp appsettings.cpp dialogs.cpp icons.cpp lngtranslator.cpp helpwindow.cpp shellmenu.cpp theme.cpp updatecheck.cpp resources.qrc)
if(WIN32)
    enable_language(RC)
    # app.rc: icon, manifest (long paths, UTF-8, per-monitor DPI) and version information, generated from app.rc.in so that
    # the version comes from the VERSION file (name "3.1.1-beta.1", number 3,1,1,0, "pre-release" flag when there is a suffix).
    string(REPLACE "." "," AR_RC_VERSION "${AR_VERSION_NUM}.0")
    if(AR_VERSION MATCHES "-")
        set(AR_RC_FLAGS "VS_FF_PRERELEASE")
    else()
        set(AR_RC_FLAGS "0")
    endif()
    configure_file(app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    list(APPEND SRC "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/app.rc" PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/renamer.ico;${CMAKE_CURRENT_SOURCE_DIR}/app.manifest")  # the .rc does not #include them: without this, changing the icon would not rebuild the exe
endif()

add_executable(antrenamer-qt WIN32 ${SRC})
target_link_libraries(antrenamer-qt PRIVATE rencore Qt6::Widgets Qt6::Network)
target_compile_definitions(antrenamer-qt PRIVATE AR_APP_VERSION="${AR_VERSION}")
# Commit of the build, shown next to a pre-release version (window title, About box). Read when CMake runs; a commit
# rewrites .git/logs/HEAD, which makes CMake run again. No git, no commit shown.
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/.git/logs/HEAD")
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE AR_BUILD_ID OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=no WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE AR_BUILD_DIRTY OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(AR_BUILD_ID)
        if(AR_BUILD_DIRTY)
            string(APPEND AR_BUILD_ID "+")  # built from modified files
        endif()
        target_compile_definitions(antrenamer-qt PRIVATE AR_BUILD_ID="${AR_BUILD_ID}")
    endif()
endif()
# Linux: file drops in a Flatpak (no --filesystem) go through the document portal, over D-Bus.
if(UNIX AND NOT APPLE)
    find_package(Qt6 QUIET COMPONENTS DBus)
    if(Qt6DBus_FOUND)
        target_link_libraries(antrenamer-qt PRIVATE Qt6::DBus)
        target_compile_definitions(antrenamer-qt PRIVATE AR_HAVE_DBUS)
    endif()
endif()
if(MSVC)
    target_compile_options(antrenamer-qt PRIVATE /utf-8)
endif()

add_executable(test_panel ../tests/test_panel.cpp actionpanel.cpp appsettings.cpp icons.cpp lngtranslator.cpp helpwindow.cpp)
target_include_directories(test_panel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(test_panel PRIVATE LNG_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Languages")
target_link_libraries(test_panel PRIVATE rencore Qt6::Widgets Qt6::Network)
add_test(NAME panel COMMAND test_panel)

add_custom_target(antrenamer-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Languages $<TARGET_FILE_DIR:antrenamer-qt>/Languages
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:antrenamer-qt>/help
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/help $<TARGET_FILE_DIR:antrenamer-qt>/help
    COMMENT "Copying the language files and the help next to the executable")

# Windows deployment
if(WIN32)
    find_program(WINDEPLOYQT_EXE windeployqt
        HINTS "${Qt6_DIR}/../../../bin" "$ENV{QT_ROOT}/bin")
    if(WINDEPLOYQT_EXE)
        add_custom_command(TARGET antrenamer-qt POST_BUILD
            COMMAND "${WINDEPLOYQT_EXE}" --no-translations --compiler-runtime "$<TARGET_FILE:antrenamer-qt>"
            COMMENT "Copying the Qt DLLs next to the executable")
    endif()
    if(NOT MSVC)
        function(antrenamer_msys_path windows_path output)
            string(REGEX REPLACE "^([A-Za-z]):/" "/\\1/" converted "${windows_path}")
            string(SUBSTRING "${converted}" 1 1 drive)
            string(TOLOWER "${drive}" drive)
            string(SUBSTRING "${converted}" 2 -1 tail)
            set(${output} "/${drive}${tail}" PARENT_SCOPE)
        endfunction()
        antrenamer_msys_path("${CMAKE_SOURCE_DIR}/scripts/windows/deploy-mingw.sh" ANTRENAMER_DEPLOY_SCRIPT)
        antrenamer_msys_path("${CMAKE_CURRENT_BINARY_DIR}" ANTRENAMER_TARGET_DIR)
        set(_antrenamer_cxx "${CMAKE_CXX_COMPILER}")
        if(NOT EXISTS "${_antrenamer_cxx}")
            find_program(_antrenamer_cxx NAMES "${CMAKE_CXX_COMPILER}" g++.exe)
        endif()
        get_filename_component(ANTRENAMER_COMPILER_BIN "${_antrenamer_cxx}" DIRECTORY)
        get_filename_component(ANTRENAMER_MINGW_ROOT "${ANTRENAMER_COMPILER_BIN}" DIRECTORY)
        get_filename_component(ANTRENAMER_MSYS_ROOT "${ANTRENAMER_MINGW_ROOT}" DIRECTORY)
        find_program(ANTRENAMER_MSYS_BASH bash.exe
            HINTS "${ANTRENAMER_MSYS_ROOT}/usr/bin"
            NO_DEFAULT_PATH)
        if(ANTRENAMER_MSYS_BASH)
            add_custom_command(TARGET antrenamer-qt POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E env
                    "PATH=${ANTRENAMER_MSYS_ROOT}/usr/bin;${ANTRENAMER_COMPILER_BIN}"
                    --
                    "${ANTRENAMER_MSYS_BASH}" "${ANTRENAMER_DEPLOY_SCRIPT}" "${ANTRENAMER_TARGET_DIR}"
                VERBATIM
                COMMENT "Copying the MinGW DLLs next to the executable")
        else()
            message(WARNING "MSYS2 Bash not found; non-Qt MinGW DLL collection is skipped")
        endif()
    endif()
endif()
# Linux
if(UNIX AND NOT APPLE)

    # Main executable
    install(TARGETS antrenamer-qt
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Language files and help
    install(DIRECTORY Languages help
        DESTINATION ${CMAKE_INSTALL_DATADIR}/antrenamer-qt
    )

    # Desktop entry
    configure_file(
        "${CMAKE_SOURCE_DIR}/scripts/linux/antrenamer-qt.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${ANTRENAMER_DESKTOP_ID}.desktop"
        @ONLY
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${ANTRENAMER_DESKTOP_ID}.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    # AppStream metadata
    set(ANTRENAMER_METAINFO_FILE
        "${CMAKE_CURRENT_BINARY_DIR}/fr.morfredus.AntRenamerQt.metainfo.xml"
    )

    configure_file(
        "${CMAKE_SOURCE_DIR}/packaging/flatpak/fr.morfredus.AntRenamerQt.metainfo.xml.in"
        "${ANTRENAMER_METAINFO_FILE}"
        @ONLY
    )

    install(
        FILES "${ANTRENAMER_METAINFO_FILE}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
    )

    # Application icons
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor/scalable/apps/antrenamer-qt.svg"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME "${ANTRENAMER_DESKTOP_ID}.svg"
    )

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/antrenamer-qt.png"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME "${ANTRENAMER_DESKTOP_ID}.png"
    )

endif()
