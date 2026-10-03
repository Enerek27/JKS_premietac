cmake_minimum_required(VERSION 3.27)

project(verzia_v2
    VERSION 1.0
    LANGUAGES C

)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_subdirectory(database)
add_subdirectory(graphical)

add_executable(verzia_v2
    main.c

)

target_link_libraries(verzia_v2 PRIVATE
    database
    graphical
    PkgConfig::GTK4
)