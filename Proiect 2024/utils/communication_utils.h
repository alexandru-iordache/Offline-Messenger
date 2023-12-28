#ifndef COMMUNICATION_UTILS_H
#define COMMUNICATION_UTILS_H

#include "communication_types.h"

int RetrieveCommandNumber(const char *command);
char *CreateClientRequest(const char *command, const char *content, int authorized);
char *CreateServerResponse(int status, const char *content);
ClientRequest ParseClientRequest(const char *request);
ServerResponse ParseServerResponse(const char *response);

#endif