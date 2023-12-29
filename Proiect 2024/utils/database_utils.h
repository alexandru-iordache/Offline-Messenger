#ifndef DATABASE_UTILS_H
#define DATABASE_UTILS_H

#include "../sql/sqlite3.h"

int CreateDatabase(sqlite3 **db, const char *databaseName);
int OpenDatabase(sqlite3 **db, const char *databaseName);

int InsertUser(sqlite3 *db, const char *username, const char *firstName, const char *lastName, const char *password);

int GetUsersCountByUsername(sqlite3 *db, const char *username);
int GetUsersByUsernameAndPassword(sqlite3 *db, char *username, char *password);

#endif