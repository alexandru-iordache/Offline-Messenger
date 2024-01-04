#include "sqlite3.h"

#include <stdio.h>
#include <string.h>

sqlite3 *db;
sqlite3_stmt *stmt;

char *err;

sqlite3 *createDatabase()
{
    sqlite3_open("proiectDb.db", &db);
    int rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS users(username VARCHAR(255), first_name VARCHAR(255), last_name VARCHAR(255), password VARCHAR(255));", NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la createa tabelei <users>: %s", err);
    }
    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages(id INTEGER PRIMARY KEY, sender VARCHAR(255), receiver VARCHAR(255), message VARCHAR(255), read INTEGER);", NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la createa tabelei <messages>: %s", err);
    }

    return db;
}

int insertUser(sqlite3 *db, char *username, char *firstName, char *lastName, char *password)
{
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    char *query = sqlite3_mprintf("INSERT INTO users VALUES('%q','%q','%q','%q');", username, firstName, lastName, password);
    printf("[Database] Query: %s\n", query);

    int rc = sqlite3_exec(db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la inserarea unui <user>: %s\n", err);
        sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
        return 0;
    }

    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    return 1;
}

int getCountAllMessages(sqlite3 *db)
{
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM messages", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru numararea tuturor mesajelor: %s", err);
        return -1;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Database] Eroare la rularea statementului numararea tuturor mesajelor: %s", err);
        return -1;
    }
    int messagesCount = sqlite3_column_int(stmt, 0);

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return messagesCount;
}

int insertMessage(sqlite3 *db, char *currentUser, char *selectedUser, char *buffer)
{
    int id = getCountAllMessages(db);
    if (id == -1)
    {
        printf("[Database] Eroare la numararea tuturor mesajelor: %s\n", err);
        return 0;
    }
    id += 1;
    int read = 0;

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    char *query = sqlite3_mprintf("INSERT INTO messages VALUES(%d,'%q','%q','%q',%d);", id, currentUser, selectedUser, buffer, read);
    printf("[Database] Query: %s\n", query);

    int rc = sqlite3_exec(db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la inserarea unui <message>: %s\n", err);
        sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
        return 0;
    }
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    return 1;
}

int verifyUsername(sqlite3 *db, char *username)
{
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE username=?", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru cautarea unui <user> dupa username: %s", err);
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Database] Eroare la rularea statementului pentru cautarea unui <user> dupa username: %s", err);
        return 0;
    }
    int users = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    printf("[Database] Users with the username: %d\n", users);
    if (users == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int verifyUsernameAndPassword(sqlite3 *db, char *username, char *password)
{
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE username=? AND password=?", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru cautarea unui <user> dupa username si parola: %s", err);
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Database] Eroare la rularea statementului pentru cautarea unui <user> dupa username si parola: %s", err);
        return 0;
    }
    int users = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    printf("[Database] Users with the username: %d\n", users);
    if (users == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void updateUnreadMessages(sqlite3 *db, char *currentUser, char *selectedUser)
{
    int rc = sqlite3_prepare_v2(db, "UPDATE messages SET read = 1 WHERE receiver = ? AND sender = ?;", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru updatarea mesajelor dintre useri: %d", rc);
        return;
    }

    rc = sqlite3_bind_text(stmt, 1, currentUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUser, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        printf("[Database] Eroare la rularea statementului pentru updatarea mesajelor dintre useri: %d", rc);
        return;
    }
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return;
}

int getCountUsers(sqlite3 *db)
{
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru numararea userilor: %s", err);
        return 0;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Database] Eroare la rularea statementului pentru numararea userilor: %s", err);
        return 0;
    }
    int usersCount = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return usersCount;
}

int getCountMessages(sqlite3 *db, char *currentUser, char *selectedUser)
{
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM messages WHERE (receiver = ? AND sender = ?) OR (receiver = ? AND sender = ?)", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru numararea mesajelor: %s", err);
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, currentUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 3, selectedUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 4, currentUser, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Database] Eroare la rularea statementului pentru numararea mesajelor: %s", err);
        return 0;
    }
    int messagesCount = sqlite3_column_int(stmt, 0);

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return messagesCount;
}

int getCountUnreadMessagesBetweenUsers(sqlite3 *db, char *currentUser, char *selectedUser)
{
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM messages WHERE (receiver = ? AND sender = ? AND read = 0)", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru numararea mesajelor dintre useri: %d", rc);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 1, currentUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUser, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        printf("[Database] Eroare la rularea statementului pentru numararea mesajelor: %d", rc);
        return -1;
    }

    int messagesCount;
    if (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER)
    {
        messagesCount = sqlite3_column_int(stmt, 0);
    }
    else
    {
        messagesCount = 0;
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return messagesCount;
}

void getAllUsers(sqlite3 *db, char *users[])
{
    int noOfUsers = getCountUsers(db);

    int rc = sqlite3_prepare_v2(db, "SELECT username FROM users", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru cautarea unui <user> dupa username: %s", err);
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        users[0] = malloc(sizeof(char) * strlen((char *)sqlite3_column_text(stmt, 0)));
        strcpy(users[0], (char *)sqlite3_column_text(stmt, 0));
    }
    else if (rc == SQLITE_DONE)
    {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        return;
    }
    else
    {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        printf("[Database] Eroare la returnarea tuturor userilor din baza de date: %s", err);
        return;
    }
    int i = 1;

    rc = sqlite3_step(stmt);
    while (i < noOfUsers)
    {
        if (rc == SQLITE_ROW)
        {
            users[i] = malloc(sizeof(char) * strlen((char *)sqlite3_column_text(stmt, 0)));
            strcpy(users[i], (char *)sqlite3_column_text(stmt, 0));
            rc = sqlite3_step(stmt);
            i++;
        }
        else if (rc == SQLITE_DONE)
        {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            return;
        }
        else
        {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            printf("[Database] Eroare la returnarea tuturor userilor din baza de date: %s", err);
            return;
        }
    }
}

void getAllMessages(sqlite3 *db, char *messages[], char *currentUser, char *selectedUser)
{
    int noOfMessages = getCountMessages(db, currentUser, selectedUser);
    if (noOfMessages == 0)
    {
        return;
    }

    int rc = sqlite3_prepare_v2(db, "SELECT * FROM messages WHERE (receiver = ? AND sender = ?) OR (receiver = ? AND sender = ?) ORDER BY id", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru listarea mesajelor: %s", err);
        return;
    }

    rc = sqlite3_bind_text(stmt, 1, currentUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 3, selectedUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 4, currentUser, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        char *sender = (char *)sqlite3_column_text(stmt, 1);
        char *message = (char *)sqlite3_column_text(stmt, 3);

        char row[2024];
        sprintf(row, "[id = %d][%s]: %s", id, sender, message);

        messages[0] = malloc(sizeof(char) * strlen(row));
        strcpy(messages[0], row);
    }
    else if (rc == SQLITE_DONE)
    {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        return;
    }
    else
    {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        printf("[Database] Eroare la returnarea tuturor userilor din baza de date: %s", err);
        return;
    }

    int i = 1;
    rc = sqlite3_step(stmt);
    while (i < noOfMessages)
    {
        if (rc == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            char *sender = (char *)sqlite3_column_text(stmt, 1);
            char *message = (char *)sqlite3_column_text(stmt, 3);

            char row[2024];
            sprintf(row, "[id = %d][%s]: %s", id, sender, message);

            messages[i] = malloc(sizeof(char) * strlen(row));
            strcpy(messages[i], row);
            i++;
            rc = sqlite3_step(stmt);
        }
        else if (rc == SQLITE_DONE)
        {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);

            updateUnreadMessages(db, currentUser, selectedUser);
            return;
        }
        else
        {
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            printf("[Database] Eroare la returnarea tuturor userilor din baza de date: %s", err);
            return;
        }
    }
    updateUnreadMessages(db, currentUser, selectedUser);
}

void getAllUnreadMessagesBetweenUsers(sqlite3 *db, char *messages[], char *currentUser, char *selectedUser)
{
    int noOfMessages = getCountUnreadMessagesBetweenUsers(db, currentUser, selectedUser);
    if (noOfMessages == 0)
    {
        return;
    }

    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM messages WHERE (receiver = ? AND sender = ? AND read = 0) ORDER BY id", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("[Database] Eroare la crearea statementului pentru listarea mesajelor dintre useri: %s", err);
        return;
    }

    rc = sqlite3_bind_text(stmt, 1, currentUser, -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(stmt, 2, selectedUser, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        char *sender = (char *)sqlite3_column_text(stmt, 1);
        char *message = (char *)sqlite3_column_text(stmt, 3);

        char row[2024];
        sprintf(row, "[id = %d][%s]: %s", id, sender, message);

        messages[0] = malloc(sizeof(char) * strlen(row));
        strcpy(messages[0], row);
    }
    else if (rc == SQLITE_DONE)
    {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        return;
    }
    else
    {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        sqlite3_finalize(stmt);
        printf("[Database] Eroare la returnarea tuturor userilor din baza de date: %s", err);
        return;
    }

    int i = 1;
    rc = sqlite3_step(stmt);
    while (i < noOfMessages)
    {
        if (rc == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            char *sender = (char *)sqlite3_column_text(stmt, 1);
            char *message = (char *)sqlite3_column_text(stmt, 3);

            char row[2024];
            sprintf(row, "[id = %d][%s]: %s", id, sender, message);

            messages[i] = malloc(sizeof(char) * strlen(row));
            strcpy(messages[i], row);
            i++;
            rc = sqlite3_step(stmt);
        }
        else if (rc == SQLITE_DONE)
        {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            updateUnreadMessages(db, currentUser, selectedUser);
            return;
        }
        else
        {
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            printf("[Database] Eroare la returnarea tuturor userilor din baza de date: %s", err);
            return;
        }
    }
    updateUnreadMessages(db, currentUser, selectedUser);
}