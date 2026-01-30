cmake_minimum_required(VERSION 3.20)
project(chronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#find_package(pqxx REQUIRED)

add_executable(chronos_backend
        backend/src/main.cpp
)
#target_link_libraries(chronos_backend PRIVATE pqxx::pqxx)

#find_package(Qt6 REQUIRED COMPONENTS Widgets)
#qt_standard_project_setup()

#add_executable(chronos_frontend
#        frontend/src/main.cpp
#)
#target_link_libraries(chronos_frontend PRIVATE Qt6::Widgets)