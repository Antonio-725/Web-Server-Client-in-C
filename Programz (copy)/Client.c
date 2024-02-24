
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define REQUEST_SIZE 8192

char request[REQUEST_SIZE];

#define SIZE 4096
#define REQUEST_COUNT 10

// Forward declarations
void printRTT(struct timeval start, struct timeval end);
void printFileContents(const char *filename);

double getAverageRTT(char *url, char *port, int printRTTFlag)
{
    int clientSocket;
    struct sockaddr_in serverAddress;
    char response[SIZE];
    struct timeval start, end;
    double totalRTT = 0;
    char *statusCode; // Move the declaration outside the loop

    // Extract the hostname and path from the URL
    char hostname[SIZE], path[SIZE];
    strcpy(hostname, url);
    char *pathStart = strchr(hostname, '/');
    if (pathStart != NULL)
    {
        *pathStart = '\0'; // Null-terminate the hostname
        strcpy(path, pathStart + 1); // Copy the path part
    }
    else
    {
        strcpy(path, "/"); // Set path to root if no path provided
    }

    // Check if the URL length exceeds the maximum allowed
    if (strlen(path) + strlen(hostname) + 50 > REQUEST_SIZE)
    {
        fprintf(stderr, "Error: URL is too long.\n");
        return -1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct hostent *server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "Error: Unable to resolve host %s\n", hostname);
        return -1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(port));
    bcopy((char *)server->h_addr_list[0], (char *)&serverAddress.sin_addr.s_addr, server->h_length);

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "Error: Unable to connect to the server.\n");
        return -1;
    }

    snprintf(request, REQUEST_SIZE, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);

    for (int i = 0; i < REQUEST_COUNT; i++)
    {
        if (printRTTFlag)
        {
            gettimeofday(&start, NULL);
        }

        send(clientSocket, request, strlen(request), 0);

        recv(clientSocket, response, SIZE, 0);
        response[SIZE - 1] = '\0'; // Null-terminate the response buffer

        if (printRTTFlag)
        {
            gettimeofday(&end, NULL);
            printRTT(start, end);
        }

        // Parse HTTP response to extract status code
        char *statusLine = strtok(response, "\r\n");
        char *httpVersion = strtok(statusLine, " ");
        statusCode = strtok(NULL, " ");
        char *statusMessage = strtok(NULL, "\r\n");
        printf("%s %s %s\n", httpVersion, statusCode, statusMessage);

        // Save the page content to TMDG.html if the request was successful
        if (strcmp(statusCode, "200") == 0)
        {
            FILE *file = fopen("TMDG.html", "w");
            if (!file)
            {
                fprintf(stderr, "Error: Failed to create file.\n");
                return -1;
            }
            fprintf(file, "%s", response);
            fclose(file);
        }

        totalRTT += ((end.tv_sec - start.tv_sec) * 1000.0) + ((end.tv_usec - start.tv_usec) / 1000.0);

        usleep(100000); // Wait for 100 milliseconds before sending the next request
    }

    close(clientSocket);

    if (strcmp(statusCode, "200") == 0)
    {
        printf("Contents of TMDG.html:\n");
        printFileContents("TMDG.html");
    }

    return totalRTT / REQUEST_COUNT;
}

void printRTT(struct timeval start, struct timeval end)
{
    long seconds = end.tv_sec - start.tv_sec;
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    double millis = (double)micros / 1000;
    printf("Round Trip Time: %.3f\n", millis);
}

void printFileContents(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Error: Failed to open file.\n");
        return;
    }

    char buffer[SIZE];
    while (fgets(buffer, SIZE, file) != NULL)
    {
        printf("%s", buffer);
    }

    fclose(file);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s [-p] server_url port_number\n", argv[0]);
        return 1;
    }

    int printRTTFlag = 0;
    char *url, *port;

    if (strcmp(argv[1], "-p") == 0)
    {
        printRTTFlag = 1;
        url = argv[2];
        port = argv[3];
    }
    else
    {
        url = argv[1];
        port = argv[2];
    }

    double averageRTT = getAverageRTT(url, port, printRTTFlag);

    if (averageRTT < 0)
    {
        fprintf(stderr, "Error: Failed to get average RTT.\n");
        return 1;
    }

    printf("Average RTT for %s:%s is %.3f milliseconds\n", url, port, averageRTT);

    return 0;
}


























