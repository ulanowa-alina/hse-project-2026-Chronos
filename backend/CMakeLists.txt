cmake_minimum_required(VERSION 3.20)
project(chronos_backend LANGUAGES CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED)

file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(NOT BACKEND_SOURCES)
    message(FATAL_ERROR "No source files found in backend/src")
endif()

add_executable(chronos_backend ${BACKEND_SOURCES})

target_include_directories(chronos_backend PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(chronos_backend PRIVATE
    Threads::Threads
)
