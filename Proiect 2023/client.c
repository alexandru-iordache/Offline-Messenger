#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

#define PORT 8989

void listenMessagesFromServer(int socketDescriptor);

int main()
{
  int socketDescriptor;
  struct sockaddr_in serverStructure;

  socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socketDescriptor == -1)
  {
    printf("[client]Eroare la crearea socketului!\n");
    return 0;
  }

  serverStructure.sin_family = AF_INET;
  serverStructure.sin_port = htons(PORT);
  serverStructure.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(socketDescriptor, (struct sockaddr *)&serverStructure, sizeof(struct sockaddr)) == -1)
  {
    printf("[client]Eroare la connect().\n");
    return 0;
  }

  printf("[Client]Conexiunea a reusit!\n");

  pthread_t id;
  pthread_create(&id, NULL, (void *)listenMessagesFromServer, socketDescriptor);

  char *buffer = NULL;
  size_t bufferSize = 0;
  while (1)
  {
    ssize_t numberOfChars = getline(&buffer, &bufferSize, stdin);

    if (send(socketDescriptor, buffer, numberOfChars, 0) <= 0)
    {
      printf("[Client]Eroare la trimiterea mesajului catre server.\n");
      return 0;
    }
  }
}

void listenMessagesFromServer(int socketDescriptor)
{
  char *buffer = malloc(sizeof(char) * 1024);

  while (1)
  {
    ssize_t noOfBytesRead = recv(socketDescriptor, buffer, 1024, 0);
    if (noOfBytesRead < 0)
    {
      printf("[Client]Eroare la primirea mesajului de la server.\n");
      return;
    }
    else if (noOfBytesRead > 0)
    {
      buffer[noOfBytesRead - 1] = 0;
      printf("[Server]%s\n", buffer);
    }
    bzero(buffer, 1024);
  }
}