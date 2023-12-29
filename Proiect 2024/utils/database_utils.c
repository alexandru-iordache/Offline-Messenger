#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../sql/sqlite3.h"

int CreateDatabase(sqlite3 **db, const char *databaseName)
{
    char *err;

    sqlite3_open(databaseName, db);
    int rc = sqlite3_exec(*db, "CREATE TABLE IF NOT EXISTS users(username VARCHAR(255) PRIMARY KEY UNIQUE, first_name VARCHAR(255), last_name VARCHAR(255), password VARCHAR(255));", NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] CREATE TABLE <USERS>: %s\n", err);
        fflush(stdout);
        return -1;
    }

    rc = sqlite3_exec(*db, "CREATE TABLE IF NOT EXISTS messages(id INTEGER PRIMARY KEY AUTOINCREMENT, sender VARCHAR(255), receiver VARCHAR(255), message VARCHAR(255), read INTEGER);", NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] CREATE TABLE <MESSAGES>: %s\n", err);
        fflush(stdout);
        return -1;
    }

    return 0;
}

int OpenDatabase(sqlite3 **db, const char *databaseName)
{
    char *err;

    int rc = sqlite3_open(databaseName, db);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] Open Database Error: %s\n", err);
        fflush(stdout);
        sqlite3_close(*db);
        return -1;
    }

    return 0;
}

int GetUsersCountByUsername(sqlite3 *db, const char *username)
{
    sqlite3_stmt *stmt;
    char *err;

    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE username=?", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] Users count by username query prepare error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Error][Database] Users count query execute error: %s\n", err);
        fflush(stdout);
        return -1;
    }
    int usersCount = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return usersCount;
}

int GetUsersByUsernameAndPassword(sqlite3 *db, char *username, char *password)
{
    sqlite3_stmt *stmt;
    char *err;

    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE username=? AND password=?", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] Users count by username and password query prepare error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Error][Database] Users count by username and password query execute error: %s\n", err);
        fflush(stdout);
        return -1;
    }
    int usersCount = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return usersCount;
}

int InsertUser(sqlite3 *db, const char *username, const char *firstName, const char *lastName, const char *password)
{
    char *err;

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    char *query = sqlite3_mprintf("INSERT INTO users VALUES('%q','%q','%q','%q');", username, firstName, lastName, password);
    printf("[Database] Query: %s\n", query);

    int rc = sqlite3_exec(db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] User insert query exec error: %s\n", err);
        fflush(stdout);

        sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
        return -1;
    }

    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    return 0;
}
