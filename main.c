#include "db.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    sqlite3 *db = NULL;
    if (!load_database(&db))
    {
        return 1;
    }

    if (!create_tables(db))
    {
        close_database(db);
        return 1;
    }
    

    close_database(db);
    printf("Ending test OK\n");
    
    return 0;
}




