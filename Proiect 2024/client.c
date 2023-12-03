#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

typedef struct ClientRequest
{
    unsigned short int authorized;
    char *command;
    char *content;
} ClientRequest;

typedef struct ServerResponse
{
    int status;
    char *content;
} ServerResponse;

#define PORT 8989
#define ADDRESS "127.0.0.1"

unsigned short int authorized = 0;

char *CreateClientRequest(const char *command, const char *content);
ServerResponse ParseServerResponse(const char *response);

int main()
{
    int socketDescriptor;
    struct sockaddr_in serverSocketStructure;

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1)
    {
        printf("[CLIENT][ERROR] Socket creation error!\n");
        return -1;
    }

    serverSocketStructure.sin_family = AF_INET;
    serverSocketStructure.sin_port = htons(PORT);
    serverSocketStructure.sin_addr.s_addr = inet_addr(ADDRESS);

    if (connect(socketDescriptor, (struct sockaddr *)&serverSocketStructure, sizeof(struct sockaddr)) == -1)
    {
        printf("[CLIENT][ERROR] Error at connect()!\n");
        return 0;
    }

    printf("[CLIENT] Connection succeded!\n");

    
    size_t bufferSize = 0;
    char serverResponse[2048];
    while (1)
    {
        memset(serverResponse, 0, 2048);
        
        char *buffer = NULL;
        ssize_t numberOfChars = getline(&buffer, &bufferSize, stdin);
        if (numberOfChars <= 0)
        {
            printf("[CLIENT] Enter more than 0 chars!\n");
            continue;
        }

        buffer[strlen(buffer) - 1] = '\0';
        char *request = CreateClientRequest(buffer, "");

        if (send(socketDescriptor, request, strlen(request), 0) <= 0)
        {
            printf("[CLIENT][ERROR] Error at send()!\n");
            continue;
        }

        ssize_t noOfBytesRead = recv(socketDescriptor, serverResponse, 2048, 0);
        if (noOfBytesRead < 0)
        {
            printf("[CLIENT][ERROR] Error at recv()!\n");
            continue;
        }
        else if (noOfBytesRead > 0)
        {
            printf("%s\n", serverResponse);
            struct ServerResponse responseStructure = ParseServerResponse(serverResponse);
            printf("[SERVER] %s\n", responseStructure.content);
        }
    }
}

char *CreateClientRequest(const char *command, const char *content)
{
    char *result = NULL;
    int len = snprintf(NULL, 0, "%d:%s:%s", authorized, command, content);

    if (len > 0)
    {
        result = (char *)malloc(len + 1);
        snprintf(result, len + 1, "%d:%s:%s", authorized, command, content);
    }

    return result;
}

ServerResponse ParseServerResponse(const char *response)
{
    struct ServerResponse responseStructure;
    responseStructure.status = 500;
    responseStructure.content = malloc(2048 * sizeof(char*));

    if (response == NULL)
    {
        return responseStructure;
    }

    char *token = strtok((char *)response, ":");
    if (token != NULL)
    {
        responseStructure.status = atoi(token);

        token = strtok(NULL, ":");
        if (token != NULL)
        {
            responseStructure.content = strdup(token);
        }
        else
        {
            responseStructure.status = 500;
            responseStructure.content = "Parse Error";
        }
    }

    return responseStructure;
}
