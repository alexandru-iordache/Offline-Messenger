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

// #include "sql/sqlite3.h"
//  #include "sql/dbutils.h"

typedef struct threadData
{
    int threadID;
    int threadClient;
} threadData;

typedef struct ServerResponse
{
    int status;
    char *content;
} ServerResponse;

typedef struct ClientRequest
{
    unsigned short int authorized;
    char *command;
    char *content;
} ClientRequest;

#define PORT 8989
#define ADDRESS "127.0.0.1"

const char *commands[] = {"Login", "Register", "Logout", "Quit", "Help", "Select_User", "View_Users"};

static void *Treat(void *);

char *CreateServerResponse(int status, const char *content);
ClientRequest ParseClientRequest(const char *request);
char *ProcessClientRequest(ClientRequest requestStructure);

int RetrieveCommandNumber(const char *command);

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
    while (1)
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
    int quit = 0;

    struct threadData tdL;
    tdL = *((struct threadData *)arg);

    pthread_detach(pthread_self());

    char clientRequest[2048];
    while (!quit)
    {
        char *serverResponse;
        while (1)
        {
            ssize_t noOfBytesRead = recv(tdL.threadClient, clientRequest, 2048, 0);
            if (noOfBytesRead == -1)
            {
                printf("[SERVER][ERROR][Thread %d] Error at recv().\n", tdL.threadID);
                serverResponse = CreateServerResponse(500, "Error at recv().");
                break;
            }

            struct ClientRequest requestStructure = ParseClientRequest(clientRequest);

            serverResponse = ProcessClientRequest(requestStructure);
            break;
        }

        if (send(tdL.threadClient, serverResponse, strlen(serverResponse), 0) <= 0)
        {
            printf("[SERVER][ERROR][Thread %d] Error at recv().\n", tdL.threadID);
        }
    }

    close((intptr_t)arg);
    return (NULL);
}

char *CreateServerResponse(int status, const char *content)
{
    char *result = NULL;
    int len = snprintf(NULL, 0, "%d:%s", status, content);

    if (len < 0)
    {
        result = (char *)malloc(len + 1);
        snprintf(result, len + 1, "%d:%s", status, content);
    }

    return result;
}

ClientRequest ParseClientRequest(const char *request)
{
    struct ClientRequest requestStructure;
    requestStructure.authorized = 0;
    requestStructure.command = NULL;
    requestStructure.content = NULL;

    if (request == NULL)
    {
        return requestStructure;
    }

    char *token = strtok((char *)request, ":");
    if (token != NULL)
    {
        requestStructure.authorized = atoi(token);

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            requestStructure.command = strdup(token);
            token = strtok(NULL, ":");
            if (token != NULL)
            {
                requestStructure.content = strdup(token);
            }
        }
        else
        {
            requestStructure.authorized = 0;
            requestStructure.command = NULL;
            requestStructure.content = NULL;
        }
    }

    return requestStructure;
}

char *ProcessClientRequest(ClientRequest requestStructure)
{
    int commandNumber = RetrieveCommandNumber(requestStructure.command);
    if (commandNumber == -1)
    {
        return CreateServerResponse(400, "Bad request");
    }

    if (!requestStructure.authorized)
    {
        switch (commandNumber)
        {
        case 0:
            return CreateServerResponse(200, "Login");
            break;
        case 1:
            return CreateServerResponse(201, "Register");
            break;
        case 3:
            return CreateServerResponse(200, "Quit");
            break;
        case 4:
            return CreateServerResponse(200, "Help");
            break;
        default:
            return CreateServerResponse(401, "Unauthorized");
            break;
        }
    }
    else
    {
        switch (commandNumber)
        {
        case 0:
            return CreateServerResponse(409, "Already logged in");
            break;
        case 1:
            return CreateServerResponse(409, "Already logged in");
            break;
        case 2:
            return CreateServerResponse(200, "Logout");
            break;
        case 3:
            return CreateServerResponse(200, "Quit");
            break;
        case 4:
            return CreateServerResponse(200, "Help");
            break;
        case 5:
            return CreateServerResponse(200, "Select_User");
            break;
        case 6:
            return CreateServerResponse(200, "View_User");
            break;
        default:
            break;
        }
    }
}

int RetrieveCommandNumber(const char *command)
{
    if (command == NULL)
    {
        return -1;
    }

    int i = 0;
    while (commands[i] != NULL)
    {
        if (strcmp(commands[i], command) == 0)
        {
            return i;
        }
        i++;
    }
    return -1;
}