cmake_minimum_required(VERSION 3.16)
project(ChronosClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

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
        include/board_screen.h
        src/board_screen.cpp
        include/status_window.h
        src/status_window.cpp
        include/task_card.h
        src/task_card.cpp
        src/profile_edit_screen.cpp
        include/profile_edit_screen.h
        src/local_db.cpp
        include/local_db.h
        local_repositories/local_board_repository.cpp
        local_repositories/local_board_repository.hpp
        local_repositories/local_status_repository.cpp
        local_repositories/local_status_repository.hpp
        local_repositories/local_task_repository.cpp
        local_repositories/local_task_repository.hpp
        local_models/local_task.hpp
        local_models/local_board.hpp
        local_models/local_status.hpp
)


target_include_directories(ChronosClient PRIVATE include)

target_link_libraries(ChronosClient PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
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
