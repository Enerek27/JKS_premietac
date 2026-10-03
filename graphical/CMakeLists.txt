add_library(graphical STATIC
    widgets.c
)


target_include_directories(graphical PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}


)

target_link_libraries(graphical PUBLIC
    database
    PkgConfig::GTK4


)

