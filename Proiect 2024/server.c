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

#define FILENAME_FOLDER "logs/"
#define DATABASE_NAME "Offline_Messenger_DB.db"

char *FILE_NAME;
sqlite3 *DB;

pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;
static void *treat(void *);

char *ProcessClientRequest(const int clientId, ClientRequest requestStructure);
ServerResponse ProccesLoginRequest(const int clientId, ClientRequest clientRequest);
ServerResponse ProccesRegisterRequest(const int clientId, ClientRequest clientRequest);
ServerResponse ProcessViewUsersRequest(const int clientId, ClientRequest clientRequest);
ServerResponse ProccesViewMessagesRequest(const int clientId, ClientRequest clientRequest);
ServerResponse ProcessGetUsersCountRequest(const int clientId, ClientRequest clientRequest);
ServerResponse ProcessGetMessagesCountRequest(const int clientId, ClientRequest clientRequest);
ServerResponse ProcessInsertMessageRequest(const int clientId, ClientRequest clientRequest);
ServerResponse ProccesUpdateMessageReadRequest(const int clientId, ClientRequest clientRequest);

char *PrepareUsersViewContent(const char **rows, const int *counts, int rowsCount);
char *PrepareViewContent(const char **rows, int rowsCount);

int FileExists(const char *filename);
int CreateFile();

void LogEvent(int clientId, const char *event);
void LogRequestEvent(int clientId, const ClientRequest clientRequestStructure);
void LogResponseEvent(int clientId, const ServerResponse serverResponseStructure);

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

    int createLogFileFlag = CreateFile();
    if (!CreateFile())
    {
        printf("[SERVER][ERROR] Error at create log file.!\n");
        return -1;
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

    LogEvent(tdL->threadID, "Client connected");

    char clientRequest[8192];
    char *serverResponse = NULL;
    while (!quit)
    {
        while (1)
        {
            ssize_t noOfBytesRead = recv(tdL->threadClient, clientRequest, 8192, 0);
            if (noOfBytesRead == -1)
            {
                printf("[SERVER][ERROR][Client %d] Error at recv().\n", tdL->threadID);
                fflush(stdout);
                serverResponse = CreateServerResponse(500, "Error at recv().");
                break;
            }
            else if (noOfBytesRead == 0)
            {
                LogEvent(tdL->threadID, "Client disconnected");
                quit = 1;
                break;
            }
            clientRequest[noOfBytesRead] = '\0';

            struct ClientRequest requestStructure = ParseClientRequest(clientRequest);
            LogRequestEvent(tdL->threadID, requestStructure);

            serverResponse = ProcessClientRequest(tdL->threadID, requestStructure);

            break;
        }

        if (quit != 1)
        {
            if (send(tdL->threadClient, serverResponse, strlen(serverResponse), 0) <= 0)
            {
                printf("[SERVER][ERROR][Client %d] Error at recv().\n", tdL->threadID);
            }

            free(serverResponse);
        }

        memset(clientRequest, 0, strlen(clientRequest));
    }

    close(tdL->threadClient);
    return (NULL);
}

// Proccesing functions
char *ProcessClientRequest(const int clientId, ClientRequest requestStructure)
{
    struct ServerResponse responseStructure;
    int commandNumber = RetrieveCommandNumber(requestStructure.command);
    if (commandNumber == -1)
    {
        responseStructure.status = 400;
        responseStructure.content = "Bad request.";
        LogResponseEvent(clientId, responseStructure);
        return CreateServerResponse(responseStructure.status, responseStructure.content);
    }

    if (!requestStructure.authorized) // Unauthorized requests handling
    {
        switch (commandNumber)
        {
        case 0:
            responseStructure = ProccesLoginRequest(clientId, requestStructure);
            break;
        case 1:
            responseStructure = ProccesRegisterRequest(clientId, requestStructure);
            break;
        case 2:
            return CreateServerResponse(200, "Quit");
        default:
            return CreateServerResponse(401, "Unauthorized.");
        }
    }
    else // Authorized requests handling
    {
        switch (commandNumber)
        {
        case 0:
            return CreateServerResponse(409, "Already logged in.");
        case 1:
            return CreateServerResponse(409, "Already logged in.");
        case 2:
            return CreateServerResponse(200, "Quit");
            break;
        case 3:
            responseStructure = ProccesViewMessagesRequest(clientId, requestStructure);
            break;
        case 4:
            responseStructure = ProcessViewUsersRequest(clientId, requestStructure);
            break;
        case 5:
            responseStructure = ProcessGetUsersCountRequest(clientId, requestStructure);
            break;
        case 6:
            responseStructure = ProcessGetMessagesCountRequest(clientId, requestStructure);
            break;
        case 7:
            responseStructure = ProcessInsertMessageRequest(clientId, requestStructure);
            break;
        case 8:
            responseStructure = ProccesUpdateMessageReadRequest(clientId, requestStructure);
            break;
        default:
            break;
        }
    }

    LogResponseEvent(clientId, responseStructure);
    return CreateServerResponse(responseStructure.status, responseStructure.content);
}

ServerResponse ProccesLoginRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfInputs = 0;
    char **userInputs = ParseContent(clientRequest.content, &numberOfInputs);

    if (userInputs == NULL || numberOfInputs != 2)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error!";

        FreeParsedStrings(userInputs, numberOfInputs);
        LogEvent(clientId, "Login - Unsuccesfully Parse Content");
        return serverResponseStructure;
    }
    LogEvent(clientId, "Login - Succesfully Parse Content");

    int usersCountByUsernameAndPassword = GetUsersCountByUsernameAndPassword(DB, userInputs[0], userInputs[1]);
    switch (usersCountByUsernameAndPassword)
    {
    case 0:
        serverResponseStructure.status = 401;
        serverResponseStructure.content = "Check Username and Password.";
        LogEvent(clientId, "Login - Database - GetUsersCountByUsernameAndPassword - Count = 0");
        break;
    case 1:
        serverResponseStructure.status = 200;
        serverResponseStructure.content = strdup(userInputs[0]);
        LogEvent(clientId, "Login - Database - GetUsersCountByUsernameAndPassword - Count = 1");
        break;
    default:
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Internal Server Error!";
        LogEvent(clientId, "Login - Database - GetUsersCountByUsernameAndPassword - Count = -1");
        break;
    }

    FreeParsedStrings(userInputs, numberOfInputs);
    return serverResponseStructure;
}

ServerResponse ProccesRegisterRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfInputs = 0;
    char **userInputs = ParseContent(clientRequest.content, &numberOfInputs);

    if (userInputs == NULL || numberOfInputs != 5)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error!";

        FreeParsedStrings(userInputs, numberOfInputs);
        LogEvent(clientId, "Register - Unsuccesfully Parse Content");
        return serverResponseStructure;
    }
    LogEvent(clientId, "Register - Succesfully Parse Content");

    int usersCountByUsername = GetUsersCountByUsername(DB, userInputs[0]);
    if (usersCountByUsername > 0)
    {
        serverResponseStructure.status = 409;
        serverResponseStructure.content = "Username already exists!";

        FreeParsedStrings(userInputs, numberOfInputs);
        LogEvent(clientId, "Register - Database - GetUsersCountByUsername - Count Bigger > 0");
        return serverResponseStructure;
    }
    if (usersCountByUsername <= -1)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error!";

        FreeParsedStrings(userInputs, numberOfInputs);
        LogEvent(clientId, "Register - Database - GetUsersCountByUsername - Count Lower < 0");
        return serverResponseStructure;
    }

    int insertResult = InsertUser(DB, userInputs[0], userInputs[1], userInputs[2], userInputs[3]);

    if (insertResult != 0)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Internal Server Error!";
        LogEvent(clientId, "Register - Database - Insert - Unsuccesful");
    }
    else
    {
        serverResponseStructure.status = 201;
        serverResponseStructure.content = strdup(userInputs[0]);
        LogEvent(clientId, "Register - Database - Insert - Succesful");
    }

    FreeParsedStrings(userInputs, numberOfInputs);
    return serverResponseStructure;
}

ServerResponse ProcessViewUsersRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfFields = 0;
    char **fields = ParseContent(clientRequest.content, &numberOfFields);
    if (numberOfFields != 2)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error";
        LogEvent(clientId, "View_Users - ParseContent - Count != 2");

        return serverResponseStructure;
    }

    int userExists = GetUsersCountByUsername(DB, fields[0]);
    if (userExists != 1)
    {
        serverResponseStructure.status = 400;
        serverResponseStructure.content = "Username doesn't exists!";
        LogEvent(clientId, "View_Users - Database - GetUsersCountByUsername - Count != 1");

        return serverResponseStructure;
    }

    int usersCount = GetUsersCount(DB);
    if (usersCount == 1)
    {
        serverResponseStructure.status = 200;
        serverResponseStructure.content = "";
        LogEvent(clientId, "View_Users - Database - GetUsersCount - Count == 1");

        return serverResponseStructure;
    }
    else if (usersCount <= 0)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error";
        LogEvent(clientId, "View_Users - Database - GetUsersCount - Unsuccesful");

        return serverResponseStructure;
    }

    char **usernames = (char **)malloc(10 * sizeof(char *));

    int usernamesCount = GetUsernamesWhereNotEqualUsername(DB, usernames, fields[0], atoi(fields[1]));
    if (usernamesCount <= -1)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Internal Server Error!";
        LogEvent(clientId, "View_Users - Database - GetUsernames - Unsuccesful");

        return serverResponseStructure;
    }
    LogEvent(clientId, "View_Users - Database - GetUsernames - Succesful");

    int unreadMessagesCounts[usernamesCount];
    for (int i = 0; i < usernamesCount; i++)
    {
        int count = GetUnreadMessagesCountBetweenUsers(DB, fields[0], usernames[i]);
        if (count < 0)
        {
            serverResponseStructure.status = 500;
            serverResponseStructure.content = "Internal Server Error!";
            LogEvent(clientId, "View_Users - Database - GetUnreadMessagesCount - Unsuccesful");

            return serverResponseStructure;
        }

        unreadMessagesCounts[i] = count;
    }

    char *content = PrepareUsersViewContent(usernames, &unreadMessagesCounts, usernamesCount);
    if (content == NULL)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error!";
        LogEvent(clientId, "View_Users - PrepareContent - Allocation Error");
    }
    else
    {
        serverResponseStructure.status = 200;
        serverResponseStructure.content = strdup(content);
        LogEvent(clientId, "View_Users - PrepareContent - Succesful");
    }

    free(content);
    FreeParsedStrings(usernames, usernamesCount);
    FreeParsedStrings(fields, numberOfFields);

    return serverResponseStructure;
}

ServerResponse ProccesViewMessagesRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfFields = 0;
    char **fields = ParseContent(clientRequest.content, &numberOfFields);
    if (numberOfFields != 3)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error";
        LogEvent(clientId, "View_Messages - ParseContent - Count != 3");

        return serverResponseStructure;
    }

    int userExists = GetUsersCountByUsername(DB, fields[1]);
    if (userExists != 1)
    {
        serverResponseStructure.status = 400;
        serverResponseStructure.content = "Username doesn't exists!";
        LogEvent(clientId, "View_Messages - Database - GetUsersCountByUsername - Count != 1");

        return serverResponseStructure;
    }

    char **messages = (char **)malloc(10 * sizeof(char *));

    int messagesCount = GetMessagesBetweenUsers(DB, messages, fields[0], fields[1], atoi(fields[2]));
    if (messagesCount <= -1)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Internal Server Error!";
        LogEvent(clientId, "View_Messages - Database - GetMessagesBetweenUsers - Unsuccesful");

        return serverResponseStructure;
    }
    LogEvent(clientId, "View_Messages - Database - GetMessagesBetweenUsers - Succesful");

    char *content = PrepareViewContent(messages, messagesCount);
    if (content == NULL)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error!";
        LogEvent(clientId, "View_Messages - PrepareContent - Allocation Error");
    }
    else
    {
        serverResponseStructure.status = 200;
        serverResponseStructure.content = strdup(content);
        LogEvent(clientId, "View_Messages - PrepareContent - Succesful");
    }

    free(content);
    FreeParsedStrings(messages, messagesCount);
    FreeParsedStrings(fields, numberOfFields);

    return serverResponseStructure;
}

ServerResponse ProcessGetUsersCountRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    char *currentUser = strdup(clientRequest.content);

    int userExists = GetUsersCountByUsername(DB, currentUser);
    if (userExists != 1)
    {
        serverResponseStructure.status = 400;
        serverResponseStructure.content = "Username doesn't exists!";
        LogEvent(clientId, "Get_Users_Count - Database - GetUsersCountByUsername - Count != 1");

        return serverResponseStructure;
    }

    int usersCount = GetUsersCount(DB);
    if (usersCount <= 0)
    {
        LogEvent(clientId, "Get_Users_Count - Database - GetUsersCount - Unsuccesful");
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error";
    }
    else
    {
        LogEvent(clientId, "Get_Users_Count - Database - GetUsersCount - Succesful");
        int len = snprintf(NULL, 0, "%d", usersCount);
        if (len <= 0)
        {
            LogEvent(clientId, "Get_Users_Count - Create Response Error");
            serverResponseStructure.status = 500;
            serverResponseStructure.content = "Server Internal Error";

            return serverResponseStructure;
        }

        char *content = (char *)malloc(len + 1);
        snprintf(content, len + 1, "%d", usersCount);

        serverResponseStructure.status = 200;
        serverResponseStructure.content = strdup(content);

        free(content);
    }

    return serverResponseStructure;
}

ServerResponse ProcessGetMessagesCountRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfFields = 0;
    char **fields = ParseContent(clientRequest.content, &numberOfFields);
    if (numberOfFields != 2)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error";
        LogEvent(clientId, "Get_Messages_Count - ParseContent - Count != 2");

        return serverResponseStructure;
    }

    int userExists = GetUsersCountByUsername(DB, fields[1]);
    if (userExists != 1)
    {
        serverResponseStructure.status = 400;
        serverResponseStructure.content = "Username doesn't exists!";
        LogEvent(clientId, "Get_Messages_Count - Database - GetUsersCountByUsername - Count != 1");

        return serverResponseStructure;
    }

    int messagesCount = GetMessagesCountBetweenUsers(DB, fields[0], fields[1]);
    if (messagesCount < 0)
    {
        LogEvent(clientId, "Get_Messages_Count - Database - GetMessagesCountBetweenUsers - Unsuccesful");
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error";
    }
    else
    {
        LogEvent(clientId, "Get_Messages_Count - Database - GetMessagesCountBetweenUsers - Succesful");
        int len = snprintf(NULL, 0, "%d", messagesCount);
        if (len <= 0)
        {
            LogEvent(clientId, "Get_Messages_Count - Create Response Error");
            serverResponseStructure.status = 500;
            serverResponseStructure.content = "Server Internal Error";

            return serverResponseStructure;
        }

        char *content = (char *)malloc(len + 1);
        snprintf(content, len + 1, "%d", messagesCount);

        serverResponseStructure.status = 200;
        serverResponseStructure.content = strdup(content);

        free(content);
    }

    FreeParsedStrings(fields, numberOfFields);
    return serverResponseStructure;
}

ServerResponse ProcessInsertMessageRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfFields = 0;
    char **fields = ParseContent(clientRequest.content, &numberOfFields);
    if (numberOfFields != 3)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Server Internal Error";
        LogEvent(clientId, "Insert_Message - ParseContent - Count != 3");

        return serverResponseStructure;
    }

    int userExists = GetUsersCountByUsername(DB, fields[1]);
    if (userExists != 1)
    {
        serverResponseStructure.status = 400;
        serverResponseStructure.content = "Username doesn't exists!";
        LogEvent(clientId, "Insert_Message - Database - GetUsersCountByUsername - Count != 1");

        return serverResponseStructure;
    }

    int insertResult = InsertMessage(DB, fields[0], fields[1], fields[2]);
    if (insertResult != 0)
    {
        serverResponseStructure.status = 500;
        serverResponseStructure.content = "Internal Server Error!";
        LogEvent(clientId, "Insert_Message - Database - Insert - Unsuccesful");
    }
    else
    {
        serverResponseStructure.status = 201;
        serverResponseStructure.content = "Created";
        LogEvent(clientId, "Insert_Message - Database - Insert - Succesful");
    }

    FreeParsedStrings(fields, numberOfFields);
    return serverResponseStructure;
}

ServerResponse ProccesUpdateMessageReadRequest(const int clientId, ClientRequest clientRequest)
{
    struct ServerResponse serverResponseStructure;

    int numberOfFields = 0;
    char **fields = ParseContent(clientRequest.content, &numberOfFields);

    for (int i = 0; i < numberOfFields; i++)
    {
        int updateResult = UpdateMessage(DB, atoi(fields[i]));
        if (updateResult != 0)
        {
            LogEvent(clientId, "Update_Message_Read - Database - Update - Unsuccesful");
        }
    }

    LogEvent(clientId, "Update_Message_Read - Database - Update - Finished");

    serverResponseStructure.status = 200;
    serverResponseStructure.content = "Updated";
    LogEvent(clientId, "Update_Message_Read - Update - Succesful");

    return serverResponseStructure;
}

char *PrepareUsersViewContent(const char **rows, const int *counts, int rowsCount)
{
    char **rowsWithCounts = (char **)malloc(rowsCount * sizeof(char *));
    for (int i = 0; i < rowsCount; i++)
    {
        int len = snprintf(NULL, 0, "%s|%d|", rows[i], counts[i]);
        if (len < 0)
        {
            return NULL;
        }

        rowsWithCounts[i] = (char **)malloc(len + 1);
        snprintf(rowsWithCounts[i], len + 1, "%s|%d|", rows[i], counts[i]);
    }

    char *content = PrepareViewContent(rowsWithCounts, rowsCount);

    free(rowsWithCounts);
    return content;
}

char *PrepareViewContent(const char **rows, int rowsCount)
{
    int contentLength = 0;
    for (int i = 0; i < rowsCount; i++)
    {
        int rowLength = snprintf(NULL, 0, "%s#", rows[i]);
        if (rowLength <= 0)
        {
            return NULL;
        }
        contentLength += rowLength;
    }

    char *content = (char *)malloc(contentLength + 1);
    if (content == NULL)
    {
        return NULL;
    }

    int offset = 0;
    for (int i = 0; i < rowsCount; i++)
    {
        offset += snprintf((content + offset), contentLength - offset + 1, "%s#", rows[i]);
        if (offset <= 0)
        {
            return NULL;
        }
    }

    return content;
}

// Helper functions
int FileExists(const char *filename)
{
    return access(filename, F_OK) != -1;
}

int CreateFile()
{
    time_t currentTime;
    struct tm timeInfo;

    char timeString[50];
    memset(timeString, 0, 50);

    time(&currentTime);

    localtime_r(&currentTime, &timeInfo);

    strftime(timeString, 50, "%Y-%m-%d_%H:%M:%S", &timeInfo);

    int len = snprintf(NULL, 0, "%s%s_LOGS.txt", FILENAME_FOLDER, timeString);
    if (len <= 0)
    {
        return 0;
    }

    FILE_NAME = (char *)malloc(len + 1);
    snprintf(FILE_NAME, len + 1, "%s%s_LOGS.txt", FILENAME_FOLDER, timeString);

    FILE *file = fopen(FILE_NAME, "a");
    if (file == NULL)
    {
        return 0;
    }

    fclose(file);
    return 1;
}

void LogEvent(int clientId, const char *event)
{
    time_t currentTime;
    struct tm timeInfo;

    char timeString[50];
    memset(timeString, 0, 50);

    time(&currentTime);

    localtime_r(&currentTime, &timeInfo);

    strftime(timeString, 50, "%H:%M:%S", &timeInfo);

    int len = snprintf(NULL, 0, "[Client %d][%s] - %s\n", clientId, timeString, event);
    if (len <= 0)
    {
        printf("[SERVER][ERROR][Client %d] Log Write error.\n", clientId);
        return;
    }

    char *eventLog = NULL;
    eventLog = (char *)malloc(len + 1);
    snprintf(eventLog, len + 1, "[Client %d][%s] - %s\n", clientId, timeString, event);

    pthread_mutex_lock(&fileMutex);

    FILE *file = fopen(FILE_NAME, "a");
    if (file == NULL)
    {
        free(eventLog);
        printf("[SERVER][ERROR][Client %d] Log Write error.\n", clientId);
        return;
    }

    fprintf(file, "%s", eventLog);
    fclose(file);
    pthread_mutex_unlock(&fileMutex);

    free(eventLog);
    return;
}

void LogRequestEvent(int clientId, const ClientRequest clientRequestStructure)
{
    int len = snprintf(NULL, 0, "[REQUEST] - [Auth: %d][Command: %s][Content: %s]", clientRequestStructure.authorized,
                       clientRequestStructure.command, clientRequestStructure.content);
    if (len <= 0)
    {
        return;
    }

    char *event = NULL;
    event = (char *)malloc(len + 1);
    snprintf(event, len + 1, "[REQUEST] - [Auth: %d][Command: %s][Content: %s]", clientRequestStructure.authorized, clientRequestStructure.command,
             clientRequestStructure.content);

    LogEvent(clientId, event);
    free(event);
}

void LogResponseEvent(int clientId, const ServerResponse serverResponseStructure)
{
    int len = snprintf(NULL, 0, "[RESPONSE] - [Status: %d][Content: %s]", serverResponseStructure.status,
                       serverResponseStructure.content);
    if (len <= 0)
    {
        return;
    }

    char *event = NULL;
    event = (char *)malloc(len + 1);
    snprintf(event, len + 1, "[RESPONSE] - [Status: %d][Content: %s]", serverResponseStructure.status,
             serverResponseStructure.content);

    LogEvent(clientId, event);
    free(event);
}
