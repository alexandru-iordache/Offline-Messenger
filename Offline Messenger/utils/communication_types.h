#ifndef COMMUNICATION_TYPES_H
#define COMMUNICATION_TYPES_H

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

typedef struct MessageStructure
{
    int id;
    char *sender;
    char *message;
    int read;
    int replyId;
} MessageStructure;

typedef struct UserViewStructure
{
    char *username;
    int unreadMessagesCount;
} UserViewStructure;

#endif