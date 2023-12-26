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

//UI constants
#define X_PRINT 14

//SOCKET constants
#define PORT 8989
#define ADDRESS "127.0.0.1"

//CLIENT variables
unsigned short int authorized = 0;

int renderFirstPage();

int main() {
    // UI settings
    initscr();
    cbreak(); // Disable line buffering
    keypad(stdscr, TRUE); // Enable special keys

    // Set up colors if supported
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
    }

    int socketDescriptor;
    struct sockaddr_in serverSocketStructure;

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1)
    {
        attron(COLOR_PAIR(2));
        mvprintw(4, X_PRINT, "[CLIENT][ERROR] Socket creation error!\n");
        attroff(COLOR_PAIR(2));
        return -1;
    }

    serverSocketStructure.sin_family = AF_INET;
    serverSocketStructure.sin_port = htons(PORT);
    serverSocketStructure.sin_addr.s_addr = inet_addr(ADDRESS);

    if (connect(socketDescriptor, (struct sockaddr *)&serverSocketStructure, sizeof(struct sockaddr)) == -1)
    {
        attron(COLOR_PAIR(2));
        mvprintw(4, X_PRINT, "[CLIENT][ERROR] Error at connect()!\n");
        attroff(COLOR_PAIR(2));
        return -1;
    }

    renderFirstPage();

    // Clean up and exit
    endwin();
    return 0;
}

int renderFirstPage()
{
    attron(COLOR_PAIR(1));
    mvprintw(4, X_PRINT, "[1] Login");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(1));
    mvprintw(6, X_PRINT, "[2] Register");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2));
    mvprintw(8, X_PRINT, "[Q] Quit");
    attroff(COLOR_PAIR(2));

    int ch;
    while ((ch = getch()) != 'q') {
        // Process button presses
        if (ch == '1' || ch == '1') {
            attron(COLOR_PAIR(1));
            mvprintw(10, X_PRINT, "Login Button Pressed!");

            char username[50], password[50];
            mvprintw(12, X_PRINT, "Enter Username: ");
            getstr(username);
            
            noecho(); 
            mvprintw(13, X_PRINT, "Enter Password: ");
            getstr(password);
            echo();

            attroff(COLOR_PAIR(1));

        } else if (ch == '2' || ch == '2') {
            attron(COLOR_PAIR(2));
            mvprintw(10, X_PRINT, "Register Button Pressed!");
            attroff(COLOR_PAIR(2));
        }

        // Refresh the screen
        //refresh();
    }
}