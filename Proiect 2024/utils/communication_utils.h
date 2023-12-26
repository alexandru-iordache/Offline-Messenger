#ifndef COMMUNICATION_UTILS_H
#define COMMUNICATION_UTILS_H

#include "communication_types.h"

char *CreateClientRequest(const char *command, const char *content, int authorized);
ServerResponse ParseServerResponse(const char *response);

#endif