cmake_minimum_required(VERSION 3.19)
project(client LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_executable(client
    WIN32 MACOSX_BUNDLE
    main.cpp
    MessageFraming.h
    DeviceClient.h DeviceClient.cpp
)

target_link_libraries(client
    PRIVATE
        Qt::Core
        Qt::Network
)
target_link_libraries(client PRIVATE Qt6::Core Qt6::Network)

include(GNUInstallDirs)

install(TARGETS client
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET client
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
