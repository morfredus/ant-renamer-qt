find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
set(CMAKE_AUTORCC ON)

set(SRC main.cpp mainwindow.cpp mainwindow_files.cpp mainwindow_batch.cpp mainwindow_run.cpp devaids.cpp actionpanel.cpp appsettings.cpp dialogs.cpp icons.cpp lngtranslator.cpp helpwindow.cpp shellmenu.cpp theme.cpp updatecheck.cpp resources.qrc)
if(WIN32)
    enable_language(RC)
    list(APPEND SRC app.rc)   # icon + manifest (long paths, UTF-8, per-monitor DPI)
    set_source_files_properties(app.rc PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/renamer.ico;${CMAKE_CURRENT_SOURCE_DIR}/app.manifest")  # the .rc does not #include them: without this, changing the icon would not rebuild the exe
endif()

add_executable(antrenamer-qt WIN32 ${SRC})
target_link_libraries(antrenamer-qt PRIVATE rencore Qt6::Widgets Qt6::Network)
target_compile_definitions(antrenamer-qt PRIVATE AR_APP_VERSION="${AR_VERSION}")
if(MSVC)
    target_compile_options(antrenamer-qt PRIVATE /utf-8)
endif()

# Round-trip test of the action panel (load an action into the editor, read it back), without a window.
# Fails if a field or an action type is added without updating ActionPanel::setFromAction.
add_executable(test_panel ../tests/test_panel.cpp actionpanel.cpp appsettings.cpp icons.cpp lngtranslator.cpp helpwindow.cpp)
target_include_directories(test_panel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(test_panel PRIVATE LNG_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Languages")
target_link_libraries(test_panel PRIVATE rencore Qt6::Widgets Qt6::Network)
add_test(NAME panel COMMAND test_panel)

# Language files and help, read at run time from <exe>/Languages and <exe>/help.
# A target of its own, run at every build (a POST_BUILD command only runs after a link, so a change to the help alone
# was not copied). The help folder is emptied first, so a deleted file does not stay.
add_custom_target(antrenamer-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Languages $<TARGET_FILE_DIR:antrenamer-qt>/Languages
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:antrenamer-qt>/help
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/help $<TARGET_FILE_DIR:antrenamer-qt>/help
    COMMENT "Copying the language files and the help next to the executable")

# --- Windows deployment: Qt DLLs (windeployqt) + MinGW DLLs (script) -------
# The exe can then be started outside the MSYS2 shell (double-click).
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
