#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "communication_types.h"

const char *commands[] = {"Login", "Register", "Logout", "Quit", "Help", "Select_User", "View_Users"};

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