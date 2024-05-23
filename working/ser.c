#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "try.c"

#define PORT 4407
#define CPORT 4474
#define MAX_CLIENTS 10
#define MAX_SS 5
#define NUM_INIT_SS 2
#define ALPHABET_SIZE 26
#define MAX_PATH_LENGTH 51
#define LRU_CACHE_SIZE 5

int cnt, ss = 2;
struct Trie *trie;

// LRU implementation
struct LRU_Node
{
    struct Paths *data;
    struct LRU_Node *next;
};

struct LRU_Cache
{
    struct LRU_Node *head;
    int count;
};

// Function to initialize a new LRU cache
struct LRU_Cache *createLRUCache()
{
    struct LRU_Cache *newCache = malloc(sizeof(struct LRU_Cache));
    if (newCache != NULL)
    {
        newCache->head = NULL;
        newCache->count = 0;
    }
    return newCache;
}

// initialize an lru cache
struct LRU_Cache *lruCache;

// Function to search for a Paths struct in the LRU cache
struct Paths *searchLRUCache(struct LRU_Cache *cache, const char *path)
{
    printf("Searching LRU Cache\n");
    printf("path: --%s--\n", path);
    struct LRU_Node *currentNode = cache->head;
    while (currentNode != NULL)
    {
        if (strcmp(currentNode->data->path, path) == 0)
        {
            // Move the accessed path to the front of the LRU cache
            if (currentNode != cache->head)
            {
                struct LRU_Node *temp = currentNode;
                cache->head = currentNode->next;
                temp->next = NULL;
                currentNode->next = cache->head;
                cache->head = temp;
            }
            printf("Path: --%s--, Storage Number: %d\n", currentNode->data->path, currentNode->data->storageNumber);
            return currentNode->data;
        }
        currentNode = currentNode->next;
    }

    // If the path is not found in the LRU cache, return NULL
    printf("Path not found in LRU Cache\n");
    return NULL;
}

// Function to insert a Paths struct into the LRU cache
void insertLRUCache(struct LRU_Cache *cache, struct Paths *path)
{
    printf("Inserting into LRU Cache\n");
    printf("Path:--%s--, Storage Number: %d\n", path->path, path->storageNumber);
    if (cache->count == LRU_CACHE_SIZE)
    {
        printf("LRU Cache is full\n");
        // Remove the least recently used path
        struct LRU_Node *temp = cache->head;
        while (temp->next->next != NULL)
        {
            temp = temp->next;
        }
        struct LRU_Node *last = temp->next;
        temp->next = NULL;
        free(last);
        cache->count--;
    }

    // Insert the new path at the front of the LRU cache
    struct LRU_Node *newNode = malloc(sizeof(struct LRU_Node));
    newNode->data = path;
    newNode->next = cache->head;
    cache->head = newNode;
    cache->count++;
    printf("Inserted! Path: --%s--, Storage Number: %d\n", newNode->data->path, newNode->data->storageNumber);
}

void deleteLRUCache(struct LRU_Cache *cache, const char *path)
{
    printf("Deleting from LRU Cache\n");
    printf("path: --%s--\n", path);
    struct LRU_Node *currentNode = cache->head;
    struct LRU_Node *prevNode = NULL;
    while (currentNode != NULL)
    {
        if (strcmp(currentNode->data->path, path) == 0)
        {
            if (prevNode == NULL)
            {
                cache->head = currentNode->next;
            }
            else
            {
                prevNode->next = currentNode->next;
            }
            free(currentNode);
            cache->count--;
            printf("Deleted!\n");
            return;
        }
        prevNode = currentNode;
        currentNode = currentNode->next;
    }
}

// Function to initialize a new Paths struct
struct Paths *createPaths(const char *path, int storageNumber)
{
    printf("Creating new Paths struct\n");
    printf("Path:--%s--, Storage Number: %d\n", path, storageNumber);
    struct Paths *newPath = malloc(sizeof(struct Paths));
    strncpy(newPath->path, path, sizeof(newPath->path) - 1);
    newPath->path[sizeof(newPath->path) - 1] = '\0'; // Ensure null-termination
    newPath->storageNumber = storageNumber;
    return newPath;
}

struct StorageServer
{
    char ip[16];
    int port;
    int cport;
    int socket;
    int alive;
    char wd[100];
};

struct StorageServer SS[MAX_SS];

void connectToSS()
{
    for (int i = 0; i < MAX_SS; ++i)
    {
        // Create socket
        SS[i].socket = socket(AF_INET, SOCK_STREAM, 0);
        if (SS[i].socket < 0)
        {
            perror("Error in socket");
            exit(EXIT_FAILURE);
        }

        // Set up server address structure
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SS[i].port);
        serverAddr.sin_addr.s_addr = inet_addr(SS[i].ip);

        // Connect to storage server
        if (connect(SS[i].socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Error in connecting to storage server");
            exit(EXIT_FAILURE);
        }

        printf("Connected to Storage Server %d at %s:%d\n", i + 1, SS[i].ip, SS[i].port);
    }
}

void *Dyna_SS(void *arg)
{
    int sockfd = *((int *)arg);
    struct sockaddr_in cliAddr;
    socklen_t addr_size = sizeof(cliAddr);
    char message[] = "Hello Storage Server!\n";
    // Accept client connection
    SS[ss].socket = accept(sockfd, (struct sockaddr *)&cliAddr, &addr_size);
    printf("HERE\n");
    if (SS[ss].socket < 0)
    {
        perror("Error in accepting");
        exit(EXIT_FAILURE);
    }

    printf("Storage Server connection accepted from %s:%d\n",
           inet_ntoa(cliAddr.sin_addr),
           ntohs(cliAddr.sin_port));

    send(SS[ss].socket, message, strlen(message), 0);
    // receive the port number, the cport number and the ip address from the storage server as a single string
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    recv(SS[ss].socket, buffer, sizeof(buffer), 0);
    // tokenize the buffer to get the port number, the cport number and the ip address
    char *token = strtok(buffer, " ");
    SS[ss].port = atoi(token);
    token = strtok(NULL, " ");
    SS[ss].cport = atoi(token);
    token = strtok(NULL, " ");
    strcpy(SS[ss].ip, token);
    token = strtok(NULL, " ");
    strcpy(SS[ss].wd, token);
    printf("The port number is %d and the cport number is %d and the ip address is %s\n", SS[ss].port, SS[ss].cport, SS[ss].ip);
    int ftime = 0;
    for (int p = 0; p < ss; p++)
    {
        if (SS[p].port == SS[ss].port)
        {
            ftime = 1;
            SS[p].alive = 1;
            SS[p].socket = SS[ss].socket;
            break;
        }
    }
    if (!ftime)
    {
        // send the message "Enter the list of accessible paths and respond with <STOP> when done" to the storage server
        char message1[] = "Enter the list of accessible paths and respond with <STOP> when done";
        send(SS[ss].socket, message1, strlen(message1), 0);
        // continue to receive the paths from the storage server until the storage server sends the message "STOP"
        while (strcmp(buffer, "STOP") != 0)
        {
            memset(buffer, 0, sizeof(buffer));
            recv(SS[ss].socket, buffer, sizeof(buffer), 0);
            if (strcmp(buffer, "STOP") != 0)
            {
                insert(trie, buffer, ss);
            }
            printf("%s\n", buffer);
        }
        ss++;
    }
    return NULL;
}

void *handleClient(void *arg)
{
    int clientSocket = *((int *)arg);
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Send a confirmation message to the client
    send(clientSocket, "Hi client", strlen("Hi client"), 0);

    while (1)
    {
        // Receive data from the client
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0); // rn buffer contains the command
        if (bytesRead <= 0)
        {
            // Either the client disconnected or an error occurred
            if (bytesRead == 0)
            {
                // Client disconnected
                printf("Client disconnected 1\n");
                cnt--;
            }
            else
            {
                perror("Error receiving data from client");
            }

            pthread_exit(NULL);
            return NULL;
        }

        // Process received data or perform other tasks with the client
        printf("Received message from client: %s\n", buffer); // echo the command you just got from the client in full

        // buffer has the data recieved from the client
        // tokenize the buffer to get the command and the path, sometimes 1 path is given but sometimes 2 paths are given
        // if 1 path is given, then the command is either "read" or "delete" or "create" or "append"
        // if 2 paths are given, then the command is "copy and paste"
        char *token = strtok(buffer, " ");
        if (token == NULL)
        {
            printf("No valid command given.\n");
            return NULL;
        }

        char command[10];
        char path1[100];
        char path2[100];
        strcpy(command, token);

        if (strcmp(command, "read") == 0 || strcmp(command, "write") == 0 || strcmp(command, "get_info") == 0)
        {
            // printf("\nYUP\n");
            token = strtok(NULL, " ");
            strcpy(path1, token);
            printf("path1: --%s--\n", path1);

            // if (path1[strlen(path1) - 1] == '\n')
            // {
            //     path1[strlen(path1) - 1] = '\0';
            // }
            for (int i = 0; i < strlen(path1); i++)
                if (path1[i] == '\n')
                    path1[i] = '\0';

            // send the path to a search for storage server function which will return the storage server number
            int ss;
            struct Paths *result = (struct Paths *)malloc(sizeof(struct Paths));
            result = searchLRUCache(lruCache, path1);
            if (result != NULL)
            {
                ss = result->storageNumber;
            }
            else
            {
                ss = search(trie, path1);
                if (ss >= 0)
                {
                    struct Paths *newPath = (struct Paths *)malloc(sizeof(struct Paths));
                    newPath = createPaths(path1, ss);
                    insertLRUCache(lruCache, newPath);
                }
            }

            printf("ss: %d\n", ss);

            if (ss < 0)
            {
                printf("Path does not exist or is inaccessible.\n");
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Path does not exist or is inaccessible.");
                send(clientSocket, buffer, strlen(buffer), 0);
                continue;
            }

            int ss_port = 0;
            // get the cport of the ss
            ss_port = SS[ss].cport;

            printf("ss_port: %d\n", ss_port);

            // send ss_port to the client
            char ss_string[10];
            memset(ss_string, 0, sizeof(ss_string));
            sprintf(ss_string, "%d", ss_port);
            send(clientSocket, ss_string, strlen(ss_string), 0);

            // send the command and path to the storage server together
            char message[100];
            strcpy(message, command);
            strcat(message, " ");
            strcat(message, path1);
            send(SS[ss].socket, message, strlen(message), 0);
            printf("The port of the storage server is %d\n", SS[ss].port);
        }

        else if (strcmp(command, "create") == 0)
        {
            token = strtok(NULL, " ");
            strcpy(path1, token); // this is the path including the actual file name to be created

            // check if there is a newline character anywhere in path1. if there is, remove it. iterate through it for it
            for (int i = 0; i < strlen(path1); i++)
                if (path1[i] == '\n')
                    path1[i] = '\0';

            // check if the path already exists in the trie
            int ss_check = search(trie, path1);
            if (ss_check >= 0)
            {
                printf("File/Folder already exists.\n");
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "File/Folder already exists.\n");
                send(clientSocket, buffer, strlen(buffer), 0);
                continue;
            }

            char path_to_find_ss[100]; // remove the file name, so that you can search for the place in which you put the new file in the trie
            strcpy(path_to_find_ss, path1);

            char *lastSlash = strrchr(path_to_find_ss, '/');
            if (*(lastSlash - 1) == '.')
                *(lastSlash + 1) = '\0'; // this was giving error, so handling this edge case
            else
                *(lastSlash) = '\0'; // the general case

            printf("path_to_find_ss: %s\n", path_to_find_ss);

            int ss;
            int flag = 0;
            // struct Paths *result = searchLRUCache(lruCache, path_to_find_ss);
            struct Paths *result = (struct Paths *)malloc(sizeof(struct Paths));
            result = searchLRUCache(lruCache, path_to_find_ss);
            if (result != NULL)
            {
                ss = result->storageNumber;
                flag = 1;
            }
            else
            {
                // look for that path in the trie and get the storageNumber
                ss = search(trie, path_to_find_ss);
            }

            if (ss < 0) // if the path is not found in the trie, then send an error message to the client
            {
                printf("Path does not exist or is inaccessible.\n");

                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Path does not exist or is inaccessible.\n");
                send(clientSocket, buffer, strlen(buffer), 0);
                continue;
            }

            char message[1024];
            memset(message, 0, sizeof(message));
            strcpy(message, command);
            strcat(message, " ");
            strcat(message, path1);
            printf("Sent message to storage server %d: %s\n", ss, message);
            send(SS[ss].socket, message, strlen(message), 0); // tell the storage server what the command is and what the path is

            // recieve the message from the storage server
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytesRead = recv(SS[ss].socket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
            {
                printf("SS disconnected\n");
                SS[ss].alive = 0;
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Storage server is currently offline.\n");
                send(clientSocket, buffer, strlen(buffer), 0);
                continue;
                // Either the client disconnected or an error occurred
                // if (bytesRead == 0)
                // {
                //     // Client disconnected
                //     printf("S disconnected 2\n");
                //     cnt--;
                // }
                // else
                // {
                //     perror("Error receiving data from client");
                // }

                // pthread_exit(NULL);
                // return NULL;
            }

            // print the message
            printf("Received message from storage server %d: %s\n", ss, buffer);

            if (strcmp(buffer, "File/folder created successfully!") == 0)
            {
                insert(trie, path1, ss);
                printf("Path --%s-- inserted into trie\n", path1);
                if (flag == 0)
                {
                    struct Paths *newPath = (struct Paths *)malloc(sizeof(struct Paths));
                    newPath = createPaths(path1, ss);
                    insertLRUCache(lruCache, newPath);
                }
            }
            // send the message from the storage server to the respective client - communicating success or failure
            send(clientSocket, buffer, strlen(buffer), 0);
        }

        else if (strcmp(command, "delete") == 0)
        {
            token = strtok(NULL, " ");
            strcpy(path1, token); // this is the path including the file name to be deleted

            char path_to_find_ss[100];
            strcpy(path_to_find_ss, path1);

            char *lastSlash = strrchr(path_to_find_ss, '/');

            if (*(lastSlash - 1) == '.')
                *(lastSlash + 1) = '\0';
            else
                *(lastSlash) = '\0';

            printf("path_to_find_ss: --%s--\n", path_to_find_ss);
            printf("path1: --%s--\n", path1);

            // if newline character is there in path1, remove it
            for (int i = 0; i < strlen(path1); i++)
                if (path1[i] == '\n')
                    path1[i] = '\0';

            printf("path1 after edit: --%s--\n", path1);

            struct Paths *result = (struct Paths *)malloc(sizeof(struct Paths));
            result = searchLRUCache(lruCache, path1);
            int ss;
            int flag = 0;
            if (result != NULL)
            {
                ss = result->storageNumber;
                flag = 1;
            }
            else
            {
                ss = search(trie, path1);
            }

            if (ss < 0)
            {
                printf("Path does not exist or is inaccessible.\n");
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Path does not exist or is inaccessible.\n");
                send(clientSocket, buffer, strlen(buffer), 0);
                continue;
            }

            // send the command and path to the storage server together
            char message[100];
            strcpy(message, command);
            strcat(message, " ");
            strcat(message, path1);
            send(SS[ss].socket, message, strlen(message), 0);

            // recieve the message from the storage server
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytesRead = recv(SS[ss].socket, buffer, sizeof(buffer), 0);

            if (bytesRead <= 0)
            {
                printf("SS disconnected\n");
                SS[ss].alive = 0;
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Storage server is currently offline.\n");
                send(clientSocket, buffer, strlen(buffer), 0);
                continue;
                // Either the client disconnected or an error occurred
                // if (bytesRead == 0)
                // {
                //     // Client disconnected
                //     printf("S disconnected 2\n");
                //     cnt--;
                // }
                // else
                // {
                //     perror("Error receiving data from client");
                // }

                // pthread_exit(NULL);
                // return NULL;
            }

            // print the message
            printf("Received message from storage server %d: %s\n", ss, buffer);

            if (strcmp(buffer, "File/folder deleted successfully!") == 0)
            {
                // delete the path from the trie
                deletion(&trie, path1);
                printf("Path deleted from trie\n");
                if (flag == 1)
                {
                    deleteLRUCache(lruCache, path1);
                }
            }
            // send the buffer message to the respective client - communicating success or failure to client
            send(clientSocket, buffer, strlen(buffer), 0);
        }

        else if (strcmp(command, "copy") == 0)
        {
            char choice[10];
            token = strtok(NULL, " ");
            strcpy(choice, token);

            if (strncmp(choice, "file", 4) == 0)
            {
                printf("processing file\n");
                // sending the prompt to enter the paths
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Enter: <source path> first and then next <destination path>\n");
                send(clientSocket, buffer, strlen(buffer), 0);

                // receive paths
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                printf("Received: %s\n", buffer);
                // tokenize the buffer to get the paths
                token = strtok(buffer, " ");
                strcpy(path1, token);

                // get the string after the last slash in path1
                char my_filename[100];
                strcpy(my_filename, path1);
                char *lastSlash = strrchr(my_filename, '/');
                strcpy(my_filename, lastSlash + 1);

                char path_to_find_ss[100];
                strcpy(path_to_find_ss, path1);

                lastSlash = strrchr(path_to_find_ss, '/');

                if (*(lastSlash - 1) == '.')
                    *(lastSlash + 1) = '\0';
                else
                    *(lastSlash) = '\0';

                int ss1;
                struct Paths *result1 = (struct Paths *)malloc(sizeof(struct Paths));
                result1 = searchLRUCache(lruCache, path1);

                if (result1 != NULL)
                {
                    ss1 = result1->storageNumber;
                }
                else
                {
                    ss1 = search(trie, path1);
                    if (ss1 >= 0)
                    {
                        struct Paths *newPath = (struct Paths *)malloc(sizeof(struct Paths));
                        newPath = createPaths(path1, ss1);
                        insertLRUCache(lruCache, newPath);
                    }
                }

                printf("ss1: %d\n", ss1);
                if (ss1 < 0)
                {
                    printf("IN SS1 <0");
                    // send path does not exist
                    memset(buffer, 0, sizeof(buffer));
                    strcpy(buffer, "Source path does not exist or is inaccessible.\n");
                    send(clientSocket, buffer, strlen(buffer), 0);
                    continue;
                }
                // store the difference between path1 and path_to_find_ss
                char path1_diff[100];
                strcpy(path1_diff, path1 + strlen(path_to_find_ss));
                // remove the first character from path1_diff
                strcpy(path1_diff, path1_diff + 1);

                token = strtok(NULL, " ");
                strcpy(path2, token);

                int ss2;
                struct Paths *result2 = (struct Paths *)malloc(sizeof(struct Paths));
                result2 = searchLRUCache(lruCache, path2);
                if (result2 != NULL)
                {
                    ss2 = result2->storageNumber;
                }
                else
                {
                    ss2 = search(trie, path2);
                    if (ss2 >= 0)
                    {
                        struct Paths *newPath = (struct Paths *)malloc(sizeof(struct Paths));
                        newPath = createPaths(path2, ss2);
                        insertLRUCache(lruCache, newPath);
                    }
                }

                if (ss2 < 0)
                {
                    // send path does not exist
                    memset(buffer, 0, sizeof(buffer));
                    strcpy(buffer, "Destination path does not exist or is inaccessible.\n");
                    send(clientSocket, buffer, strlen(buffer), 0);
                    continue;
                }
                char path2backup[100];
                strcpy(path2backup, path2);

                memset(buffer, 0, sizeof(buffer));
                // remove the first two characters from the start of the path_to_ss string
                strcpy(buffer, path1 + 1);
                // concatenate SS[ss1].wd to the buffer
                memset(path1, 0, sizeof(path1));
                strcpy(path1, SS[ss1].wd);
                // strcat(path1, "/");
                strcat(path1, buffer);
                printf("\nPath1: %s\n", path1);

                printf("path1: --%s--\n", path1);
                printf("path2: --%s--\n", path2);

                // same for the second path
                memset(buffer, 0, sizeof(buffer));
                // remove the first two characters from the start of the path_to_ss string
                strcpy(buffer, path2 + 1);
                // concatenate SS[ss1].wd to the buffer
                memset(path2, 0, sizeof(path2));
                strcpy(path2, SS[ss2].wd);
                // strcat(path2, "/");
                strcat(path2, buffer);
                printf("\nPath2: %s\n", path2);

                pid_t pid = fork();

                if (pid == -1)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0)
                { // Child process
                    // Use execlp to execute the cp command
                    execlp("cp", "cp", path1, path2, NULL);

                    // If execlp fails
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }
                else
                { // Parent process
                    // Wait for the child process to complete
                    int status;
                    waitpid(pid, &status, 0);

                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                    {
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, "File copied successfully.\n");
                        // concatenate path1_diff to path2backup
                        strcat(path2backup, "/");
                        strcat(path2backup, my_filename);
                        // check if path2backup has two consecutive '/' characters
                        char *p = strstr(path2backup, "//");
                        // if yes, then remove one of them which are in the middle of the string
                        if (p != NULL)
                        {
                            strcpy(p, p + 1);
                        }

                        insert(trie, path2backup, ss2);
                        send(clientSocket, buffer, strlen(buffer), 0);
                    }
                    else
                    {
                        fprintf(stderr, "Error copying file.\n");
                    }
                }
            }
            if (strncmp(choice, "folder", 6) == 0)
            {
                printf("processing file\n");
                // sending the prompt to enter the paths
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Enter: <source path> <destination path>\n");
                send(clientSocket, buffer, strlen(buffer), 0);

                // receive paths
                memset(buffer, 0, sizeof(buffer));
                recv(clientSocket, buffer, sizeof(buffer), 0);
                printf("Received: %s\n", buffer);
                // tokenize the buffer to get the paths
                token = strtok(buffer, " ");
                strcpy(path1, token);

                int ss1;
                struct Paths *result1 = (struct Paths *)malloc(sizeof(struct Paths));
                result1 = searchLRUCache(lruCache, path1);

                if (result1 != NULL)
                {
                    ss1 = result1->storageNumber;
                }
                else
                {
                    ss1 = search(trie, path1);
                    if (ss1 >= 0)
                    {
                        struct Paths *newPath = (struct Paths *)malloc(sizeof(struct Paths));
                        newPath = createPaths(path1, ss1);
                        insertLRUCache(lruCache, newPath);
                    }
                }

                printf("ss1: %d\n", ss1);
                if (ss1 < 0)
                {
                    printf("IN SS1 <0");
                    // send path does not exist
                    memset(buffer, 0, sizeof(buffer));
                    strcpy(buffer, "Source path does not exist or is inaccessible.\n");
                    send(clientSocket, buffer, strlen(buffer), 0);
                    continue;
                }

                token = strtok(NULL, " ");
                strcpy(path2, token);

                int ss2;
                struct Paths *result2 = (struct Paths *)malloc(sizeof(struct Paths));
                result2 = searchLRUCache(lruCache, path2);
                if (result2 != NULL)
                {
                    ss2 = result2->storageNumber;
                }
                else
                {
                    ss2 = search(trie, path2);
                    if (ss2 >= 0)
                    {
                        struct Paths *newPath = (struct Paths *)malloc(sizeof(struct Paths));
                        newPath = createPaths(path2, ss2);
                        insertLRUCache(lruCache, newPath);
                    }
                }

                if (ss2 < 0)
                {
                    // send path does not exist
                    memset(buffer, 0, sizeof(buffer));
                    strcpy(buffer, "Destination path does not exist or is inaccessible.\n");
                    send(clientSocket, buffer, strlen(buffer), 0);
                    continue;
                }
                char path2copy[100], dir_name[20];

                strcpy(path2copy, path2);
                // get everything after the last slash from path2 and store in dir_name
                char *lastSlash = strrchr(path2copy, '/');
                strcpy(dir_name, lastSlash + 1);

                // concatenate / to path2backup
                strcat(path2copy, "/");
                // concatenate dir_name to path2backup
                strcat(path2copy, dir_name);

                memset(buffer, 0, sizeof(buffer));
                // remove the first two characters from the start of the path_to_ss string
                strcpy(buffer, path1 + 1);
                // concatenate SS[ss1].wd to the buffer
                memset(path1, 0, sizeof(path1));
                strcpy(path1, SS[ss1].wd);
                // strcat(path1, "/");
                strcat(path1, buffer);
                printf("\nPath1: %s\n", path1);

                // same for the second path
                memset(buffer, 0, sizeof(buffer));
                // remove the first two characters from the start of the path_to_ss string
                strcpy(buffer, path2 + 1);
                // concatenate SS[ss1].wd to the buffer
                memset(path2, 0, sizeof(path2));
                strcpy(path2, SS[ss2].wd);
                // strcat(path2, "/");
                strcat(path2, buffer);
                printf("\nPath2: %s\n", path2);

                pid_t pid = fork();

                if (pid == -1)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0)
                { // Child process
                    // Use execlp to execute the cp command
                    execlp("cp", "cp", "-r", path1, path2, NULL);

                    // If execlp fails
                    perror("execlp");
                    exit(EXIT_FAILURE);
                }
                else
                { // Parent process
                    // Wait for the child process to complete
                    int status;
                    waitpid(pid, &status, 0);

                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                    {
                        // printf("Folder copied successfully.\n");
                        //  send the same message to the client
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, "Folder copied successfully.\n");
                        insert(trie, path2copy, ss2);
                        send(clientSocket, buffer, strlen(buffer), 0);
                    }
                    else
                    {
                        fprintf(stderr, "Error copying folder.\n");
                    }
                }
            }
        }

        // Clear the buffer for the next iteration
        memset(buffer, 0, sizeof(buffer));
    }

    // Close the client socket
    close(clientSocket);

    // Exit the thread
    pthread_exit(NULL);
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

int main()
{
    int sockfd, ret, clsockfd;
    struct sockaddr_in serverAddr, cliserAddr;
    char ip[16];
    getIPAddress(ip, sizeof(ip));

    trie = getNewTrieNode();

    // initialize LRU using createLRUCache()

    lruCache = createLRUCache();

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    // create new socket for client connections
    clsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clsockfd < 0)
    {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }
    printf("Server Socket is created.\n");

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    memset(&cliserAddr, 0, sizeof(cliserAddr));
    cliserAddr.sin_family = AF_INET;
    cliserAddr.sin_port = htons(CPORT);
    cliserAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        perror("Error in binding");
        exit(EXIT_FAILURE);
    }
    // bind the client socket
    ret = bind(clsockfd, (struct sockaddr *)&cliserAddr, sizeof(cliserAddr));
    if (ret < 0)
    {
        perror("Error in CL binding");
        exit(EXIT_FAILURE);
    }

    //  Listen for connections

    pthread_t one_more_thread;

    if (listen(sockfd, MAX_SS) == 0)
    {
        printf("Listening...\n\n");
    }
    else
    {
        perror("Error in listening");
        exit(EXIT_FAILURE);
    }

    char message[] = "Hello Storage Server!\n";

    for (int i = 0; i < NUM_INIT_SS; i++)
    {
        struct sockaddr_in cliAddr;
        socklen_t addr_size = sizeof(cliAddr);

        // Accept client connection
        SS[i].socket = accept(sockfd, (struct sockaddr *)&cliAddr, &addr_size);
        printf("HERE\n");
        if (SS[i].socket < 0)
        {
            perror("Error in accepting");
            exit(EXIT_FAILURE);
        }

        printf("Storage Server connection accepted from %s:%d\n",
               inet_ntoa(cliAddr.sin_addr),
               ntohs(cliAddr.sin_port));

        send(SS[i].socket, message, strlen(message), 0);
        // receive the port number, the cport number and the ip address from the storage server as a single string
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(SS[i].socket, buffer, sizeof(buffer), 0);
        // tokenize the buffer to get the port number, the cport number and the ip address
        char *token = strtok(buffer, " ");
        SS[i].port = atoi(token);
        token = strtok(NULL, " ");
        SS[i].cport = atoi(token);
        token = strtok(NULL, " ");
        strcpy(SS[i].ip, token);
        token = strtok(NULL, " ");
        strcpy(SS[i].wd, token);
        printf("The port number is %d and the cport number is %d and the ip address is %s\n", SS[i].port, SS[i].cport, SS[i].ip);
        // send the message "Enter the list of accessible paths and respond with <STOP> when done" to the storage server
        char message1[] = "Enter the list of accessible paths and respond with <STOP> when done";
        send(SS[i].socket, message1, strlen(message1), 0);
        // continue to receive the paths from the storage server until the storage server sends the message "STOP"
        while (strcmp(buffer, "STOP") != 0)
        {
            memset(buffer, 0, sizeof(buffer));
            recv(SS[i].socket, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
            if (strcmp(buffer, "STOP") != 0)
            {
                insert(trie, buffer, i);
            }
            // printf("%s\n", buffer);
        }
    }
    if (pthread_create(&one_more_thread, NULL, Dyna_SS, &sockfd) != 0)
    {
        perror("Error in creating thread");
        exit(EXIT_FAILURE);
    }

    // listen for client conncections
    if (listen(clsockfd, MAX_CLIENTS) == 0)
    {
        printf("Listening for Client Connections...\n\n");
    }
    else
    {
        perror("Error in listening");
        exit(EXIT_FAILURE);
    }

    cnt = 0;
    pthread_t thread[MAX_CLIENTS];

    while (1)
    {
        // printf("here\n");
        int clientSocket;
        struct sockaddr_in cliAddr;
        socklen_t addr_size = sizeof(cliAddr);

        // Accept client connection
        clientSocket = accept(clsockfd, (struct sockaddr *)&cliAddr, &addr_size);
        if (clientSocket < 0)
        {
            perror("Error in accepting");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted from %s:%d\n",
               inet_ntoa(cliAddr.sin_addr),
               ntohs(cliAddr.sin_port));

        printf("Clients connected: %d\n\n", ++cnt);

        // Create a thread to handle the client
        if (pthread_create(&thread[cnt - 1], NULL, handleClient, &clientSocket) != 0)
        {
            perror("Error in creating thread");
            exit(EXIT_FAILURE);
        }
    }

    // Close the server socket
    close(sockfd);
    close(clsockfd);

    return 0;
}