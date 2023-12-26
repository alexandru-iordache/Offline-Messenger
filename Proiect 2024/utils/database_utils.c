#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sql/sqlite3.h"

int createDatabase(sqlite3 **db, const char *databaseName)
{
    char *err;

    sqlite3_open(databaseName, &db);
    int rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS users(username VARCHAR(255) PRIMARY KEY UNIQUE, first_name VARCHAR(255), last_name VARCHAR(255), password VARCHAR(255));", NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] CREATE TABLE <USERS>: %s\n", err);
        return -1;
    }

    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages(id INTEGER PRIMARY KEY AUTOINCREMENT, sender VARCHAR(255), receiver VARCHAR(255), message VARCHAR(255), read INTEGER);", NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] CREATE TABLE <MESSAGES>: %s\n", err);
        return -1;
    }

    return 0;
}