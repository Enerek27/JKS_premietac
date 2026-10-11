add_library(graphical STATIC
    widgets.c
    buttons_reactions.c
    widget_structs.h
    presenter.c
)


target_include_directories(graphical PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}


)

target_link_libraries(graphical PUBLIC
    
    wifi
    database
    PkgConfig::GTK4


)

