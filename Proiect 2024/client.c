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
#define X_PRINT 8

// SOCKET constants
#define PORT 8989
#define ADDRESS "127.0.0.1"

// CLIENT variables
int socketDescriptor;
WINDOW *window;
char *loggedUsername = NULL;
unsigned short int authorized = 0;

// UI functions
int RenderFirstPage();
char RenderLoginView();
char RenderRegisterView();

int RenderSecondPage();
char RenderViewUsersView();

// Communication functions
unsigned short int ValidateUserInputs(const char *command, int length, char userInputs[][50]);
ServerResponse SendRequest(char *request);
ServerResponse SendLoginRequest(char userInputs[][50]);
ServerResponse SendRegisterRequest(char userInputs[][50]);
ServerResponse SendViewUsersRequest();

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
    keypad(window, TRUE);   // Enable special keys
    scrollok(window, TRUE); // Enable scrolling
    curs_set(0);            // Hide cursor

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    window = newwin(yMax / 2, xMax / 2, yMax / 4, xMax / 4);

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

        if (signal == -1)
        {
            continue;
        }

        signal = RenderSecondPage();
    }

    endwin();
    return 0;
}

// UI functions
int RenderFirstPage()
{
    wclear(window);
    box(window, 0, 0);

    mvwprintw(window, 2, X_PRINT + 12, "Offline Messenger");

    wattron(window, COLOR_PAIR(1));
    mvwprintw(window, 6, X_PRINT, "[1] Login");
    mvwprintw(window, 8, X_PRINT, "[2] Register");
    wattroff(window, COLOR_PAIR(1));

    wattron(window, COLOR_PAIR(2));
    mvwprintw(window, 10, X_PRINT, "[Q] Quit");
    wattroff(window, COLOR_PAIR(2));

    int ch = wgetch(window);
    if ((ch != 'q') && (ch != 'Q'))
    {
        if (ch == '1')
        {
            ch = RenderLoginView();
        }
        else if (ch == '2')
        {
            ch = RenderRegisterView();
        }
    }

    if (ch == 0)
    {
        return 0;
    }
    else if (ch == 'q' || ch == 'Q')
    {
        return -1;
    }

    RenderFirstPage();
}

char RenderLoginView()
{
    wclear(window);
    box(window, 0, 0);

    const int Y_PRINT = 4;
    char userInputs[2][50];

    mvwprintw(window, Y_PRINT - 2, X_PRINT + 12, "Login Page");

    wattron(window, COLOR_PAIR(1));

    echo();
    mvwprintw(window, Y_PRINT + 3, X_PRINT, "Enter Username: ");
    wgetstr(window, userInputs[0]);
    noecho();

    mvwprintw(window, Y_PRINT + 4, X_PRINT, "Enter Password: ");
    wgetstr(window, userInputs[1]);

    wattroff(window, COLOR_PAIR(1));

    struct ServerResponse serverResponse = SendLoginRequest(userInputs);
    if (serverResponse.status == 200)
    {
        authorized = 1;
        loggedUsername = strdup(serverResponse.content);
        return 0;
    }

    wattron(window, COLOR_PAIR(2));
    mvwaddstr(window, Y_PRINT, X_PRINT, "Press any button to continue or \'q\' to quit.");
    if (serverResponse.status == 0)
    {
        mvwaddstr(window, Y_PRINT + 1, X_PRINT, "Client Internal Error");
    }
    else
    {
        mvwaddstr(window, Y_PRINT + 1, X_PRINT, serverResponse.content);
    }
    wattroff(window, COLOR_PAIR(2));

    memset(userInputs, 0, sizeof(userInputs));
    free(serverResponse.content);

    char ch = wgetch(window);
    return ch;
}

char RenderRegisterView()
{
    wclear(window);
    box(window, 0, 0);

    const int Y_PRINT = 4;
    char userInputs[5][50];

    mvwprintw(window, Y_PRINT - 2, X_PRINT + 12, "Register Page");

    wattron(window, COLOR_PAIR(1));
    echo();

    mvwprintw(window, Y_PRINT + 3, X_PRINT, "Enter Username: ");
    wgetstr(window, userInputs[0]);

    mvwprintw(window, Y_PRINT + 4, X_PRINT, "Enter First Name: ");
    wgetstr(window, userInputs[1]);

    mvwprintw(window, Y_PRINT + 5, X_PRINT, "Enter Last Name: ");
    wgetstr(window, userInputs[2]);

    noecho();

    mvwprintw(window, Y_PRINT + 6, X_PRINT, "Enter Password: ");
    wgetstr(window, userInputs[3]);

    mvwprintw(window, Y_PRINT + 7, X_PRINT, "Confirm Password: ");
    wgetstr(window, userInputs[4]);

    wattroff(window, COLOR_PAIR(1));

    struct ServerResponse serverResponse = SendRegisterRequest(userInputs);
    if (serverResponse.status == 201)
    {
        authorized = 1;
        loggedUsername = strdup(serverResponse.content);
        return 0;
    }

    wattron(window, COLOR_PAIR(2));
    mvwaddstr(window, Y_PRINT, X_PRINT, "Press any button to continue or \'q\' to quit.");
    if (serverResponse.status == 0)
    {
        mvwaddstr(window, Y_PRINT + 1, X_PRINT, "Client Internal Error");
    }
    else
    {
        mvwaddstr(window, Y_PRINT + 1, X_PRINT, serverResponse.content);
    }
    wattroff(window, COLOR_PAIR(2));

    for (int i = 0; i < 5; i++)
    {
        memset(userInputs[i], 0, sizeof(userInputs[i]));
    }

    free(serverResponse.content);

    char ch = wgetch(window);
    return ch;
}

int RenderSecondPage()
{
    wclear(window);
    box(window, 0, 0);

    const int Y_PRINT = 2;

    mvwprintw(window, Y_PRINT, X_PRINT + 12, "Offline Messenger");

    wattron(window, COLOR_PAIR(1));
    mvwprintw(window, Y_PRINT + 4, X_PRINT, "[1] View Users");
    mvwprintw(window, Y_PRINT + 6, X_PRINT, "[2] View Unread Messages");
    wattroff(window, COLOR_PAIR(1));

    wattron(window, COLOR_PAIR(2));
    mvwprintw(window, Y_PRINT + 8, X_PRINT, "[L] Logout");
    mvwprintw(window, Y_PRINT + 10, X_PRINT, "[Q] Quit");
    wattroff(window, COLOR_PAIR(2));

    int ch = wgetch(window);
    if ((ch != 'q') && (ch != 'Q'))
    {
        if (ch == '1')
        {
            ch = RenderViewUsersView();
        }
        else if (ch == '2')
        {
            RenderSecondPage();
        }
        else if (ch == 'L' || ch == 'l')
        {
            authorized = 0;
            loggedUsername = NULL;
            return 0;
        }
    }

    if (ch == 'q' || ch == 'Q')
    {
        return -1;
    }

    RenderSecondPage();
}

char RenderViewUsersView()
{
    wclear(window);
    box(window, 0, 0);

    const int Y_PRINT = 2;

    mvwprintw(window, Y_PRINT, X_PRINT + 16, "View Users");

    ServerResponse serverResponse = SendViewUsersRequest();

    int numOfUsers = 0;
    char **users = ParseContent(serverResponse.content, &numOfUsers);
    
    int user_Y_PRINT = Y_PRINT + 4;
    for (int i = 0; i < numOfUsers; i++)
    {
        int len = snprintf(NULL, 0, "[%d] %s", i, users[i]);
        if (len <= 0)
        {
            wattron(window, COLOR_PAIR(2));
            mvwprintw(window, user_Y_PRINT , X_PRINT, "Client Error");
            wattroff(window, COLOR_PAIR(2));
            break;
        }

        char *userRow = (char *)malloc(len + 1);
        snprintf(userRow, len + 1, "[%d] %s", i, users[i]);
        
        wattron(window, COLOR_PAIR(1));
        mvwprintw(window, user_Y_PRINT, X_PRINT, userRow);
        wattroff(window, COLOR_PAIR(1));

        free(userRow);
        user_Y_PRINT += 2;
    }

    char ch = wgetch(window);
    int signal = 0;
    while (signal == 0)
    {
        if (numOfUsers > 0 && ch >= 0 && ch < numOfUsers)
        {
            FreeParsedStrings(users, numOfUsers);
            return 0;
        }
        else
        {
            signal != 1;
        }
    }

    FreeParsedStrings(users, numOfUsers);
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
    int len = snprintf(NULL, 0, "%s#%s#", userInputs[0], userInputs[1]);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;
        errorResponse.content = "Unrecognized inputs error\n";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s#", userInputs[0], userInputs[1]);

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
    int len = snprintf(NULL, 0, "%s#%s#%s#%s#%s#",
                       userInputs[0], userInputs[1], userInputs[2], userInputs[3], userInputs[4]);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s#%s#%s#%s#",
             userInputs[0], userInputs[1], userInputs[2], userInputs[3], userInputs[4]);

    char *clientRequest = CreateClientRequest("Register", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

ServerResponse SendViewUsersRequest()
{
    char *content = NULL;
    int len = snprintf(NULL, 0, "%s", loggedUsername);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s", loggedUsername);

    char *clientRequest = CreateClientRequest("View_Users", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}