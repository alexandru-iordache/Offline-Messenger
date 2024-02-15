# Offline Messenger

This repository contains a client-server application written in C. The application is a messenger, that persists the messages in a DB. 
The server can handle multiple clients concurrently. 
The client consists of a GUI rendered in the terminal.

## Description

To use the application the user has to log in or register. The username he wants to use must be unique and also a strong password is required.
After the user is logged in, he has access to view users, select one, and send a message. All the users that have been registered are visible to the logged-in user.

In the View Users menu, the user can view all the existent users on a view of 10 users per page, and for each one, the number of unread messages is shown. From this view, the user can go back, select a user he wants to communicate with, change the current page, or refresh the view.
In the conversation menu, the user can see 10 messages sent to/received from the selected user. The user can reply to a message (using the ID of the message) or view the entire content of a message. This view uses pagination too. The refresh button will reload the messages from the server, and for the unread ones, the text will be shown in red.


## Technical Approach

This section will cover some details about the implementation of the app.

### Server

The messages and user fields are saved in a SQLite DB.
For every run of the server, a log file is generated, where logs are written.
The server sends a response of type ServerReponse. (status code, content)

### Client

The GUI is written using the ncurses library.
The client sends a request of type ClientRequest. (authorized, command, content0
