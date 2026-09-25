//tools to work with database

#include <stdbool.h>
#include "sqlite3.h"

//load database from file songs.db
bool load_database(sqlite3 ** db);
//Close databse 
void close_database(sqlite3 * db);

bool create_tables(sqlite3 * db);