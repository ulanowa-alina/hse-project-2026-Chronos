cmake_minimum_required(VERSION 3.20)
project(chronos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    include_directories(
            /opt/homebrew/include
            /opt/homebrew/opt/libpq/include
            /opt/homebrew/opt/libpqxx/include
    )
    link_directories(
            /opt/homebrew/lib
            /opt/homebrew/opt/libpq/lib
            /opt/homebrew/opt/libpqxx/lib
    )
    set(DB_LIBS pqxx pq)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PQXX REQUIRED libpqxx)
    pkg_check_modules(LIBPQ REQUIRED libpq)

    include_directories(${PQXX_INCLUDE_DIRS} ${LIBPQ_INCLUDE_DIRS})
    link_directories(${PQXX_LIBRARY_DIRS} ${LIBPQ_LIBRARY_DIRS})
    set(DB_LIBS ${PQXX_LIBRARIES} ${LIBPQ_LIBRARIES})
endif()

add_executable(chronos_backend backend/src/main.cpp)

target_link_libraries(chronos_backend PRIVATE
        ${DB_LIBS}
)
