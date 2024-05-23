#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include "try.c"

// #define STORAGE_SERVER_PORT 5555
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define PORT 4407

pthread_mutex_t file_mutex[100];

int create(char *path)
{
    // save the cwd so that you can change back to it after completing the create
    char currentDir[100];
    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        return -3;
    }

    char temp[100];
    strcpy(temp, path);

    char *token = strtok(temp, "/");
    char fileName[100];
    strcpy(fileName, token);
    while (token != NULL)
    {
        strcpy(fileName, token);
        token = strtok(NULL, "/");
    }

    // remove newline character at the end of the fileName
    // fileName[strlen(fileName) - 1] = '\0';

    char *lastSlash = strrchr(path, '/');
    *lastSlash = '\0';

    if (strcmp(path, "") != 0)
    {
        if (chdir(path) != 0)
        {
            return -2;
        }
    }

    int isDir = 0;
    // check if 'fileName' has a dot in it. If no, then set isDir to 1
    if (strchr(fileName, '.') == NULL)
    {
        isDir = 1;
    }

    // if isDir is 1, then make a folder in the current working directory called fileName
    if (isDir == 1)
    {
        if (mkdir(fileName, 0777) != 0)
        {
            chdir(currentDir);
            return -1;
        }
    }
    else
    {
        FILE *fp = fopen(fileName, "w");
        if (fp == NULL)
        {
            chdir(currentDir);
            return -1;
        }
    }

    if (chdir(currentDir) != 0)
    {
        return -3;
    }

    return 0;
}

char *get_path_seek(char *token, char *name)
{
    char *path = (char *)malloc(4096 * sizeof(char));
    strcpy(path, token);
    strcat(path, "/");
    strcat(path, name);
    return path;
}

int recDelete(char *fileName, char *currentDir)
{
    DIR *dir;
    struct dirent *entry;
    struct stat st;

    if ((dir = opendir(fileName)) == NULL)
    {
        chdir(currentDir);
        return -1;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        if (stat(get_path_seek(fileName, entry->d_name), &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                recDelete(get_path_seek(fileName, entry->d_name), currentDir);
            }
            else
            {
                remove(get_path_seek(fileName, entry->d_name));
            }
        }
    }
    rmdir(fileName);
    return 0;
}

int delete(char *path)
{
    char currentDir[100];
    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        return -3;
    }
    char temp[100];
    strcpy(temp, path);

    char *token = strtok(temp, "/");
    char fileName[100];
    strcpy(fileName, token);
    while (token != NULL)
    {
        strcpy(fileName, token);
        token = strtok(NULL, "/");
    }

    // fileName[strlen(fileName) - 1] = '\0';
    // check if there is a newline character anywhere in the fileName. If yes, then remove it
    for (int i = 0; i < strlen(fileName); i++)
    {
        if (fileName[i] == '\n')
        {
            fileName[i] = '\0';
        }
    }

    char *lastSlash = strrchr(path, '/');
    if (*(lastSlash - 1) == '.')
        *(lastSlash + 1) = '\0'; // this was giving error, so handling this edge case
    else
        *(lastSlash) = '\0'; // the general case

    if (strcmp(path, "") != 0)
    {
        if (chdir(path) != 0)
        {
            return -2;
        }
    }

    int isDir = 0;
    // check if 'fileName' has a dot in it. If no, then set isDir to 1
    if (strchr(fileName, '.') == NULL)
    {
        isDir = 1;
    }

    // if isDir is 1, then remove the folder in the current working directory called fileName, else remove the file called fileName
    if (isDir == 1)
    {
        if (recDelete(fileName, currentDir))
        {
            chdir(currentDir);
            return -1;
        }
    }
    else
    {
        if (remove(fileName) != 0)
        {
            chdir(currentDir);
            return -1;
        }
    }

    if (chdir(currentDir) != 0)
    {
        return -3;
    }

    return 0;
}

void getIPAddress(char *ipBuffer, size_t bufferSize)
{
    char hostname[256];
    struct hostent *host_info;

    gethostname(hostname, sizeof(hostname));
    host_info = gethostbyname(hostname);

    if (host_info != NULL)
    {
        inet_ntop(AF_INET, host_info->h_addr_list[0], ipBuffer, bufferSize);
    }
    else
    {
        perror("Error getting IP address");
        exit(EXIT_FAILURE);
    }
}

void *handleClient(void *arg)
{
    int s2Socket = *((int *)arg);
    char buffer[MAX_BUFFER_SIZE];
    if (listen(s2Socket, MAX_CLIENTS) == 0)
    {
        printf("Listening...\n\n");
    }
    else
    {
        perror("Error in listening");
        exit(EXIT_FAILURE);
    }
    int clientSocket;
    struct sockaddr_in cliAddr;
    socklen_t addr_size = sizeof(cliAddr);

    // Accept client connection
    clientSocket = accept(s2Socket, (struct sockaddr *)&cliAddr, &addr_size);
    if (clientSocket < 0)
    {
        perror("Error in accepting");
        exit(EXIT_FAILURE);
    }

    printf("Connection accepted from %s:%d\n",
           inet_ntoa(cliAddr.sin_addr),
           ntohs(cliAddr.sin_port));

    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    // send(clientSocket, message, strlen(message), 0);
    printf("Command from client: %s\n", buffer);
    // tokenize it to find if the first word is read or write or get_info
    char temp[100];
    strcpy(temp, buffer);
    char *token = strtok(temp, " ");
    int status = 0;
    printf("token: --%s--\n", token);
    if (strcmp(token, "read") == 0)
    {
        printf("read command\n");
        // get present working directory
        char currentDir[100];
        if (getcwd(currentDir, sizeof(currentDir)) == NULL)
        {
            // perror("Error getting current working directory");
            status = -3;
        }
        // get the path from temp
        token = strtok(NULL, " ");
        char path[100];
        strcpy(path, token);
        char tempPath[100];
        strcpy(tempPath, path);
        printf("path: --%s--\n", path);
        // remove new line character from anywhere of path
        for (int i = 0; i < strlen(path); i++)
            if (path[i] == '\n')
                path[i] = '\0';

        // get the file name from path
        char fileName[100];
        char *lastSlash = strrchr(path, '/');
        strcpy(fileName, lastSlash + 1);
        printf("fileName: --%s--\n", fileName);
        // remove the file name from the path
        *lastSlash = '\0';
        printf("new path: --%s--\n", path);
        // go to the directory where the file is to be created
        // if chdir is not possible, print error
        // if new path is not empty then change directory
        if (strcmp(path, "") != 0)
        {
            if (chdir(path) != 0)
            {
                // perror("Error changing directory");
                status = -2;
            }
        }
        // read the file and store it somewhere in buffer and send it to client
        if (status != -2 && status != -3)
        {
            FILE *fp;
            fp = fopen(fileName, "r");
            if (fp == NULL)
            {
                // perror("Error opening file");
                status = -1;
            }
            else
            {
                // read the file and store it in buffer
                printf("Reading file\n");
                char fileBuffer[MAX_BUFFER_SIZE];
                memset(fileBuffer, 0, sizeof(fileBuffer));
                int bytesRead = fread(fileBuffer, 1, sizeof(fileBuffer), fp);
                if (bytesRead < 0)
                {
                    // perror("Error reading file");
                    status = -1;
                }
                else
                {
                    // send the buffer to client
                    printf("Sending file to client\n");
                    printf("File contents: %s\n", fileBuffer);
                    send(clientSocket, fileBuffer, sizeof(fileBuffer), 0);
                    status = 0;
                }
                fclose(fp);
            }
        }
        // change back to initial directory
        if (chdir(currentDir) != 0)
        {
            // perror("Error changing back to initial directory");
            status = -3;
        }
        // send the status to client
        printf("Sending status to client\n");
        if (status == -1)
        {
            send(clientSocket, "File not found", sizeof("File not found"), 0);
        }
        else if (status == -2)
        {
            send(clientSocket, "Directory not found", sizeof("Directory not found"), 0);
        }
        else if (status == -3)
        {
            send(clientSocket, "Error changing directory", sizeof("Error changing directory"), 0);
        }
    }
    if (strcmp(token, "get_info") == 0)
    {
        // retrieve information about the file
        printf("get_info command\n");
        // get present working directory
        char currentDir[100];
        if (getcwd(currentDir, sizeof(currentDir)) == NULL)
        {
            // perror("Error getting current working directory");
            status = -3;
        }
        // get the path from temp
        token = strtok(NULL, " ");
        char path[100];
        strcpy(path, token);
        char tempPath[100];
        strcpy(tempPath, path);
        printf("path: --%s--\n", path);
        // remove new line character from the end of path
        // path[strlen(path) - 1] = '\0';
        if (path[strlen(path) - 1] == '\n')
        {
            path[strlen(path) - 1] = '\0';
        }
        // get the file name from path
        char fileName[100];
        char *lastSlash = strrchr(path, '/');
        strcpy(fileName, lastSlash + 1);
        printf("fileName: --%s--\n", fileName);
        // remove the file name from the path
        *lastSlash = '\0';
        printf("new path: --%s--\n", path);
        // go to the directory where the file is to be created
        // if chdir is not possible, print error
        // if new path is not empty then change directory
        if (strcmp(path, "") != 0)
        {
            if (chdir(path) != 0)
            {
                // perror("Error changing directory");
                status = -2;
            }
        }
        // read the file and store it somewhere in buffer and send it to client
        if (status != -2 && status != -3)
        {
            FILE *fp;
            fp = fopen(fileName, "r");
            if (fp == NULL)
            {
                // perror("Error opening file");
                status = -1;
            }
            else
            {
                // get size and permissions
                struct stat st;
                stat(fileName, &st);
                int size = st.st_size;
                // int permissions = st.st_mode;
                mode_t perms = st.st_mode;

                // send the size and permissions to client
                printf("Sending size and permissions to client\n");
                // print the size and permissions
                printf("Size: %d\n", size);
                printf("Permissions: %d\n", perms);

                // get the permissions in a string of format rwxrwxrwx
                char permissions[10];
                permissions[0] = (perms & S_IRUSR) ? 'r' : '-';
                permissions[1] = (perms & S_IWUSR) ? 'w' : '-';
                permissions[2] = (perms & S_IXUSR) ? 'x' : '-';
                permissions[3] = (perms & S_IRGRP) ? 'r' : '-';
                permissions[4] = (perms & S_IWGRP) ? 'w' : '-';
                permissions[5] = (perms & S_IXGRP) ? 'x' : '-';
                permissions[6] = (perms & S_IROTH) ? 'r' : '-';
                permissions[7] = (perms & S_IWOTH) ? 'w' : '-';
                permissions[8] = (perms & S_IXOTH) ? 'x' : '-';
                permissions[9] = '\0';

                // send both size and permissions as a string
                char sizeAndPermissions[100];
                sprintf(sizeAndPermissions, "%d %s", size, permissions);
                send(clientSocket, sizeAndPermissions, sizeof(sizeAndPermissions), 0);
                status = 0;

                fclose(fp);
            }
        }
        // change back to initial directory
        if (chdir(currentDir) != 0)
        {
            // perror("Error changing back to initial directory");
            status = -3;
        }
        // send the status to client
        printf("Sending status to client\n");
        if (status == -1)
        {
            send(clientSocket, "File not found", sizeof("File not found"), 0);
        }
        else if (status == -2)
        {
            send(clientSocket, "Directory not found", sizeof("Directory not found"), 0);
        }
        else if (status == -3)
        {
            send(clientSocket, "Error changing directory", sizeof("Error changing directory"), 0);
        }
    }
    if (strcmp(token, "write") == 0)
    {
        // write into the file
        printf("write command\n");
        char currentDir[100];
        if (getcwd(currentDir, sizeof(currentDir)) == NULL)
        {
            // perror("Error getting current working directory");
            status = -3;
        }
        // get the path from temp
        token = strtok(NULL, " ");
        char path[100];
        strcpy(path, token);
        char tempPath[100];
        strcpy(tempPath, path);
        printf("path: --%s--\n", path);
        // remove new line character from the end of path
        // path[strlen(path) - 1] = '\0';
        if (path[strlen(path) - 1] == '\n')
        {
            path[strlen(path) - 1] = '\0';
        }
        // copy the path given to a temp variable called use for lock
        char use_for_lock[100];
        strcpy(use_for_lock, path);
        // get the file name from path
        char fileName[100];
        char *lastSlash = strrchr(path, '/');
        strcpy(fileName, lastSlash + 1);
        printf("fileName: --%s--\n", fileName);
        // remove the file name from the path
        *lastSlash = '\0';
        printf("new path: --%s--\n", path);
        // go to the directory where the file is to be created
        // if chdir is not possible, print error
        // if new path is not empty then change directory
        if (strcmp(path, "") != 0)
        {
            if (chdir(path) != 0)
            {
                // perror("Error changing directory");
                status = -2;
            }
        }

        // calculate a hash for the file path to determine the mutex index
        unsigned int hash = 0;
        printf("use for lock path: %s", use_for_lock);
        for (int i = 0; i < strlen(use_for_lock); i++)
        {
            // make hash function simple like sum of all characters in the path
            // hash += use_for_lock[i];
            hash = (hash * 31) + use_for_lock[i];
            // printf("path[i]: %c, hash: %d\n", use_for_lock[i], hash);
        }
        int mutexIndex = hash % 100;

        // printf("Mutex index: %d\n", mutexIndex);

        pthread_mutex_lock(&file_mutex[mutexIndex]);

        if (status != -2 && status != -3)
        // read the file and store it somewhere in buffer and send it to client
        {
            FILE *fp;
            fp = fopen(fileName, "w");
            if (fp == NULL)
            {
                // perror("Error opening file");
                status = -1;
            }
            else
            {
                // recieve the text to be written into file from client
                // char text[1000];
                // send(clientSocket, "Send text", sizeof("Send text"), 0);
                // recv(clientSocket, text, sizeof(text), 0);
                // printf("Text recieved: --%s--\n", text);
                // // write the text into the file
                // fprintf(fp, "%s", text);

                // Send a prompt to the client to start sending text
                send(clientSocket, "Send text", sizeof("Send text"), 0);

                // Receive the text to be written into the file from client
                char text[1000];
                recv(clientSocket, text, sizeof(text), 0);
                printf("Text recieved: --%s--\n", text);

                // Write the entire received text into the file
                fprintf(fp, "%s", text);

                fclose(fp);
                status = 0;
            }
        }

        // change back to initial directory
        if (chdir(currentDir) != 0)
        {
            // perror("Error changing back to initial directory");
            status = -3;
        }
        pthread_mutex_unlock(&file_mutex[mutexIndex]);
        // send the status to client
        printf("Sending status to client\n");
        if (status == -1)
        {
            send(clientSocket, "File not found", sizeof("File not found"), 0);
        }
        else if (status == -2)
        {
            send(clientSocket, "Directory not found", sizeof("Directory not found"), 0);
        }
        else if (status == -3)
        {
            send(clientSocket, "Error changing directory", sizeof("Error changing directory"), 0);
        }
        else if (status == 0)
        {
            send(clientSocket, "File written successfully", sizeof("File written successfully"), 0);
        }
    }

    return NULL;
}

int main()
{
    int serverSocket, STORAGE_SERVER_PORT, CLIENT_COMMUNICATION_PORT, s2Socket, clcnt = 0;
    struct sockaddr_in serverAddr, s2Addr;
    char buffer[MAX_BUFFER_SIZE];
    char ip[16];
    getIPAddress(ip, sizeof(ip));

    printf("Enter the Port Number: ");
    scanf("%d", &STORAGE_SERVER_PORT);
    printf("Enter the Client Communication Port Number: ");
    scanf("%d", &CLIENT_COMMUNICATION_PORT);
    printf("\n The IP address of this SS is: %s", ip);
    // Create socket for NM
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    // Create socket for Clients
    s2Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (s2Socket < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(STORAGE_SERVER_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    memset(&s2Addr, 0, sizeof(s2Addr));
    s2Addr.sin_family = AF_INET;
    s2Addr.sin_port = htons(CLIENT_COMMUNICATION_PORT);
    s2Addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket for NM connections
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error in binding 1");
        exit(EXIT_FAILURE);
    }

    // Bind the socket for Client connections
    if (bind(s2Socket, (struct sockaddr *)&s2Addr, sizeof(s2Addr)) < 0)
    {
        perror("Error in binding 2");
        exit(EXIT_FAILURE);
    }

    // // Listen for connections
    // if (listen(serverSocket, 1) == 0)
    // {
    //     printf("Storage Server Listening on port %d...\n", STORAGE_SERVER_PORT);
    // }
    // else
    // {
    //     perror("Error in listening");
    //     exit(EXIT_FAILURE);
    // }

    // // Accept a client connection
    // socklen_t addrSize = sizeof(clientAddr);
    // clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize);
    // if (clientSocket < 0)
    // {
    //     perror("Error in accepting");
    //     exit(EXIT_FAILURE);
    // }

    // printf("Storage Server: Connection accepted from %s:%d\n",
    //        inet_ntoa(clientAddr.sin_addr),
    //        ntohs(clientAddr.sin_port));

    int clientSocket;
    struct sockaddr_in NMserverAddr;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    memset(&NMserverAddr, 0, sizeof(NMserverAddr));
    NMserverAddr.sin_family = AF_INET;
    NMserverAddr.sin_port = htons(PORT);
    NMserverAddr.sin_addr.s_addr = inet_addr("127.0.1.1");

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&NMserverAddr, sizeof(NMserverAddr)) < 0)
    {
        perror("Error in connection");
        exit(EXIT_FAILURE);
    }

    printf("\nConnected to the Naming Server :)\n");

    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    printf("Storage Server: Message from Naming Server: %s\n", buffer);

    // send the port number, the cport number and the ip address to the naming server as a single string
    // char port[10], cport[10];
    char port[200], cport[200], Curr_Dir[200];
    getcwd(Curr_Dir, sizeof(Curr_Dir));

    sprintf(port, "%d", STORAGE_SERVER_PORT);
    sprintf(cport, "%d", CLIENT_COMMUNICATION_PORT);
    strcat(port, " ");
    strcat(port, cport);
    strcat(port, " ");
    strcat(port, ip);
    strcat(port, " ");
    strcat(port, Curr_Dir);
    send(clientSocket, port, strlen(port), 0);

    printf("Enter 0 if connecting for the first time or enter 1\n");
    int choice;
    scanf("%d", &choice);
    if (choice == 0)
    {
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("Storage Server: Message from Naming Server: %s\n", buffer);

        while (strcmp(buffer, "STOP") != 0)
        {
            scanf("%s", buffer);
            send(clientSocket, buffer, strlen(buffer), 0);
        }
    }
    pthread_t thread[MAX_CLIENTS];
    // Receive and print messages from the naming server
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        // message recieved is in buffer
        char *token = strtok(buffer, " ");
        char command[10];
        char path[100];
        strcpy(command, token);
        if (strcmp(command, "create") == 0)
        {
            token = strtok(NULL, " ");
            strcpy(path, token);
            printf("Storage Server: Create command recieved for path: %s\n", path);

            int ack = create(path);
            if (ack == -1)
            {
                char message[] = "Error creating file";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
            else if (ack == 0)
            {
                char message[] = "File/folder created successfully!";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
            else if (ack == -2)
            {
                char message[] = "Error changing directory as it might not be present";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
            else if (ack == -3)
            {
                char message[] = "Error changing back to initial directory";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
        }
        // similarly make delete file
        if (strcmp(command, "delete") == 0)
        {
            token = strtok(NULL, " ");
            strcpy(path, token);
            printf("Storage Server: Delete command recieved for path: %s\n", path);

            int ack = delete (path);
            if (ack == -1)
            {
                char message[] = "Error deleting file";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
            else if (ack == 0)
            {
                char message[] = "File/folder deleted successfully!";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
            else if (ack == -2)
            {
                char message[] = "Error changing directory as it might not be present";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
            else if (ack == -3)
            {
                char message[] = "Error changing back to initial directory";
                printf("Storage Server: %s\n", message);
                send(clientSocket, message, strlen(message), 0);
            }
        }

        if (strcmp(command, "read") == 0 || strcmp(command, "write") == 0 || strcmp(command, "get_info") == 0)
        {
            if (pthread_create(&thread[clcnt], NULL, handleClient, &s2Socket) != 0)
            {
                perror("Error in creating thread");
                exit(EXIT_FAILURE);
            }
        }
        // if (strcmp(command, "path") == 0)
        // {
        //     if (pthread_create(&thread[clcnt], NULL, handleClient, &s2Socket) != 0)
        //     {
        //         perror("Error in creating thread");
        //         exit(EXIT_FAILURE);
        //     }
        // }
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
