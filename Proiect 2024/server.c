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
#include "utils/communication_types.h"
#include "utils/communication_utils.h"

#include "utils/database_utils.h"

typedef struct threadData
{
    int threadID;
    int threadClient;
} threadData;

// SOCKET constants
#define PORT 8989
#define ADDRESS "127.0.0.1"

#define DATABASE_NAME "Offline_Messenger_DB.db"
sqlite3 *DB;

static void *treat(void *);

char *ProcessClientRequest(ClientRequest requestStructure);
ServerResponse ProccesLoginRequest(ClientRequest clientRequest);
ServerResponse ProccesRegisterRequest(ClientRequest clientRequest);

int FileExists(const char *filename);
char **ParseContent(const char *content, int *numberOfInputs);
void FreeParsedStrings(char **strings, int numStrings);
int main()
{
    if (FileExists(DATABASE_NAME))
    {
        OpenDatabase(&DB, DATABASE_NAME);
    }
    else
    {
        CreateDatabase(&DB, DATABASE_NAME);
    }

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
        td->threadID = clientId;
        td->threadClient = client;

        int threadCreationResult = pthread_create(&threads[clientId], NULL, &treat, td);
        if (threadCreationResult != 0)
        {
            fprintf(stderr, "[SERVER][ERROR] Failed to create thread: %s\n", strerror(threadCreationResult));
        }
        clientId++;
    }
}

static void *treat(void *arg)
{
    int quit = 0;

    struct threadData *tdL;
    tdL = (struct threadData *)arg;

    pthread_detach(pthread_self());

    char clientRequest[2048];
    char *serverResponse = malloc(2048 * sizeof(char));
    while (!quit)
    {
        while (1)
        {
            ssize_t noOfBytesRead = recv(tdL->threadClient, clientRequest, 2048, 0);
            if (noOfBytesRead == -1)
            {
                printf("[SERVER][ERROR][Thread %d] Error at recv().\n", tdL->threadID);
                serverResponse = CreateServerResponse(500, "Error at recv().");
                break;
            }

            struct ClientRequest requestStructure = ParseClientRequest(clientRequest);
            serverResponse = ProcessClientRequest(requestStructure);

            break;
        }

        if (send(tdL->threadClient, serverResponse, strlen(serverResponse), 0) <= 0)
        {
            printf("[SERVER][ERROR][Thread %d] Error at recv().\n", tdL->threadID);
        }

        memset(serverResponse, 0, strlen(serverResponse));
    }

    free(serverResponse);
    close(tdL->threadClient);
    return (NULL);
}

// Proccesing functions
char *ProcessClientRequest(ClientRequest requestStructure)
{
    int commandNumber = RetrieveCommandNumber(requestStructure.command);
    if (commandNumber == -1)
    {
        return CreateServerResponse(400, "Bad request");
    }

    if (!requestStructure.authorized) // Unauthorized requests handling
    {
        struct ServerResponse responseStructure;

        switch (commandNumber)
        {
        case 0:
            responseStructure = ProccesLoginRequest(requestStructure);
        case 1:
            responseStructure = ProccesRegisterRequest(requestStructure);
        case 3:
            return CreateServerResponse(200, "Quit");
        case 4:
            return CreateServerResponse(200, "Help");
        default:
            return CreateServerResponse(401, "Unauthorized.");
        }

        return CreateServerResponse(responseStructure.status, responseStructure.content);
    }
    else // Authorized requests handling
    {
        switch (commandNumber)
        {
        case 0:
            return CreateServerResponse(409, "Already logged in.");
            break;
        case 1:
            return CreateServerResponse(409, "Already logged in.");
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

ServerResponse ProccesLoginRequest(ClientRequest clientRequest)
{
}

ServerResponse ProccesRegisterRequest(ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfInputs = 1;
    char **userInputs = ParseContent(clientRequest.content, &numberOfInputs);

    if (userInputs == NULL || numberOfInputs != 5)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error!";

        FreeParsedStrings(userInputs, numberOfInputs);
        return serverResponseStructure;
    }

    int usersCountByUsername = GetUsersCountByUsername(DB, userInputs[0]);
    if (usersCountByUsername > 0)
    {
        serverResponseStructure.status = 409;
        serverResponseStructure.content = "Username already exists!";

        FreeParsedStrings(userInputs, numberOfInputs);
        return serverResponseStructure;
    }
    if (usersCountByUsername < -1)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error!";

        return serverResponseStructure;
    }

    int insertResult = InsertUser(DB, userInputs[0], userInputs[1], userInputs[2], userInputs[3]);

    if (insertResult != 0)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Internal Server Error!";
    }
    else
    {
        serverResponseStructure.status = 201;
        serverResponseStructure.content = "Created.";
    }

    FreeParsedStrings(userInputs, numberOfInputs);
    return serverResponseStructure;
}

// Helper functions
int FileExists(const char *filename)
{
    return access(filename, F_OK) != -1;
}

void FreeParsedStrings(char **strings, int numStrings)
{
    for (int i = 0; i < numStrings; i++)
    {
        free(strings[i]);
    }
    free(strings);
}

char **ParseContent(const char *content, int *numberOfInputs)
{
    for (const char *ptr = content; *ptr != '\0'; ++ptr)
    {
        if (*ptr == '#')
        {
            (*numberOfInputs)++;
        }
    }

    if (numberOfInputs <= 0)
    {
        return NULL;
    }

    char **result = (char **)malloc((*numberOfInputs) * sizeof(char *));

    int i = 0;
    char *token = strtok((char *)content, "#");
    while (token != NULL)
    {
        result[i] = strdup(token);

        token = strtok(NULL, "#");
        i++;
    }

    return result;
}