cmake_minimum_required(VERSION 3.19)
project(Llama-launcher LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network LinguistTools)

set(CMAKE_AUTORCC ON)
qt_standard_project_setup()

qt_add_executable(Llama-launcher
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    gguf.cpp
    gguf.h
    resources.qrc
)

# Ukrainian UI translation, compiled and embedded under :/i18n
qt_add_translations(Llama-launcher
    TS_FILES translations/llama-launcher_uk.ts
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete
)

target_link_libraries(Llama-launcher
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

include(GNUInstallDirs)

install(TARGETS Llama-launcher
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Desktop integration (XDG): icon + menu entry pointing at the installed binary.
install(FILES rsc/llama-launcher.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
configure_file(llama-launcher.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/llama-launcher.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/llama-launcher.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
