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

#include <jansson.h>

// #include "sql/sqlite3.h"
// #include "sql/dbutils.h"

typedef struct threadData
{
    int threadID;
    int threadClient;
} threadData;

#define PORT 8989
#define ADDRESS "127.0.0.1"

const char *commands[] = {"Login", "Register", "Exit", "Logout", "Quit", "Back", "Help", "Select User", "View Users"};

static void *Treat(void *);

int main()
{
    // createDB("Database.db");

    struct sockaddr_in serverSocketStructure;
    struct sockaddr_in clientSocketStructure;

    int socketDescriptor;
    pthread_t threads[100];

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1)
    {
        printf("[SERVER][ERROR] Socket creation error!\n");
        return -1;
    }

    serverSocketStructure.sin_family = AF_INET;
    serverSocketStructure.sin_port = htons(PORT);
    serverSocketStructure.sin_addr.s_addr = inet_addr(ADDRESS);

    int reuse = 1;
    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        printf("[SERVER][ERROR] Error at set socket options (SO_REUSEADDR)!\n");
        return -1;
    }

    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        printf("[SERVER][ERROR] Error at set socket options (SO_REUSEPORT)!\n");
        return -1;
    }

    if (bind(socketDescriptor, (struct sockaddr *)&serverSocketStructure, sizeof(struct sockaddr)) == -1)
    {
        printf("[SERVER][ERROR] Error at bind!\n");
        return -1;
    }

    if (listen(socketDescriptor, 2) == -1)
    {
        printf("[SERVER][ERROR]  Error at listen().\n");
        return -1;
    }

    int clientId = 0;
    while(1)
    {
        int client;
        threadData *td;
        int length = sizeof(clientSocketStructure);

        printf("[SERVER] Waiting at PORT %d.\n", PORT);
        fflush(stdout);

        client = accept(socketDescriptor, (struct sockaddr *)&clientSocketStructure, &length);
        if (client < 0)
        {
            printf("[SERVER][ERROR] Error at accept().\n");
            continue;
        }

        printf("[SERVER] Client %d is accepted.\n", clientId);

        td = (struct threadData *)malloc(sizeof(struct threadData));
        td->threadID = clientId++;
        td->threadClient = client;

        pthread_create(&threads[clientId], NULL, &Treat, td);
    }
}

static void *Treat(void *arg)
{
    int logged = 0;
    int emergencyExit = 0;
    int quit = 0;

    ssize_t noOfBytesRead;
    char buffer[1024];

    struct threadData tdL;
    tdL = *((struct threadData *)arg);

    while(1)
    {

    }

    // pthread_detach(pthread_self());
    // while (!quit && !emergencyExit)
    // {
    //     while (!logged && !emergencyExit && !quit)
    //     {
    //         responseLogin = authentication(&tdL);
    //         if (responseLogin != NULL)
    //         {
    //             if (strcmp(responseLogin, "Error") == 0)
    //                 emergencyExit = 1;
    //             if (strcmp(responseLogin, "Quit") == 0)
    //                 quit = 1;

    //             logged = 1;
    //             username = malloc(sizeof(char) * strlen(responseLogin));
    //             strcpy(username, responseLogin);
    //         }
    //     }
    //     while (logged && !emergencyExit && !quit)
    //     {
    //         responseMessenger = messenger(&tdL, responseLogin);
    //         if (responseMessenger != NULL)
    //         {
    //             if (strcmp(responseLogin, "Error") == 0)
    //                 emergencyExit = 1;
    //             if (strcmp(responseLogin, "Quit") == 0)
    //                 quit = 1;

    //             logged = 0;
    //         }
    //     }
    // }
    // close((intptr_t)arg);
    // return (NULL);
}