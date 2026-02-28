cmake_minimum_required(VERSION 3.16)
project(planner_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.74 REQUIRED)
find_package(Threads REQUIRED)

add_executable(planner_backend
    src/main.cpp
    src/server/personal/v1/info.cpp
)

target_include_directories(planner_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(planner_backend PRIVATE
    Boost::boost
    Threads::Threads
)

