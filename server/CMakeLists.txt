cmake_minimum_required(VERSION 3.19)
project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

qt_standard_project_setup()

qt_add_executable(server
    WIN32 MACOSX_BUNDLE
    main.cpp
    MessageFraming.h
    ClientConnection.h ClientConnection.cpp
    ServerManager.h ServerManager.cpp
    MainWindow.h MainWindow.cpp
)

target_link_libraries(server
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Gui
        Qt::Network
)
target_link_libraries(server PRIVATE Qt6::Core)
target_link_libraries(server PRIVATE Qt6::Core)
target_link_libraries(server PRIVATE Qt6::Widgets)

include(GNUInstallDirs)

install(TARGETS server
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET server
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
