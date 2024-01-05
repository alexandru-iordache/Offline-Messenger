#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "communication_types.h"

const char *commands[] = {"Login", "Register", "Quit", "View_Messages", "View_Users", "Get_Users_Count", "Get_Messages_Count", "Insert_Message"};

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

char *CreateClientRequest(const char *command, const char *content, int authorized)
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

char *CreateServerResponse(int status, const char *content)
{
    char *result = NULL;
    int len = snprintf(NULL, 0, "%d:%s", status, content);

    if (len > 0)
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
            else
            {
                requestStructure.authorized = 0;
                requestStructure.command = NULL;
            }
        }
        else
        {
            requestStructure.authorized = 0;
        }
    }

    return requestStructure;
}

ServerResponse ParseServerResponse(const char *response)
{
    struct ServerResponse responseStructure;
    responseStructure.status = 500;
    responseStructure.content = NULL;

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
        }
    }

    return responseStructure;
}

MessageStructure ParseMessage(const char *message)
{
    struct MessageStructure messageStructure;
    messageStructure.id = -1;
    messageStructure.sender = NULL;
    messageStructure.message = NULL;

    if (message == NULL)
    {
        return messageStructure;
    }

    char *token = strtok((char *)message, "|");
    if (token != NULL)
    {
        messageStructure.id = atoi(token);

        token = strtok(NULL, "|");
        if (token != NULL)
        {
            messageStructure.sender = strdup(token);
            token = strtok(NULL, "|");
            if (token != NULL)
            {
                messageStructure.message = strdup(token);
            }
            else
            {
                messageStructure.id = -1;
                messageStructure.sender = NULL;
            }
        }
        else
        {
            messageStructure.id = -1;
        }
    }

    return messageStructure;
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

    if (*numberOfInputs <= 0)
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