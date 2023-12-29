#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <ncurses.h>

#include "utils/communication_types.h"
#include "utils/communication_utils.h"

// UI constants
#define X_PRINT 14

// SOCKET constants
#define PORT 8989
#define ADDRESS "127.0.0.1"

// CLIENT variables
int socketDescriptor;
unsigned short int authorized = 0;

// UI functions
int RenderFirstPage();

// Communication functions
unsigned short int ValidateUserInputs(const char *command, int length, char userInputs[][50]);
ServerResponse SendRequest(char *request);
ServerResponse SendLoginRequest(char userInputs[][50]);
ServerResponse SendRegisterRequest(char userInputs[][50]);

int main()
{
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
        return -1;
    }

    // UI settings
    initscr();
    cbreak();               // Disable line buffering
    noecho();               // Don't print user input
    keypad(stdscr, TRUE);   // Enable special keys
    scrollok(stdscr, TRUE); // Enable scrolling
    curs_set(0);            // Hide cursor

    // Set up colors if supported
    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
    }

    int signal = 0;
    while (signal != -1)
    {
        signal = RenderFirstPage();
    }

    // Clean up and exit
    endwin();
    return 0;
}

// UI functions
int RenderFirstPage()
{
    clear();

    attron(COLOR_PAIR(1));
    mvprintw(2, X_PRINT, "[1] Login");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(1));
    mvprintw(4, X_PRINT, "[2] Register");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2));
    mvprintw(6, X_PRINT, "[Q] Quit");
    attroff(COLOR_PAIR(2));

    const int Y_PRINT = 10;

    int ch = getch();
    while ((ch != 'q') && (ch != 'Q'))
    {
        // Process button presses
        if (ch == '1' || ch == '1')
        {
            attron(COLOR_PAIR(1));
            mvprintw(Y_PRINT - 2, X_PRINT, "Login Button Pressed!");

            char userInputs[2][50];

            echo();
            mvprintw(Y_PRINT + 2, X_PRINT, "Enter Username: ");
            getstr(userInputs[0]);
            noecho();

            mvprintw(Y_PRINT + 3, X_PRINT, "Enter Password: ");
            getstr(userInputs[1]);

            attroff(COLOR_PAIR(1));

            struct ServerResponse serverResponse = SendLoginRequest(userInputs);

            if (serverResponse.status == 200)
            {
                authorized = 1;
                return 0;
            }

            attron(COLOR_PAIR(2));
            mvaddstr(Y_PRINT - 2, X_PRINT, "Press any button to continue or \'q\' to quit.");
            if (serverResponse.status == 0)
            {
                mvaddstr(Y_PRINT, X_PRINT, "Client Internal Error");
            }
            else
            {
                mvaddstr(Y_PRINT, X_PRINT, serverResponse.content);
            }
            attroff(COLOR_PAIR(2));

            memset(userInputs, 0, sizeof(userInputs));
            free(serverResponse.content);

            ch = getch();

            break;
        }
        else if (ch == '2' || ch == '2')
        {
            char userInputs[5][50];

            attron(COLOR_PAIR(1));
            mvprintw(Y_PRINT, X_PRINT, "Register Button Pressed!");

            echo();

            mvprintw(Y_PRINT + 2, X_PRINT, "Enter Username: ");
            getstr(userInputs[0]);

            mvprintw(Y_PRINT + 3, X_PRINT, "Enter First Name: ");
            getstr(userInputs[1]);

            mvprintw(Y_PRINT + 4, X_PRINT, "Enter Last Name: ");
            getstr(userInputs[2]);

            noecho();

            mvprintw(Y_PRINT + 5, X_PRINT, "Enter Password: ");
            getstr(userInputs[3]);

            mvprintw(Y_PRINT + 6, X_PRINT, "Confirm Password: ");
            getstr(userInputs[4]);

            attroff(COLOR_PAIR(1));

            struct ServerResponse serverResponse = SendRegisterRequest(userInputs);

            if (serverResponse.status == 201)
            {
                authorized = 1;
                return 0;
            }

            attron(COLOR_PAIR(2));
            mvaddstr(Y_PRINT - 2, X_PRINT, "Press any button to continue or \'q\' to quit.");
            if (serverResponse.status == 0)
            {
                mvaddstr(Y_PRINT, X_PRINT, "Client Internal Error");
            }
            else
            {
                mvaddstr(Y_PRINT, X_PRINT, serverResponse.content);
            }

            attroff(COLOR_PAIR(2));

            for (int i = 0; i < 5; i++)
            {
                memset(userInputs[i], 0, sizeof(userInputs[i]));
            }

            free(serverResponse.content);
            ch = getch();

            break;
        }
    }

    if (ch == 'q' || ch == 'Q')
    {
        return -1;
    }

    RenderFirstPage();
}

// Communication functions
unsigned short int ValidateUserInputs(const char *command, int length, char userInputs[][50])
{
    for (int i = 0; i < length; i++)
    {
        if (userInputs[i] == NULL || strlen(userInputs[i]) == 0)
        {
            return 1;
        }

        userInputs[i][strlen(userInputs[i])] = '\0';
        if (strchr(userInputs[i], ':') != NULL || strchr(userInputs[i], '#') != NULL)
        {
            return 2;
        }
    }

    int commandNumber = RetrieveCommandNumber(command);

    switch (commandNumber)
    {
    case 1:
        if (strlen(userInputs[3]) < 6)
        {
            return 3;
        }
        if (strcmp(userInputs[3], userInputs[4]) != 0)
        {
            return 4;
        }
        break;
    default:
        break;
    }

    return 0;
}

ServerResponse SendRequest(char *request)
{
    char serverResponse[2048];
    memset(serverResponse, 0, 2048);

    if (send(socketDescriptor, request, strlen(request), 0) <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;
        errorResponse.content = "[CLIENT][ERROR] Error at send()!\n";

        free(request);
        return errorResponse;
    }

    ssize_t noOfBytesRead = recv(socketDescriptor, serverResponse, 2048, 0);
    if (noOfBytesRead < 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;
        errorResponse.content = "[CLIENT][ERROR] Error at recv()!\n";

        free(request);
        return errorResponse;
    }
    else if (noOfBytesRead > 0)
    {
        struct ServerResponse responseStructure = ParseServerResponse(serverResponse);
        free(request);
        return responseStructure;
    }
}

ServerResponse SendLoginRequest(char userInputs[][50])
{
    unsigned short int validationCode = ValidateUserInputs("Login", 2, userInputs);
    if (validationCode != 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;

        switch (validationCode)
        {
        case 1:
            errorResponse.content = "The inputs should not be empty.";
            break;
        case 2:
            errorResponse.content = "The inputs should not contain \":\" or \"#\" characters.";
            break;
        }

        return errorResponse;
    }

    char *content = NULL;
    int len = snprintf(NULL, 0, "%s#%s", userInputs[0], userInputs[1]);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;
        errorResponse.content = "Unrecognized inputs error\n";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s", userInputs[0], userInputs[1]);

    char *clientRequest = CreateClientRequest("Login", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

ServerResponse SendRegisterRequest(char userInputs[][50])
{
    unsigned short int validationCode = ValidateUserInputs("Register", 5, userInputs);
    if (validationCode != 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;

        switch (validationCode)
        {
        case 1:
            errorResponse.content = "The inputs should not be empty.";
            break;
        case 2:
            errorResponse.content = "The inputs should not contain \":\" or \"#\" characters.";
            break;
        case 3:
            errorResponse.content = "The password should have at least 6 characters.";
            break;
        case 4:
            errorResponse.content = "The passwords don't match.";
        }

        return errorResponse;
    }

    char *content = NULL;
    int len = snprintf(NULL, 0, "%s#%s#%s#%s#%s",
                       userInputs[0], userInputs[1], userInputs[2], userInputs[3], userInputs[4]);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;
        errorResponse.content = "Unrecognized inputs error.\n";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s#%s#%s#%s",
             userInputs[0], userInputs[1], userInputs[2], userInputs[3], userInputs[4]);

    char *clientRequest = CreateClientRequest("Register", content, authorized);

    return SendRequest(clientRequest);
}