find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
set(CMAKE_AUTORCC ON)

set(SRC main.cpp mainwindow.cpp actionpanel.cpp appsettings.cpp dialogs.cpp icons.cpp lngtranslator.cpp helpwindow.cpp shellmenu.cpp theme.cpp updatecheck.cpp resources.qrc)
if(WIN32)
    enable_language(RC)
    list(APPEND SRC app.rc)   # icone + manifeste (chemins longs, UTF-8, DPI par ecran)
    set_source_files_properties(app.rc PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/renamer.ico;${CMAKE_CURRENT_SOURCE_DIR}/app.manifest")  # le .rc ne les cite pas en #include : sans ceci, changer l'icone ne refait pas l'exe
endif()

add_executable(antrenamer-qt WIN32 ${SRC})
target_link_libraries(antrenamer-qt PRIVATE rencore Qt6::Widgets Qt6::Network)
target_compile_definitions(antrenamer-qt PRIVATE AR_APP_VERSION="${AR_VERSION}")
if(MSVC)
    target_compile_options(antrenamer-qt PRIVATE /utf-8)
endif()

# Test d'aller-retour du panneau d'actions (charger une action dans l'editeur puis la relire), sans fenetre.
# Echoue si un champ ou un type d'action est ajoute sans mettre a jour ActionPanel::setFromAction.
add_executable(test_panel ../tests/test_panel.cpp actionpanel.cpp appsettings.cpp icons.cpp lngtranslator.cpp helpwindow.cpp)
target_include_directories(test_panel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(test_panel PRIVATE LNG_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Languages")
target_link_libraries(test_panel PRIVATE rencore Qt6::Widgets Qt6::Network)
add_test(NAME panel COMMAND test_panel)

# Fichiers de langue et aide, lus a l'execution depuis <exe>/Languages et <exe>/help.
# Cible a part, jouee a chaque compilation (une commande POST_BUILD ne l'est qu'apres une edition de liens : une
# modification de l'aide seule n'etait pas recopiee). L'aide est videe d'abord : un fichier supprime ne reste pas.
add_custom_target(antrenamer-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Languages $<TARGET_FILE_DIR:antrenamer-qt>/Languages
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:antrenamer-qt>/help
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/help $<TARGET_FILE_DIR:antrenamer-qt>/help
    COMMENT "Copie des fichiers de langue et de l'aide a cote de l'executable")

# --- Deploiement Windows : DLL Qt (windeployqt) + DLL MinGW (script) -------
# Meme mecanisme que ComponentHub : l'exe est lancable hors du shell MSYS2 (double-clic).
if(WIN32)
    find_program(WINDEPLOYQT_EXE windeployqt
        HINTS "${Qt6_DIR}/../../../bin" "$ENV{QT_ROOT}/bin")
    if(WINDEPLOYQT_EXE)
        add_custom_command(TARGET antrenamer-qt POST_BUILD
            COMMAND "${WINDEPLOYQT_EXE}" --no-translations --compiler-runtime "$<TARGET_FILE:antrenamer-qt>"
            COMMENT "Deploiement des DLL Qt a cote de l'executable")
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
                COMMENT "Copie des DLL MinGW a cote de l'executable")
        else()
            message(WARNING "MSYS2 Bash not found; non-Qt MinGW DLL collection is skipped")
        endif()
    endif()
endif()
