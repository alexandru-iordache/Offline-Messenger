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

int Y_MAX, X_MAX;

// UI functions
int RenderFirstPage();
char RenderLoginView();
char RenderRegisterView();

int RenderSecondPage();
char RenderViewUsersView();
char RenderSelectUserView(const char *selectedUser);

int RenderSendMessageComponent(const char *selectedUser, const int replyId, const int Y_PRINT);
int RenderViewMessageView(const struct MessageStructure messageObject, const char *selectedUser);

// Helper Functions
char *CreatePrintRow(struct MessageStructure messageObject, int i);
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
ServerResponse SendInsertMessageRequest(const char *selectedUser, const char *message, int replyId);
ServerResponse SendUpdateMessageReadRequest(struct MessageStructure *messageObjects, int numOfMessages);

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

    getmaxyx(stdscr, Y_MAX, X_MAX);

    window = newwin(Y_MAX / 1.5, X_MAX / 2, Y_MAX / 4, X_MAX / 4);

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
            break;
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
    wgetnstr(window, userInputs[0], 49);
    noecho();

    mvwprintw(window, Y_PRINT + 4, X_PRINT, "Enter Password: ");
    wgetnstr(window, userInputs[1], 49);

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
    mvwaddstr(window, Y_PRINT + 1, X_PRINT, serverResponse.content);
    wattroff(window, COLOR_PAIR(2));

    for (int i = 0; i < 2; i++)
    {
        memset(userInputs, 0, strlen(userInputs[i]));
    }

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
    wgetnstr(window, userInputs[0], 49);

    mvwprintw(window, Y_PRINT + 4, X_PRINT, "Enter First Name: ");
    wgetnstr(window, userInputs[1], 49);

    mvwprintw(window, Y_PRINT + 5, X_PRINT, "Enter Last Name: ");
    wgetnstr(window, userInputs[2], 49);

    noecho();

    mvwprintw(window, Y_PRINT + 6, X_PRINT, "Enter Password: ");
    wgetnstr(window, userInputs[3], 49);

    mvwprintw(window, Y_PRINT + 7, X_PRINT, "Confirm Password: ");
    wgetnstr(window, userInputs[4], 49);

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
    mvwaddstr(window, Y_PRINT + 1, X_PRINT, serverResponse.content);
    wattroff(window, COLOR_PAIR(2));

    for (int i = 0; i < 5; i++)
    {
        memset(userInputs[i], 0, strlen(userInputs[i]));
    }

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
    wattroff(window, COLOR_PAIR(1));

    wattron(window, COLOR_PAIR(2));
    mvwprintw(window, Y_PRINT + 6, X_PRINT, "[L] Logout");
    mvwprintw(window, Y_PRINT + 8, X_PRINT, "[Q] Quit");
    wattroff(window, COLOR_PAIR(2));

    char ch = wgetch(window);
    while (1)
    {
        if (ch == '1')
        {
            RenderViewUsersView();
            break;
        }

        if (ch == 'L' || ch == 'l')
        {
            authorized = 0;
            loggedUsername = NULL;
            return 0;
        }

        if (ch == 'Q' || ch == 'q')
        {
            return -1;
        }

        ch = wgetch(window);
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

            wattron(window, COLOR_PAIR(1));
            mvwprintw(window, Y_PRINT + 25, X_PRINT + 15, "[R] Refresh");
            wattroff(window, COLOR_PAIR(1));

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

                wgetch(window);
                return -1;
            }

            int numOfUsers = 0;
            char **users = ParseContent(viewUsersServerResponse.content, &numOfUsers);

            free(viewUsersServerResponse.content);

            int user_Y_PRINT = Y_PRINT + 4;
            struct UserViewStructure userObjects[numOfUsers];
            for (int i = 0; i < numOfUsers; i++)
            {
                userObjects[i] = ParseUserViewStructure(users[i]);

                int len = snprintf(NULL, 0, "[%d] %s [%d Unread Messages]", i,
                                   userObjects[i].username, userObjects[i].unreadMessagesCount);
                if (len <= 0)
                {
                    wattron(window, COLOR_PAIR(2));
                    mvwprintw(window, user_Y_PRINT, X_PRINT, "Client Error");
                    wattroff(window, COLOR_PAIR(2));

                    wgetch(window);
                    break;
                }

                char *userRow = (char *)malloc(len + 1);
                snprintf(userRow, len + 1, "[%d] %s [%d Unread Messages]", i,
                         userObjects[i].username, userObjects[i].unreadMessagesCount);

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
                        RenderSelectUserView(userObjects[digit].username);
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

                    if ((ch == 'R') || ch == 'r')
                    {
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
        } while (ch != 'B' && ch != 'b' && ch != 'R' && ch != 'r');
    }
    else
    {
        wattron(window, COLOR_PAIR(2));
        mvwprintw(window, Y_PRINT + 25, X_PRINT + 30, "[B] Back");
        wattroff(window, COLOR_PAIR(2));

        wattron(window, COLOR_PAIR(1));
        mvwprintw(window, Y_PRINT + 25, X_PRINT + 15, "[R] Refresh");
        wattroff(window, COLOR_PAIR(1));

        // When no page exists, wait for back command or refresh
        while (ch != 'B' && ch != 'b' && ch != 'R' && ch != 'r')
        {
            ch = wgetch(window);
        }
    }

    if (ch == 'R' || ch == 'r')
    {
        RenderViewUsersView();
    }

    return 0;
}

char RenderSelectUserView(const char *selectedUser)
{
    wclear(window);
    box(window, 0, 0);

    const int Y_PRINT = 2;

    mvwprintw(window, Y_PRINT, X_PRINT + 12, "Conversation View");

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

        wattron(window, COLOR_PAIR(1));
        mvwprintw(window, Y_PRINT + 25, X_PRINT + 15, "[R] Refresh");
        wattroff(window, COLOR_PAIR(1));

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
        struct MessageStructure *messageObjects = NULL;
        if (messagesCount > 0)
        {
            wattron(window, COLOR_PAIR(1));
            mvwaddstr(window, Y_PRINT + 3, X_PRINT, "Press message digit to view entire message");
            wattroff(window, COLOR_PAIR(1));

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

            messageObjects = (struct MessageStructure *)malloc(numOfMessages * sizeof(struct MessageStructure));
            int message_Y_PRINT = Y_PRINT + 5;
            for (int i = numOfMessages - 1; i >= 0; i--)
            {
                messageObjects[i] = ParseMessage(messages[i]);

                int len = snprintf(NULL, 0, "[%d][ID: %d] %s: %s", i, messageObjects[i].id,
                                   messageObjects[i].sender, messageObjects[i].message);

                char *messageRow = CreatePrintRow(messageObjects[i], i);
                if (messageRow == NULL)
                {
                    wattron(window, COLOR_PAIR(2));
                    mvwprintw(window, message_Y_PRINT, X_PRINT, "Client Error");
                    wattroff(window, COLOR_PAIR(2));

                    wgetch(window);
                    break;
                }

                int messageColor = messageObjects[i].read == 0 && strcmp(messageObjects[i].sender, loggedUsername) != 0 ? 2 : 1;
                wattron(window, COLOR_PAIR(messageColor));
                wmove(window, message_Y_PRINT, X_PRINT);
                waddnstr(window, messageRow, X_MAX / 2 - X_PRINT - 1);
                wattroff(window, COLOR_PAIR(messageColor));

                free(messageRow);
                message_Y_PRINT += 1;
            }

            struct ServerResponse updateMessageReadServerResponse = SendUpdateMessageReadRequest(messageObjects, numOfMessages);
            if (updateMessageReadServerResponse.status != 200)
            {
                wattron(window, COLOR_PAIR(2));
                mvwaddstr(window, Y_PRINT + 3, X_PRINT, "Press any button to return.");
                mvwaddstr(window, Y_PRINT + 4, X_PRINT, "Validation Failed");
                wattroff(window, COLOR_PAIR(2));

                wgetch(window);
                ClearRows(Y_PRINT + 3, Y_PRINT + 4);
            }
        }

        ch = wgetch(window);
        while (1)
        {
            if (isalnum(ch))
            {
                int digit = ch - '0';
                if (digit >= 0 && digit < numOfMessages)
                {
                    RenderViewMessageView(messageObjects[digit], selectedUser);

                    ch = RenderSelectUserView(selectedUser);
                    break;
                }

                if (ch == 'S' || ch == 's')
                {
                    ClearRows(Y_PRINT + 23, Y_PRINT + 25);

                    int result = RenderSendMessageComponent(selectedUser, -1, Y_PRINT);
                    if (result != 0)
                    {
                        break;
                    }

                    ch = RenderSelectUserView(selectedUser);
                    break;
                }

                if (ch == 'R' || ch == 'r')
                {
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

        ClearRows(Y_PRINT + 3, Y_PRINT + 25);
        FreeParsedStrings(messages, numOfMessages);
        if (messageObjects != NULL)
        {
            free(messageObjects);
        }
    } while (ch != 'B' && ch != 'b');

    return ch;
}

int RenderViewMessageView(const struct MessageStructure messageObject, const char *selectedUser)
{
    wclear(window);
    box(window, 0, 0);

    const int Y_PRINT = 2;

    mvwprintw(window, Y_PRINT, X_PRINT + 12, "Message View");

    wattron(window, COLOR_PAIR(2));
    mvwprintw(window, Y_PRINT + 25, X_PRINT + 30, "[B] Back");
    wattroff(window, COLOR_PAIR(2));

    wattron(window, COLOR_PAIR(1));
    mvwprintw(window, Y_PRINT + 25, X_PRINT + 15, "[R] Reply");
    wattroff(window, COLOR_PAIR(1));

    int len = snprintf(NULL, 0, "%s: %s", messageObject.sender, messageObject.message);
    if (len <= 0)
    {
        wattron(window, COLOR_PAIR(2));
        mvwprintw(window, Y_PRINT + 4, X_PRINT, "Client Error");
        wattroff(window, COLOR_PAIR(2));

        wgetch(window);
        return -1;
    }

    char *messageRow = (char *)malloc(len + 1);
    snprintf(messageRow, len + 1, "%s: %s", messageObject.sender, messageObject.message);

    int currentY = Y_PRINT + 4;
    int currentX = X_PRINT - 2;
    wattron(window, COLOR_PAIR(1));
    for (int i = 0; i < len + 1; i++)
    {
        if (currentX + 1 >= X_MAX / 2 - 1)
        {
            currentX = X_PRINT - 2;
            currentY += 1;
        }

        mvwprintw(window, currentY, currentX, "%c", messageRow[i]);
        currentX += 1;
    }
    wattroff(window, COLOR_PAIR(1));

    free(messageRow);

    char ch = wgetch(window);
    while (1)
    {
        if (isalnum(ch))
        {
            if (ch == 'B' || ch == 'b')
            {
                return 0;
            }

            if (ch == 'R' || ch == 'r')
            {
                ClearRows(Y_PRINT + 23, Y_PRINT + 25);

                int result = RenderSendMessageComponent(selectedUser, messageObject.id, Y_PRINT);
                if (result != 0)
                {
                    break;
                }

                return 0;
            }
        }
        ch = wgetch(window);
    }
}

int RenderSendMessageComponent(const char *selectedUser, const int replyId, const int Y_PRINT)
{
    wattron(window, COLOR_PAIR(1));
    mvwprintw(window, Y_PRINT + 21, X_PRINT, "Write Message: ");
    wattroff(window, COLOR_PAIR(1));

    char *message = (char *)malloc(1000);
    echo();
    wgetnstr(window, message, 999);
    noecho();

    struct ServerResponse insertMessageServerResponse = SendInsertMessageRequest(selectedUser, message, replyId);
    if (insertMessageServerResponse.status != 201)
    {
        ClearRows(Y_PRINT + 3, Y_PRINT + 3);

        wattron(window, COLOR_PAIR(2));
        mvwaddstr(window, Y_PRINT + 3, X_PRINT, "Press any button to return.");
        mvwaddstr(window, Y_PRINT + 4, X_PRINT, insertMessageServerResponse.content);
        wattroff(window, COLOR_PAIR(2));

        wgetch(window);
        free(message);
        ClearRows(Y_PRINT + 3, Y_PRINT + 4);
        return -1;
    }

    free(message);

    return 0;
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
    else if (noOfBytesRead == 0)
    {
        exit(0);
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
        errorResponse.status = 0;

        switch (validationCode)
        {
        case 1:
            errorResponse.content = "The inputs should not be empty.";
            break;
        case 2:
            errorResponse.content = "The inputs should not contain \":\", \"|\" or \"#\".";
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
        errorResponse.status = 0;

        switch (validationCode)
        {
        case 1:
            errorResponse.content = "The inputs should not be empty.";
            break;
        case 2:
            errorResponse.content = "The inputs should not contain \":\", \"|\" or \"#\".";
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

ServerResponse SendInsertMessageRequest(const char *selectedUser, const char *message, int replyId)
{
    if (message == NULL || strlen(message) == 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "The inputs should not be empty.";

        return errorResponse;
    }

    if (strchr(message, ':') != NULL || strchr(message, '#') != NULL || strchr(message, '|'))
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "The inputs should not contain \":\", \"|\" or \"#\".";

        return errorResponse;
    }

    char *content = NULL;
    int len = snprintf(NULL, 0, "%s#%s#%s#%d#", loggedUsername, selectedUser, message, replyId);

    if (len <= 0)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    content = (char *)malloc(len + 1);
    snprintf(content, len + 1, "%s#%s#%s#%d#", loggedUsername, selectedUser, message, replyId);

    char *clientRequest = CreateClientRequest("Insert_Message", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

ServerResponse SendUpdateMessageReadRequest(struct MessageStructure *messageObjects, int numOfMessages)
{
    int contentLength = 0;
    for (int i = 0; i < numOfMessages; i++)
    {
        if (messageObjects[i].read == 0 && strcmp(messageObjects[i].sender, loggedUsername) != 0)
        {
            int rowLength = snprintf(NULL, 0, "%d#", messageObjects[i].id);
            if (rowLength <= 0)
            {
                struct ServerResponse errorResponse;
                errorResponse.status = 0;
                errorResponse.content = "Client Internal Error";
                return errorResponse;
            }
            contentLength += rowLength;
        }
    }

    if (contentLength == 0)
    {
        struct ServerResponse serverResponse;
        serverResponse.status = 200;
        serverResponse.content = "No message to update";
        return serverResponse;
    }

    char *content = (char *)malloc(contentLength + 1);
    if (content == NULL)
    {
        struct ServerResponse errorResponse;
        errorResponse.status = 0;
        errorResponse.content = "Client Internal Error";
        return errorResponse;
    }

    int offset = 0;
    for (int i = 0; i < numOfMessages; i++)
    {
        if (messageObjects[i].read == 0 && strcmp(messageObjects[i].sender, loggedUsername) != 0)
        {
            offset += snprintf((content + offset), contentLength - offset + 1, "%d#", messageObjects[i].id);
            if (offset <= 0)
            {
                struct ServerResponse errorResponse;
                errorResponse.status = 0;
                errorResponse.content = "Client Internal Error";
                return errorResponse;
            }
        }
    }

    char *clientRequest = CreateClientRequest("Update_Message_Read", content, authorized);

    free(content);
    return SendRequest(clientRequest);
}

// Helper Functions
char *CreatePrintRow(struct MessageStructure messageObject, int i)
{
    int len;
    if (messageObject.replyId != -1)
    {
        len = snprintf(NULL, 0, "[%d][ID: %d][RE: %d] %s: %s", i, messageObject.id, messageObject.replyId,
                       messageObject.sender, messageObject.message);
    }
    else
    {
        len = snprintf(NULL, 0, "[%d][ID: %d] %s: %s", i, messageObject.id,
                       messageObject.sender, messageObject.message);
    }
    if (len <= 0)
    {
        return NULL;
    }

    char *messageRow = (char *)malloc(len + 1);
    if (messageObject.replyId != -1)
    {
        snprintf(messageRow, len + 1, "[%d][ID: %d][RE: %d] %s: %s", i, messageObject.id, messageObject.replyId,
                 messageObject.sender, messageObject.message);
    }
    else
    {
        snprintf(messageRow, len + 1, "[%d][ID: %d] %s: %s", i, messageObject.id,
                 messageObject.sender, messageObject.message);
    }

    return messageRow;
}

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
