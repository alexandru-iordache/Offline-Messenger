#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include "sql/sqlite3.h"
#include "sql/dbutils.h"

#define PORT 8989
#define ADDRESS "127.0.0.1"

int main()
{
    createDB("Database.db");
    printDatabaseName();
}