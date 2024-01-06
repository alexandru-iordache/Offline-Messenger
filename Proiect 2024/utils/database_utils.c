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

    rc = sqlite3_exec(*db, "CREATE TABLE IF NOT EXISTS messages(id INTEGER PRIMARY KEY, sender VARCHAR(255), receiver VARCHAR(255), message VARCHAR(255), read INTEGER);", NULL, NULL, &err);
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

int GetUsersCount(sqlite3 *db)
{
    sqlite3_stmt *stmt;
    char *err;

    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] Users count query prepare error: %s\n", err);
        fflush(stdout);
        return -1;
    }

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

int GetUsersCountByUsernameAndPassword(sqlite3 *db, const char *username, const char *password)
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

int GetUsernamesWhereNotEqualUsername(sqlite3 *db, char **usernames, const char *username, const int page)
{
    sqlite3_stmt *stmt;
    char *err;

    const int PAGE_SIZE = 10;
    int offset = (page - 1) * PAGE_SIZE;

    int rc = sqlite3_prepare_v2(db, "SELECT username FROM users WHERE username!=? LIMIT ? OFFSET ?", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] GET usernames query prepare error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    rc = sqlite3_bind_int(stmt, 2, PAGE_SIZE);
    rc = sqlite3_bind_int(stmt, 3, offset);

    int i = 0;
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW)
    {
        usernames[i] = strdup((char *)sqlite3_column_text(stmt, 0));
        usernames[i][strlen(usernames[i])] = '\0';
        rc = sqlite3_step(stmt);
        i++;
    }

    if (rc != SQLITE_DONE)
    {
        printf("[Error][Database] GET usernames query execute error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    return i;
}

int GetMessagesCountBetweenUsers(sqlite3 *db, const char *loggedUsername, const char *selectedUsername)
{
    sqlite3_stmt *stmt;
    char *err;

    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM messages WHERE (receiver = ? AND sender = ?) OR (receiver = ? AND sender = ?)", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] Messages count between users query prepare error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, loggedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 3, selectedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 4, loggedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Error][Database] Messages count between users query execute error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    int messagesCount = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return messagesCount;
}

int GetMessagesBetweenUsers(sqlite3 *db, char **messages, const char *loggedUsername, const char *selectedUsername, const int page)
{
    sqlite3_stmt *stmt;
    char *err;

    const int PAGE_SIZE = 10;
    int offset = (page - 1) * PAGE_SIZE;

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM messages WHERE (receiver = ? AND sender = ?) OR (receiver = ? AND sender = ?) ORDER BY id DESC LIMIT ? OFFSET ?", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] GET messages between users query prepare error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, loggedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 3, selectedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 4, loggedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_int(stmt, 5, PAGE_SIZE);
    rc = sqlite3_bind_int(stmt, 6, offset);

    int i = 0;
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        char *sender = (char *)sqlite3_column_text(stmt, 1);
        char *message = (char *)sqlite3_column_text(stmt, 3);
        int read = sqlite3_column_int(stmt, 4);

        int len = snprintf(NULL, 0, "%d|%s|%s|%d", id, sender, message, read);
        if (len < 0)
        {
            printf("[Error][Database] GET messages parsing error: %s\n", err);
            fflush(stdout);
            return -1;
        }

        messages[i] = (char *)malloc(len + 1);
        snprintf(messages[i], len + 1, "%d|%s|%s|%d", id, sender, message, read);
        messages[i][strlen(messages[i])] = '\0';

        rc = sqlite3_step(stmt);
        i++;
    }

    if (rc != SQLITE_DONE)
    {
        printf("[Error][Database] GET messages query execute error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    return i;
}

int GetUnreadMessagesCountBetweenUsers(sqlite3 *db, const char *loggedUsername, const char *selectedUsername)
{
    sqlite3_stmt *stmt;
    char *err;

    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM messages WHERE (receiver = ? AND sender = ?) AND read = 0", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] GET Unread Messages count between users query prepare error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, loggedUsername, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUsername, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Error][Database] GET Unread Messages count between users query execute error: %s\n", err);
        fflush(stdout);
        return -1;
    }

    int messagesCount = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return messagesCount;
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

int InsertMessage(sqlite3 *db, const char *loggedUsername, const char *selectedUser, const char *message)
{
    char *err;

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    char *query = sqlite3_mprintf("INSERT INTO messages(sender, receiver, message, read)  VALUES('%q','%q','%q', %d);", loggedUsername, selectedUser, message, 0);
    printf("[Database] Query: %s\n", query);

    int rc = sqlite3_exec(db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] Message insert query exec error: %s\n", err);
        fflush(stdout);

        sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
        return -1;
    }

    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    return 0;
}

int UpdateMessage(sqlite3 *db, const int messageId)
{
    char *err;

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    char *query = sqlite3_mprintf("UPDATE messages SET read = 1 WHERE id = %d;", messageId);

    int rc = sqlite3_exec(db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Error][Database] Message update query exec error: %s\n", err);
        fflush(stdout);

        sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
        return -1;
    }

    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    return 0;
}