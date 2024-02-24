#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE 1024

void handleClient(int clientSocket)
{
    char request[SIZE], response[SIZE];
    char *fileContent;
    FILE *file;

    if (recv(clientSocket, request, SIZE, 0) < 0)
    {
        fprintf(stderr, "Error: Failed to read request from client.\n");
        return;
    }

    char method[10], path[100];
    sscanf(request, "%s %s", method, path);

    if (strcmp(method, "GET") != 0)
    {
        sprintf(response, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        send(clientSocket, response, strlen(response), 0);
        return;
    }

    if (access(path + 1, F_OK) == -1)
    {
        sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\nFile not found.\n");
        send(clientSocket, response, strlen(response), 0);
        return;
    }

    file = fopen(path + 1, "r");
    if (!file)
    {
        fprintf(stderr, "Error: Failed to open file.\n");
        sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        send(clientSocket, response, strlen(response), 0);
        return;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    fileContent = (char *)malloc(fsize);
    fread(fileContent, fsize, 1, file);
    fclose(file);

    // Determine Content-Type based on file extension
    const char *ext = strrchr(path + 1, '.');
    const char *contentType = "text/plain";
    if (ext != NULL)
    {
        if (strcmp(ext, ".html") == 0)
        {
            contentType = "text/html";
        }
        else if (strcmp(ext, ".txt") == 0)
        {
            contentType = "text/plain";
        }
        
    }

    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n%s", fsize, contentType, fileContent);
    send(clientSocket, response, strlen(response), 0);
    free(fileContent);
}

void sigintHandler(int sig)
{
    printf("Shutting down server.\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port_number\n", argv[0]);
        return 1;
    }

    int serverSocket, clientSocket, port;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientLength = sizeof(clientAddress);

    port = atoi(argv[1]);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        fprintf(stderr, "Error: Failed to create server socket.\n");
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "Error: Failed to bind server socket.\n");
        return 1;
    }

    if (listen(serverSocket, 5) < 0)
    {
        fprintf(stderr, "Error: Failed to listen on server socket.\n");
        return 1;
    }

    signal(SIGINT, sigintHandler);

    printf("Server running on port %d...\n", port);

    while (1)
    {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLength);
        if (clientSocket < 0)
        {
            fprintf(stderr, "Error: Failed to accept client connection.\n");
            continue;
        }

        // Print a message when a new client connects
        char clientAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientAddr, INET_ADDRSTRLEN);
        printf("Client connected: %s\n", clientAddr);

        handleClient(clientSocket);

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}









