#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 4474
#define MAX_BUFFER_SIZE 1024

// Function to send and receive messages from the storage server
void sendReceiveToSS(int ss_port, char *ss_ip, char *full_string)
{
    int ssSocket;
    struct sockaddr_in ssAddr;
    char buffer[MAX_BUFFER_SIZE];
    char message[MAX_BUFFER_SIZE];
    strcpy(message, full_string);

    // Create socket
    ssSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ssSocket < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    memset(&ssAddr, 0, sizeof(ssAddr));
    ssAddr.sin_family = AF_INET;
    ssAddr.sin_port = htons(ss_port);
    ssAddr.sin_addr.s_addr = inet_addr(ss_ip);

    // Connect to the storage server
    if (connect(ssSocket, (struct sockaddr *)&ssAddr, sizeof(ssAddr)) < 0)
    {
        perror("Error in connection to storage server");
        // exit(EXIT_FAILURE);
        return;
    }

    // store message in temp, tokenize it to find the command
    char temp[MAX_BUFFER_SIZE];
    strcpy(temp, message);
    char *token = strtok(temp, " ");

    if (strcmp(token, "read") == 0)
    {
        // Send the message to the storage server
        send(ssSocket, message, strlen(message), 0);

        // Receive and print the response from the storage server
        memset(buffer, 0, sizeof(buffer));
        recv(ssSocket, buffer, sizeof(buffer), 0);
        printf("Storage Server Response: %s\n", buffer);

        // Close the storage server socket
        close(ssSocket);
    }
    else if (strcmp(token, "write") == 0)
    {
        // Send the message to the storage server
        send(ssSocket, message, strlen(message), 0);

        // Receive and print the response from the storage server
        memset(buffer, 0, sizeof(buffer));
        recv(ssSocket, buffer, sizeof(buffer), 0);
        if (strcmp(buffer, "Send text") == 0)
        {
            printf("Enter what you want to write: ");
            char write_buffer[MAX_BUFFER_SIZE];
            fgets(write_buffer, MAX_BUFFER_SIZE, stdin);
            size_t ln = strlen(write_buffer);
            if (ln > 0 && write_buffer[ln - 1] == '\n')
            {
                write_buffer[ln - 1] = '\0';
            }
            printf("Writing %s to the file\n", write_buffer);
            send(ssSocket, write_buffer, strlen(write_buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            recv(ssSocket, buffer, sizeof(buffer), 0);
        }
        printf("Storage Server Response: %s\n", buffer);

        // Close the storage server socket
        close(ssSocket);
    }
    else if (strcmp(token, "get_info") == 0)
    {
        // Send the message to the storage server
        send(ssSocket, message, strlen(message), 0);

        memset(buffer, 0, sizeof(buffer));
        recv(ssSocket, buffer, sizeof(buffer), 0);
        // tokenize the buffer into two parts where the first part is the size and the second part is the permissions but do this only if the first character of the buffer is a digit
        if (isdigit(buffer[0]))
        {
            char *token = strtok(buffer, " ");
            char *size = token;
            token = strtok(NULL, " ");
            char *permissions = token;
            printf("Storage Server Response: Size: %s, Permissions: %s\n", size, permissions);
        }
        else
        {
            printf("Storage Server Response: %s\n", buffer);
        }
        // Close the storage server socket
        close(ssSocket);
    }
    else
    {
        printf("Invalid command\n");
    }
}

void process_function(char *full_string, int clientSocket)
{
    // make a copy of full_string
    char *full_string_copy = (char *)malloc(strlen(full_string) + 1);
    strcpy(full_string_copy, full_string);

    // get the first token of full_string_copy
    char *token = strtok(full_string_copy, " ");

    // if the first token is "create" or "delete" then send the full string to the server
    if (!strcmp(token, "create") || !strcmp(token, "delete"))
    {
        send(clientSocket, full_string, strlen(full_string), 0); // send the whole command to the NS
        // recieve message back from the server
        char buffer[MAX_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("Server: %s\n", buffer);

        // clear the buffer for the next iteration
        memset(buffer, 0, sizeof(buffer));
    }

    // if the first token is "read" then send the full string to the server and receive back the ss_number from the server
    else if (!strcmp(token, "read") || !strcmp(token, "write") || !strcmp(token, "get_info"))
    {
        send(clientSocket, full_string, strlen(full_string), 0);
        char buffer[MAX_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("Server: %s\n", buffer);
        if (strncmp(buffer, "Path does not exist", 19) == 0)
        {
            return;
        }

        int sspno;
        // convert the buffer to an int
        sspno = atoi(buffer);

        printf("Enter the above storage server port number for authentication: ");
        scanf("%d", &sspno);
        while ((getchar()) != '\n')
            ;
        sendReceiveToSS(sspno, "127.0.1.1", full_string);
    }

    else if (!strcmp(token, "copy"))
    {
        char buffer[MAX_BUFFER_SIZE], message[MAX_BUFFER_SIZE], path1[100], path2[100];
        // send the full string to the server
        send(clientSocket, full_string, strlen(full_string), 0);

        // receive message regarding the choice
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);

        printf("Naming Server: %s\n", buffer);

        // receive the paths through input i the form of <path1> <path2>
        printf("Enter the source path: ");
        scanf("%s", path1);
        printf("Enter the destination path: ");
        scanf("%s", path2);
        // flush input buffer
        while ((getchar()) != '\n')
            ;
        // merge both the paths in "message" separated by a space
        strcpy(message, path1);
        strcat(message, " ");
        strcat(message, path2);
        printf("Message: %s\n", message);
        send(clientSocket, message, strlen(message), 0);

        // also receive error message
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("Naming Server: %s\n", buffer);
        // printf("Why are you theeeeeere\n");
    }

    else
    {
        printf("Invalid command\n");
    }
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.1.1");

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error in connection");
        exit(EXIT_FAILURE);
    }

    // Receive message from the server
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0); // this is receiving the "Hi client" message
    printf("Server: %s\n", buffer);                // printing the "Hi client" message
    while (1)
    {
        printf("Enter message to send to server: ");
        fgets(buffer, sizeof(buffer), stdin);

        process_function(buffer, clientSocket);

        // Clear the buffer for the next iteration
        memset(buffer, 0, sizeof(buffer));
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}