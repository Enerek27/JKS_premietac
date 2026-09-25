#include "db.h"

#include <stdio.h>
#include <stdlib.h>



bool load_database(sqlite3 ** db) {
    
    int result = sqlite3_open("songs.db", db);
    if (result != SQLITE_OK) 
    {
        printf("Error loading database\n");
        sqlite3_close(*db);
        *db = NULL;
        sqlite3_errmsg(*db);
        return false;

    }

    printf("Database loaded\n");

    return true;
    
}



void close_database(sqlite3 * db) {
    if (db != NULL)
    {
        sqlite3_close(db);
        printf("Database closed\n");
    }
    
}

bool create_tables(sqlite3 *db)
{
    FILE * file = fopen("database/database_setup.sql", "r");
    if (file == NULL)
    {
        printf("Error opening database setup \n");
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);


    char *sql = malloc(size + 1);
    if (sql == NULL)
    {   
        fclose(file);
        printf("Error allocating memory\n");
        return false;
    }
    

    fread(sql, 1, size, file);
    sql[size] = '\0';

    fclose(file);

    char * error_msg = NULL;

    int result = sqlite3_exec(
        db,
        sql,
        NULL,
        NULL,
        &error_msg
    );


    free(sql);
    if (result != SQLITE_OK)
    {
        printf("Database setup error: %s\n", error_msg);
        return false;
    }
    
    printf("Database setup completed\n");
    

    return true;
}
