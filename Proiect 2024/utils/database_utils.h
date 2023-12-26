#ifndef DATABASE_UTILS_H
#define DATABASE_UTILS_H

#include "../sql/sqlite3.h"

int createDatabase(sqlite3 **db, const char *databaseName);

#endif