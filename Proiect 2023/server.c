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

#include "sql/sqlite3.h"
#include "sql/dbutils.h"

#define PORT 8989
#define ADDRESS "127.0.0.1"

typedef struct threadData
{
    int threadID;
    int threadClient;
} threadData;

typedef struct chatRoomStructure
{
    int clientSocket;
    char *currentUser;
    char *selectedUser;
    int backFlag;
} chatRoomStructure;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
sqlite3 *db; //    0          1         2        3        4       5       6        7                8
const char *commands[] = {"Login", "Register", "Exit", "Logout", "Quit", "Back", "Help", "Select User", "View Users"};

void createDB();
static void *treat(void *);

int RetrieveCommandNumber(char clientInput[1024]);

int validateRegister(int clientSocket, int threadId, char credentials[1024]);
int registerUser(int clientSocket, int threadId);

char *ValidateLogin(int clientSocket, int threadId, char credentials[1024]);
char *loginUser(int clientSocket, int threadId, char *response);

void *receiveMessagesFromClient(void *arg);
char *chatRoom(int clientSocket, int threadId, char *currentUser, char *selectedUser);
char *viewUsers(int clientSocket, int threadId);
char *selectUser(int clientSocket, int threadId, char *currentUser);

char *authentication(threadData *tdL);
char *messenger(threadData *tdL, char *username);

int main()
{
    createDB();

    struct sockaddr_in serverStructure;
    struct sockaddr_in clientStructure;

    int socketDescriptor;
    pthread_t threads[100];

    int i = 0; // identificator threadd

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1)
    {
        printf("[server]Eroare la crearea socketului!\n");
        return 0;
    }

    bzero(&serverStructure, sizeof(serverStructure));
    bzero(&clientStructure, sizeof(clientStructure));

    serverStructure.sin_family = AF_INET;
    serverStructure.sin_port = htons(PORT);
    serverStructure.sin_addr.s_addr = inet_addr(ADDRESS);

    int reuse = 1;
    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        printf("[server] Eroare la SO_REUSEADDR!\n");
        return 1;
    }
    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        printf("[server] Eroare la SO_REUSEPORT!\n");
        return 1;
    }
    if (bind(socketDescriptor, (struct sockaddr *)&serverStructure, sizeof(struct sockaddr)) == -1)
    {
        printf("[server] Eroare la bind!\n");
        return 1;
    }

    if (listen(socketDescriptor, 2) == -1)
    {
        printf("[server] Eroare la listen().\n");
        return 1;
    }

    while (1)
    {

        int client;
        threadData *td;
        int length = sizeof(clientStructure);

        printf("[server] Asteptam la portul %d.\n", PORT);
        fflush(stdout);

        client = accept(socketDescriptor, (struct sockaddr *)&clientStructure, &length);
        if (client < 0)
        {
            printf("[server] Eroare la acceptare.\n");
            continue;
        }

        printf("[server] Clientul %d a fost acceptat.\n", client);

        td = (struct threadData *)malloc(sizeof(struct threadData));
        td->threadID = i++;
        td->threadClient = client;

        pthread_create(&threads[i], NULL, &treat, td);
    }
};

void createDB()
{
    db = createDatabase();
}

static void *treat(void *arg)
{
    int logged = 0;
    int emergencyExit = 0;
    int quit = 0;

    char *responseLogin;
    char *responseMessenger;
    char *username;

    ssize_t noOfBytesRead;
    char buffer[1024];

    struct threadData tdL;
    tdL = *((struct threadData *)arg);

    pthread_detach(pthread_self());
    while (!quit && !emergencyExit)
    {
        while (!logged && !emergencyExit && !quit)
        {
            responseLogin = authentication(&tdL);
            if (responseLogin != NULL)
            {
                if (strcmp(responseLogin, "Error") == 0)
                    emergencyExit = 1;
                if (strcmp(responseLogin, "Quit") == 0)
                    quit = 1;

                logged = 1;
                username = malloc(sizeof(char) * strlen(responseLogin));
                strcpy(username, responseLogin);
            }
        }
        while (logged && !emergencyExit && !quit)
        {
            responseMessenger = messenger(&tdL, responseLogin);
            if (responseMessenger != NULL)
            {
                if (strcmp(responseLogin, "Error") == 0)
                    emergencyExit = 1;
                if (strcmp(responseLogin, "Quit") == 0)
                    quit = 1;

                logged = 0;
            }
        }
    }
    close((intptr_t)arg);
    return (NULL);
}

/// @brief 
/// @param clientInput Char array representing the user command
/// @return An integer representing the command
int RetrieveCommandNumber(char clientInput[1024])
{
    int i = 0;
    while (commands[i] != NULL)
    {
        if (strcmp(commands[i], clientInput) == 0)
        {
            return i;
        }
        i++;
    }
    return -1;
}

/// @brief 
/// @param clientSocket 
/// @param threadId 
/// @param credentials 
/// @return A char array representing the response of login validation
char * ValidateLogin(int clientSocket, int threadId, char credentials[1024])
{
    char *sepCredentials[2];

    int i = 0;
    char *token = strtok(credentials, " ");
    while (token != NULL)
    {
        sepCredentials[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    char buffer[1024];
    if (i < 2)
    {
        strcpy(buffer, "Prea putine argumente transmise!\n");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return NULL;
    }
    if (i > 2)
    {
        strcpy(buffer, "Prea multe argumente transmise!\n");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return NULL;
    }
    pthread_mutex_lock(&clients_mutex);
    int validate = verifyUsernameAndPassword(db, sepCredentials[0], sepCredentials[1]);
    pthread_mutex_unlock(&clients_mutex);
    if (validate)
    {
        strcpy(buffer, "V-ati logat cu succes!\n");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return sepCredentials[0];
    }
    else
    {
        strcpy(buffer, "Datele introduse sunt gresite!\n");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return NULL;
    }
}

int validateRegister(int clientSocket, int threadId, char credentials[1024])
{
    char *sepCredentials[4];

    int i = 0;
    char *token = strtok(credentials, " ");
    while (token != NULL)
    {
        sepCredentials[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    char buffer[1024];
    if (i < 4)
    {
        strcpy(buffer, "Prea putine argumente transmise!");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return 0;
    }

    if (i > 4)
    {
        strcpy(buffer, "Prea multe argumente transmise!");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return 0;
    }

    pthread_mutex_lock(&clients_mutex);
    int validate = verifyUsername(db, sepCredentials[0]);
    pthread_mutex_unlock(&clients_mutex);
    if (!validate)
    {
        strcpy(buffer, "Username-ul exista deja!\n");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return 0;
    }

    if (strlen(sepCredentials[3]) < 8)
    {
        strcpy(buffer, "Parola trebuie sa aiba cel putin 8 caractere!");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        }
        fflush(stdout);
        return 0;
    }

    pthread_mutex_lock(&clients_mutex);
    validate = insertUser(db, sepCredentials[0], sepCredentials[1], sepCredentials[2], sepCredentials[3]);
    pthread_mutex_lock(&clients_mutex);

    if (!validate)
    {
        strcpy(buffer, "Inregistrarea nu a reusit!\n");
    }
    else
    {
        strcpy(buffer, "V-ati inregistrat cu succes!\n");
    }
    if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
    {
        printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
    }
    fflush(stdout);

    return validate;
}

int registerUser(int clientSocket, int threadId)
{
    char buffer[1024] = "Introduceti datele de conectare: <Username> <First_Name> <Last_Name> <Password> sau o comanda!\n";

    if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
    {
        printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
    }
    fflush(stdout);

    bzero(buffer, 1024);
    ssize_t noOfBytesRead = recv(clientSocket, buffer, 1024, 0);
    if (noOfBytesRead < 0)
    {
        printf("[Thread %d] Eroare la read() de la client.\n", threadId);
        return 0;
    }

    buffer[noOfBytesRead - 1] = 0;
    printf("[Thread %d] Datele au fost receptionate: %s.\n", threadId, buffer);

    int succes = 0;
    int commandNumber = RetrieveCommandNumber(buffer);
    switch (commandNumber)
    {
    case -1:
        succes = validateRegister(clientSocket, threadId, buffer);
        if (!succes)
            registerUser(clientSocket, threadId);
        else
            return 1;
        break;
    case 5:
        return 0;
        break;
    default:
        break;
    }
}

char *loginUser(int clientSocket, int threadId, char *response)
{
    char buffer[1024] = "Introduceti datele de conectare: <Username> <Password> sau o comanda!\n";

    if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
    {
        printf("[Thread %d] Eroare trimitere mesaj catre client!\n", threadId);
        strcpy(response, "Error");
        return response;
    }
    fflush(stdout);

    bzero(buffer, 1024);
    ssize_t noOfBytesRead = recv(clientSocket, buffer, 1024, 0);
    if (noOfBytesRead < 0)
    {
        printf("[Thread %d] Eroare la read() de la client.\n", threadId);
        strcpy(response, "Error");
        return response;
    }

    buffer[noOfBytesRead - 1] = 0;
    printf("[Thread %d] Datele au fost receptionate: %s.\n", threadId, buffer);

    int commandNumber = RetrieveCommandNumber(buffer);
    switch (commandNumber)
    {
    case -1:
        response = ValidateLogin(clientSocket, threadId, buffer);
        if (response == NULL)
            return loginUser(clientSocket, threadId);
        else
            return response;
        break;
    case 2:
        return NULL;
        break;
    case 5:
        return NULL;
        break;
    default:
        return NULL;
        break;
    }
}

char *authentication(threadData *tdL)
{
    char response[1024];

    char buffer[1024];
    ssize_t noOfBytesRead;

    printf("[Thread %d] Asteptam mesajul in etapa de logare/inregistrare:\n", tdL->threadID);
    fflush(stdout);

    strcpy(buffer, "Pentru a continua trebuie sa va logati sau sa va inregistrati! Pentru mai multe informatii accesati commanda <Help>!\n");
    if (send(tdL->threadClient, buffer, strlen(buffer), 0) <= 0)
    {
        printf("[Thread %d] Eroare la send() catre client!\n", tdL->threadID);
        strcpy(response, "Error");
        return response;
    }
    fflush(stdout);

    bzero(buffer, 1024);
    noOfBytesRead = recv(tdL->threadClient, buffer, 1024, 0);
    if (noOfBytesRead == -1)
    {
        printf("[Thread %d] Eroare la recv() de la client.\n", tdL->threadID);
        strcpy(response, "Error");
        return response;
    }

    buffer[noOfBytesRead - 1] = 0;
    printf("[Thread %d] Mesajul a fost receptionat: %s.\n", tdL->threadID, buffer);

    int commandNumber = RetrieveCommandNumber(buffer);
    switch (commandNumber)
    {
    case -1:
        printf("[Thread %d] Comanda introdusa este gresita!\n", tdL->threadID);
        return NULL;
    case 0:
        printf("[Thread %d] Login selected!\n", tdL->threadID);

        response = loginUser(tdL->threadClient, tdL->threadID);
        if (response != NULL)
            return response;
        return NULL;
    case 1:
        printf("[Thread %d] Register selected!\n", tdL->threadID);

        if (registerUser(tdL->threadClient, tdL->threadID))
        {
            response = loginUser(tdL->threadClient, tdL->threadID);
            if (response != NULL)
                return response;
        }

        break;
    case 4:
        printf("[Thread %d] Quit selected!\n", tdL->threadID);

        strcpy(response, "Quit");
        return response;
    case 6:
        printf("[Thread %d] Help selected!\n", tdL->threadID);

        strcpy(buffer, "Comenzile posibile sunt:\nLogin - logare\nRegister - inregistrare\nBack - inapoi\n\nQuit - inchidere\n");
        if (send(tdL->threadClient, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare la send() catre client!\n", tdL->threadID);
            strcpy(response, "Error send()");
        }
        fflush(stdout);
        return NULL;
    default:
        return NULL;
    }
}

void *receiveMessagesFromClient(void *arg)
{
    char buffer[1024];

    chatRoomStructure *chatR = (chatRoomStructure *)arg;
    int clientSocket = chatR->clientSocket;
    while (1)
    {
        ssize_t noOfBytesRead = recv(clientSocket, buffer, 1024, 0);
        if (noOfBytesRead < 0)
        {
            printf("[Server]Eroare la primirea mesajului de la client.\n");
            return;
        }
        else if (noOfBytesRead > 0)
        {
            buffer[noOfBytesRead - 1] = 0;
            if (strcmp(buffer, "Back") == 0)
            {
                chatR->backFlag = 1;
                pthread_exit(NULL);
                return;
            }

            pthread_mutex_lock(&clients_mutex);
            int succes = insertMessage(db, chatR->currentUser, chatR->selectedUser, buffer);
            if (succes == 0)
            {
                printf("[Server]Eroare la inserarea mesajului de la client.\n");
                chatR->backFlag = 1;
                pthread_mutex_unlock(&clients_mutex);
                pthread_exit(NULL);
                return;
            }
            pthread_mutex_unlock(&clients_mutex);
        }
        bzero(buffer, 1024);
    }
}

char *chatRoom(int clientSocket, int threadId, char *currentUser, char *selectedUser)
{
    char *response;
    char buffer[1024];

    pthread_mutex_lock(&clients_mutex);
    if (verifyUsername(db, selectedUser) != 0)
    {
        strcpy(buffer, "Userul introdus nu exista!");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            response = "Error";
            pthread_mutex_unlock(&clients_mutex);
            return response;
        }
        bzero(buffer, 1024);
        pthread_mutex_unlock(&clients_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&clients_mutex);

    pthread_mutex_lock(&clients_mutex);
    int noOfMessages = getCountMessages(db, currentUser, selectedUser);
    char *messages[noOfMessages];

    getAllMessages(db, messages, currentUser, selectedUser);
    pthread_mutex_unlock(&clients_mutex);

    for (int i = 0; i < noOfMessages; i++)
    {
        strcpy(buffer, messages[i]);
        strcat(buffer, "\n");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare la send() catre client!\n", threadId);
            response = malloc(sizeof(char) * strlen("Error"));
            strcpy(response, "Error");
            return response;
        }
        bzero(buffer, 1024);
        fflush(stdout);
    }

    chatRoomStructure chatR = {.clientSocket = clientSocket, .currentUser = currentUser, .selectedUser = selectedUser, .backFlag = 0};
    pthread_t receiverThread;

    pthread_create(&receiverThread, NULL, receiveMessagesFromClient, (void *)&chatR);

    while (chatR.backFlag == 0)
    {
        pthread_mutex_lock(&clients_mutex);
        int unreadMessages = getCountUnreadMessagesBetweenUsers(db, currentUser, selectedUser);
        pthread_mutex_unlock(&clients_mutex);

        if (unreadMessages < 0)
        {
            response = malloc(sizeof(char) * strlen("Error"));
            strcpy(response, "Error");
            return response;
        }
        if (unreadMessages > 0)
        {
            char *messages[unreadMessages];

            pthread_mutex_lock(&clients_mutex);
            getAllUnreadMessagesBetweenUsers(db, messages, currentUser, selectedUser);
            pthread_mutex_unlock(&clients_mutex);

            for (int i = 0; i < unreadMessages; i++)
            {
                strcpy(buffer, messages[i]);
                strcat(buffer, "\n");
                if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
                {
                    response = malloc(sizeof(char) * strlen("Error"));
                    strcpy(response, "Error");
                    return response;
                }
                bzero(buffer, 1024);
                fflush(stdout);
            }
        }
    }
    response = malloc(sizeof(char) * strlen("Back"));
    strcpy(response, "Back");
    return NULL;
}

char *selectUser(int clientSocket, int threadId, char *currentUser)
{
    char *response;

    char buffer[1024];
    ssize_t noOfBytesRead;

    strcpy(buffer, "Selectati user-ul pentru care doriti sa intrati in chatroom:\n");
    if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
    {
        printf("[Thread %d] Eroare la send() catre client!\n", threadId);
        strcpy(response, "Error");
        return response;
    }
    fflush(stdout);

    bzero(buffer, 1024);
    noOfBytesRead = recv(clientSocket, buffer, 1024, 0);
    if (noOfBytesRead < 0)
    {
        printf("[Thread %d] Eroare la read() de la client.\n", threadId);
        strcpy(response, "Error");
        return response;
    }

    buffer[noOfBytesRead - 1] = 0;
    printf("[Thread %d] Datele au fost receptionate: %s.\n", threadId, buffer);

    char *selectedUser = malloc(sizeof(char) * strlen(buffer));

    int commandNumber = RetrieveCommandNumber(buffer);
    switch (commandNumber)
    {
    case -1:
        strcpy(selectedUser, buffer);
        response = chatRoom(clientSocket, threadId, currentUser, selectedUser);
        return response;
        break;
    case 5:
        return NULL;
        break;
    default:
        return NULL;
        break;
    }
}

char *viewUsers(int clientSocket, int threadId)
{
    char *response;

    char buffer[1024];
    ssize_t noOfBytesRead;

    strcpy(buffer, "Vor fi listati toti userii care exista in baza de date!\n");
    if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
    {
        printf("[Thread %d] Eroare la send() catre client!\n", threadId);
        strcpy(response, "Error");
        return response;
    }
    fflush(stdout);

    pthread_mutex_lock(&clients_mutex);
    int noOfUsers = getCountUsers(db);
    char *users[noOfUsers];

    getAllUsers(db, users);
    pthread_mutex_unlock(&clients_mutex);

    for (int i = 0; i < noOfUsers; i++)
    {
        printf("%s\n", users[i]);
        strcpy(buffer, users[i]);
        strcat(buffer, "\n");
        if (send(clientSocket, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare la send() catre client!\n", threadId);
            strcpy(response, "Error");
            return response;
        }
        bzero(buffer, 1024);
        fflush(stdout);
    }
    return NULL;
}

char *messenger(threadData *tdL, char *username)
{
    char *response;

    char buffer[1024];
    ssize_t noOfBytesRead;

    printf("[Thread %d] Asteptam mesajul in etapa de rulare:\n", tdL->threadID);
    fflush(stdout);

    strcpy(buffer, "Trimiteti o comanda! Pentru mai multe informatii accesati commanda <Help>!\n");
    if (send(tdL->threadClient, buffer, strlen(buffer), 0) <= 0)
    {
        printf("[Thread %d] Eroare la send() catre client!\n", tdL->threadID);
        strcpy(response, "Error");
        return response;
    }
    fflush(stdout);

    bzero(buffer, 1024);
    noOfBytesRead = recv(tdL->threadClient, buffer, 1024, 0);
    if (noOfBytesRead == -1)
    {
        printf("[Thread %d] Eroare la recv() de la client.\n", tdL->threadID);
        strcpy(response, "Error");
        return response;
    }

    buffer[noOfBytesRead - 1] = 0;
    printf("[Thread %d] Mesajul a fost receptionat: %s.\n", tdL->threadID, buffer);

    int commandNumber = RetrieveCommandNumber(buffer);
    switch (commandNumber)
    {
    case -1:
        printf("[Thread %d] Comanda introdusa este gresita!\n", tdL->threadID);
        return NULL;
        break;
    case 3:
        printf("[Thread %d] Logout selected!\n", tdL->threadID);

        strcpy(response, "Logout");
        return response;

        break;
    case 4:
        printf("[Thread %d] Quit selected!\n", tdL->threadID);

        strcpy(response, "Quit");
        return response;

        break;
    case 6:
        printf("[Thread %d] Help selected!\n", tdL->threadID);

        strcpy(buffer, "Comenzile posibile sunt:\nLogout - logare\nSelect User - intrare chatroom\nView Users - vizualizare useri\nBack - inapoi\nQuit - inchidere\n");
        if (send(tdL->threadClient, buffer, strlen(buffer), 0) <= 0)
        {
            printf("[Thread %d] Eroare la send() catre client!\n", tdL->threadID);
            strcpy(response, "Error send()");
        }
        fflush(stdout);
        return NULL;

        break;
    case 7:
        printf("[Thread %d] Select User selected!\n", tdL->threadID);

        return selectUser(tdL->threadClient, tdL->threadID, username);

        break;
    case 8:
        printf("[Thread %d] View Users selected!\n", tdL->threadID);

        return viewUsers(tdL->threadClient, tdL->threadID);

        break;
    default:
        return NULL;

        break;
    }
}