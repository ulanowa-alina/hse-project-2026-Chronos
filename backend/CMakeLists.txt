cmake_minimum_required(VERSION 3.20)
project(chronos_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PostgreSQL REQUIRED)
find_package(libpqxx REQUIRED)
find_package(Boost 1.74 REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PQXX REQUIRED libpqxx)

file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/models/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/repositories/*.cpp"
)

if(NOT BACKEND_SOURCES)
    message(FATAL_ERROR "No source files found in backend/src")
endif()

add_executable(chronos_backend ${BACKEND_SOURCES})

target_include_directories(chronos_backend PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/models"
        "${CMAKE_CURRENT_SOURCE_DIR}/repositories"
        ${Boost_INCLUDE_DIRS}
        ${PostgreSQL_INCLUDE_DIRS}
        ${PQXX_INCLUDE_DIRS}
)

target_compile_definitions(chronos_backend PRIVATE
    BOOST_ERROR_CODE_HEADER_ONLY
)

target_link_libraries(chronos_backend PRIVATE
        Boost::boost
        Threads::Threads
        pqxx::pqxx
        ${PQXX_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
)

target_compile_options(chronos_backend PRIVATE ${PQXX_CFLAGS_OTHER})
