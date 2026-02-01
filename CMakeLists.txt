cmake_minimum_required(VERSION 3.20)
project(chronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt6")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(
        /opt/homebrew/include
        /opt/homebrew/opt/libpq/include
)

link_directories(
        /opt/homebrew/lib
        /opt/homebrew/opt/libpq/lib
)

add_executable(chronos_backend
        backend/src/main.cpp
)

target_link_libraries(chronos_backend PRIVATE
        Qt6::Widgets
        pqxx
        "/opt/homebrew/opt/libpq/lib/libpq.dylib"
)