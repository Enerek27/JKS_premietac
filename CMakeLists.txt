cmake_minimum_required(VERSION 3.27)

project(verzia_v2
    VERSION 1.0
    LANGUAGES C

)


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)

add_subdirectory(database)

add_executable(verzia_v2
    main.c

)

target_link_libraries(verzia_v2 PRIVATE
    database
)