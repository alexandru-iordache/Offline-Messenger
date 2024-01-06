#ifndef DATABASE_UTILS_H
#define DATABASE_UTILS_H

#include "../sql/sqlite3.h"

int CreateDatabase(sqlite3 **db, const char *databaseName);
int OpenDatabase(sqlite3 **db, const char *databaseName);

int InsertUser(sqlite3 *db, const char *username, const char *firstName, const char *lastName, const char *password);
int InsertMessage(sqlite3 *db, const char *loggedUsername, const char  *selectedUser, const char *message);

int UpdateMessage(sqlite3 *db, const int messageId);

int GetUsersCount(sqlite3 *db);
int GetUsersCountByUsername(sqlite3 *db, const char *username);
int GetUsersCountByUsernameAndPassword(sqlite3 *db, const char *username, const char *password);
int GetUsernamesWhereNotEqualUsername(sqlite3 *db, char **usernames, const char *username, const int page);

int GetMessagesCountBetweenUsers(sqlite3 *db, const char *loggedUsername, const char *selectedUsername);
int GetMessagesBetweenUsers(sqlite3 *db, char **messages, const char *loggedUsername, const char *selectedUsername, const int page);
#endif