add_library(database STATIC

    db.c


)


target_include_directories(database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}


)

target_link_libraries(database PUBLIC
    ${SQLite3_LIBRARIES}


)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/database_setup.sql
    ${CMAKE_BINARY_DIR}/database/database_setup.sql
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/songs.db
    ${CMAKE_BINARY_DIR}/database/songs.db
    COPYONLY
)