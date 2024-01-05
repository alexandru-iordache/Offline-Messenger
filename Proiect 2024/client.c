#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
char RenderSelectUserView(const char *selectedUser);

// Helper Functions
void ClearRows(int startRow, int endRow);

// Communication functions
unsigned short int ValidateUserInputs(const char *command, int length, char userInputs[][50]);
ServerResponse SendRequest(char *request);
ServerResponse SendLoginRequest(char userInputs[][50]);
ServerResponse SendRegisterRequest(char userInputs[][50]);
ServerResponse SendViewUsersRequest(int currentPage);
ServerResponse SendViewMessagesRequest(int currentPage, const char *selectedUser);
ServerResponse SendGetUsersCountRequest();
ServerResponse SendGetMessagesCountRequest(const char *selectedUser);
ServerResponse SendInsertMessageRequest(const char *selectedUser, const char *message);

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
    ServerResponse getUsersCountServerResponse = SendGetUsersCountRequest();
    if (getUsersCountServerResponse.status != 200)
    {
        wattron(window, COLOR_PAIR(2));
        mvwaddstr(window, Y_PRINT + 4, X_PRINT, "Press any button to return.");
        mvwaddstr(window, Y_PRINT + 5, X_PRINT, getUsersCountServerResponse.content);
        wattroff(window, COLOR_PAIR(2));

        char ch = wgetch(window);
        return -1;
    }

    int usersCount = atoi(getUsersCountServerResponse.content) - 1;
    int usersPages = usersCount % 10 == 0 ? usersCount / 10 : usersCount / 10 + 1;

    char ch;
    if (usersPages != 0)
    {
        int currentPage = 1;

        do
        {
            wattron(window, COLOR_PAIR(2));
            mvwprintw(window, Y_PRINT + 25, X_PRINT + 30, "[B] Back");
            wattroff(window, COLOR_PAIR(2));

            // Print back button only if current page is not the first one
            if (currentPage > 1)
            {
                wattron(window, COLOR_PAIR(1));
                mvwprintw(window, Y_PRINT + 25, X_PRINT, "[Z] Previous");
                wattroff(window, COLOR_PAIR(1));
            }

            // Print next button only if more pages exists
            if (currentPage < usersPages)
            {
                wattron(window, COLOR_PAIR(1));
                mvwprintw(window, Y_PRINT + 25, X_PRINT + 15, "[X] Next");
                wattroff(window, COLOR_PAIR(1));
            }

            ServerResponse viewUsersServerResponse = SendViewUsersRequest(currentPage);
            if (viewUsersServerResponse.status != 200)
            {
                wattron(window, COLOR_PAIR(2));
                mvwaddstr(window, Y_PRINT + 4, X_PRINT, "Press any button to return.");
                mvwaddstr(window, Y_PRINT + 5, X_PRINT, viewUsersServerResponse.content);
                wattroff(window, COLOR_PAIR(2));

                char ch = wgetch(window);
                return -1;
            }

            int numOfUsers = 0;
            char **users = ParseContent(viewUsersServerResponse.content, &numOfUsers);

            free(viewUsersServerResponse.content);
            int user_Y_PRINT = Y_PRINT + 4;
            for (int i = 0; i < numOfUsers; i++)
            {
                int len = snprintf(NULL, 0, "[%d] %s", i, users[i]);
                if (len <= 0)
                {
                    wattron(window, COLOR_PAIR(2));
                    mvwprintw(window, user_Y_PRINT, X_PRINT, "Client Error");
                    wattroff(window, COLOR_PAIR(2));
                    break;
                }

                char *userRow = (char *)malloc(len + 1);
                snprintf(userRow, len + 1, "[%d] %s", i, users[i]);

                wattron(window, COLOR_PAIR(1));
                mvwprintw(window, user_Y_PRINT, X_PRINT, userRow);
                wattroff(window, COLOR_PAIR(1));

                free(userRow);
                user_Y_PRINT += 1;
            }

            ch = wgetch(window);
            while (1)
            {
                if (isalnum(ch))
                {
                    int digit = ch - '0';
                    if (digit >= 0 && digit < numOfUsers)
                    {
                        RenderSelectUserView(users[digit]);
                        break;
                    }

                    if ((ch == 'Z' || ch == 'z') && currentPage > 1)
                    {
                        currentPage -= 1;
                        break;
                    }

                    if ((ch == 'X' || ch == 'x') && currentPage < usersPages)
                    {
                        currentPage += 1;
                        break;
                    }

                    if (ch == 'B' || ch == 'b')
                    {
                        break;
                    }
                }

                ch = wgetch(window);
            }

            ClearRows(Y_PRINT + 4, Y_PRINT + 25);
            FreeParsedStrings(users, numOfUsers);
        } while (ch != 'B' && ch != 'b');
    }
    else
    {
        wattron(window, COLOR_PAIR(2));
        mvwprintw(window, Y_PRINT + 25, X_PRINT + 30, "[B] Back");
        wattroff(window, COLOR_PAIR(2));

        // When no page exists, wait for back command
        while (ch != 'B' && ch != 'b')
        {
            ch = wgetch(window);
        }
    }

    return 0;
}

char RenderSelectUserView(const char *selectedUser)
{
    wclear(window);
    box(window, 0, 0);

    const int Y_PRINT = 2;

    mvwprintw(window, Y_PRINT, X_PRINT + 16, "Conversation View");

    ServerResponse getMessageCountServerResponse = SendGetMessagesCountRequest(selectedUser);
    if (getMessageCountServerResponse.status != 200)
    {
        wattron(window, COLOR_PAIR(2));
        mvwaddstr(window, Y_PRINT + 4, X_PRINT, "Press any button to return.");
        mvwaddstr(window, Y_PRINT + 5, X_PRINT, getMessageCountServerResponse.content);
        wattroff(window, COLOR_PAIR(2));

        char ch = wgetch(window);
        return -1;
    }

    int messagesCount = atoi(getMessageCountServerResponse.content);
    int messagesPages = messagesCount % 10 == 0 ? messagesCount / 10 : messagesCount / 10 + 1;

    int currentPage = 1;
    char ch;
    do
    {
        wattron(window, COLOR_PAIR(1));
        mvwprintw(window, Y_PRINT + 23, X_PRINT, "[S] Send Message");
        wattroff(window, COLOR_PAIR(1));

        wattron(window, COLOR_PAIR(2));
        mvwprintw(window, Y_PRINT + 23, X_PRINT + 30, "[B] Back");
        wattroff(window, COLOR_PAIR(2));

        // Print back button only if current page is not the first one
        if (currentPage > 1)
        {
            wattron(window, COLOR_PAIR(1));
            mvwprintw(window, Y_PRINT + 25, X_PRINT, "[Z] Previous");
            wattroff(window, COLOR_PAIR(1));
        }

        // Print next button only if more pages exists
        if (currentPage < messagesPages)
        {
            wattron(window, COLOR_PAIR(1));
            mvwprintw(window, Y_PRINT + 25, X_PRINT + 30, "[X] Next");
            wattroff(window, COLOR_PAIR(1));
        }

        int numOfMessages = 0;
        char **messages = NULL;
        if (messagesCount > 0)
        {
            ServerResponse getMessagesServerResponse = SendViewMessagesRequest(currentPage, selectedUser);
            if (getMessagesServerResponse.status != 200)
            {
                wattron(window, COLOR_PAIR(2));
                mvwaddstr(window, Y_PRINT + 4, X_PRINT, "Press any button to return.");
                mvwaddstr(window, Y_PRINT + 5, X_PRINT, getMessagesServerResponse.content);
                wattroff(window, COLOR_PAIR(2));

                char ch = wgetch(window);
                return -1;
            }

            messages = ParseContent(getMessagesServerResponse.content, &numOfMessages);

            free(getMessagesServerResponse.content);
            int message_Y_PRINT = Y_PRINT + 5;
            for (int i = 0; i < numOfMessages; i++)
            {
                int len = snprintf(NULL, 0, "[%d] %s", i, messages[i]);
                if (len <= 0)
                {
                    wattron(window, COLOR_PAIR(2));
                    mvwprintw(window, message_Y_PRINT, X_PRINT, "Client Error");
                    wattroff(window, COLOR_PAIR(2));
                    break;
                }

                char *messageRow = (char *)malloc(len + 1);
                snprintf(messageRow, len + 1, "[%d] %s", i, messages[i]);

                wattron(window, COLOR_PAIR(1));
                mvwprintw(window, message_Y_PRINT, X_PRINT, messageRow);
                wattroff(window, COLOR_PAIR(1));

                free(messageRow);
                message_Y_PRINT += 1;
            }
        }

        ch = wgetch(window);
        while (1)
        {
            if (isalnum(ch))
            {
                int digit = ch - '0';
                if (digit >= 0 && digit < messagesPages)
                {
                    break;
                }

                if (ch == 'S' || ch == 's')
                {
                    wattron(window, COLOR_PAIR(1));
                    mvwprintw(window, Y_PRINT + 21, X_PRINT, "Write Message: ");
                    wattroff(window, COLOR_PAIR(1));

                    char message[1][100];
                    echo();
                    wgetstr(window, message[0]);
                    noecho();

                    int validationResult = ValidateUserInputs("Insert_Message", 1, message);
                    if (validationResult != 0)
                    {
                        wattron(window, COLOR_PAIR(2));
                        mvwaddstr(window, Y_PRINT + 3, X_PRINT, "Press any button to return.");
                        mvwaddstr(window, Y_PRINT + 4, X_PRINT, "Validation Failed");
                        wattroff(window, COLOR_PAIR(2));

                        wgetch(window);
                        break;
                    }

                    struct ServerResponse insertMessageServerResponse = SendInsertMessageRequest(selectedUser, message[0]);
                    if (insertMessageServerResponse.status != 201)
                    {
                        wattron(window, COLOR_PAIR(2));
                        mvwaddstr(window, Y_PRINT + 3, X_PRINT, "Press any button to return.");
                        mvwaddstr(window, Y_PRINT + 4, X_PRINT, insertMessageServerResponse.content);
                        wattroff(window, COLOR_PAIR(2));
                        wgetch(window);

                        break;
                    }

                    memset(message[0], 0, sizeof(message[0]));
                    ch = RenderSelectUserView(selectedUser);
                    break;
                }

                if ((ch == 'Z' || ch == 'z') && currentPage > 1)
                {
                    currentPage -= 1;
                    break;
                }

                if ((ch == 'X' || ch == 'x') && currentPage < messagesPages)
                {
                    currentPage += 1;
                    break;
                }

                if (ch == 'B' || ch == 'b')
                {
                    break;
                }
            }

            ch = wgetch(window);
        }

        ClearRows(Y_PRINT + 5, Y_PRINT + 25);
        FreeParsedStrings(messages, numOfMessages);

    } while (ch != 'B' && ch != 'b');

    return ch;
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
        if (strchr(userInputs[i], ':') != NULL || strchr(userInputs[i], '#') != NULL || strchr(userInputs[i], '|'))
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
    char serverResponse[8192];
    memset(serverResponse, 0, 8192);

    if (send(socketDescriptor, request, strlen(request), 0) <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 400;
        errorResponse.content = "[CLIENT][ERROR] Error at send()!\n";

        free(request);
        return errorResponse;
    }

    ssize_t noOfBytesRead = recv(socketDescriptor, serverResponse, 8192, 0);
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

ServerResponse SendViewUsersRequest(int currentPage)
{
    char *content = NULL;
    int len = snprintf(NULL, 0, "%s#%d#", loggedUsername, currentPage);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%d#", loggedUsername, currentPage);

    char *clientRequest = CreateClientRequest("View_Users", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

ServerResponse SendViewMessagesRequest(int currentPage, const char *selectedUser)
{
    char *content = NULL;
    int len = snprintf(NULL, 0, "%s#%s#%d#", loggedUsername, selectedUser, currentPage);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s#%d#", loggedUsername, selectedUser, currentPage);

    char *clientRequest = CreateClientRequest("View_Messages", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

ServerResponse SendGetMessagesCountRequest(const char *selectedUser)
{
    char *content = NULL;
    int len = snprintf(NULL, 0, "%s#%s#", loggedUsername, selectedUser);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s#", loggedUsername, selectedUser);

    char *clientRequest = CreateClientRequest("Get_Messages_Count", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

ServerResponse SendGetUsersCountRequest()
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

    char *clientRequest = CreateClientRequest("Get_Users_Count", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

ServerResponse SendInsertMessageRequest(const char *selectedUser, const char *message)
{
    char *content = NULL;
    int len = snprintf(NULL, 0, "%s#%s#%s#", loggedUsername, selectedUser, message);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s#%s#", loggedUsername, selectedUser, message);

    char *clientRequest = CreateClientRequest("Insert_Message", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}
// Helper Functions
void ClearRows(int startRow, int endRow)
{
    while (startRow <= endRow)
    {
        wmove(window, startRow, 1);
        wclrtoeol(window);
        startRow += 1;
    }
    box(window, 0, 0);
    wrefresh(window);
}
