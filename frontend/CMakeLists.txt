cmake_minimum_required(VERSION 3.16)
project(ChronosClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(ChronosClient
        src/main.cpp
        src/login_screen.cpp
        src/profile_screen.cpp
        src/registration_screen.cpp
        src/main_window.cpp
        src/network_manager.cpp
        include/login_screen.h
        include/profile_screen.h
        include/registration_screen.h
        include/main_window.h
        include/network_manager.h
)


target_include_directories(ChronosClient PRIVATE include)

target_link_libraries(ChronosClient PRIVATE
        Qt6::Widgets
        Qt6::Network
)


set_target_properties(ChronosClient PROPERTIES
        MACOSX_BUNDLE TRUE
)

add_custom_target(run
        COMMAND ./ChronosClient.app/Contents/MacOS/ChronosClient
        DEPENDS ChronosClient
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Launching ChronosClient..."
)
