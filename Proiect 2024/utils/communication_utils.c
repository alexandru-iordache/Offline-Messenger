#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "communication_types.h"

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