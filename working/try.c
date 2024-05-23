#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Define the character size
#define CHAR_SIZE 126
#define MAX_PATH_LENGTH 51
// Data structure to store a Trie node
struct Trie
{
    int isLeaf;
    struct Paths *data;
    struct Trie *character[CHAR_SIZE];
};

// Data structure for storing paths
struct Paths
{
    char path[MAX_PATH_LENGTH];
    int storageNumber;
};
struct Trie *head;

// Function that returns a new Trie node
struct Trie *getNewTrieNode()
{
    struct Trie *node = (struct Trie *)malloc(sizeof(struct Trie));
    node->isLeaf = 0;

    for (int i = 0; i < CHAR_SIZE; i++)
    {
        node->character[i] = NULL;
    }

    return node;
}

void insert(struct Trie *head, char *path, int storageNumber)
{
    // printf("here 1\n");
    // Start from the root node
    struct Trie *curr = head;
    // print path
    printf("path: %s\n", path);
    // printf("here 2\n");
    while (*path)
    {
        if (curr->character[*path - 'a' + 100] == NULL)
        {
            // printf("here 2\n");
            curr->character[*path - 'a' + 100] = getNewTrieNode();
            // printf("here 4\n");
        }

        curr = curr->character[*path - 'a' + 100];
        path++;
    }

    // Mark the current node as a leaf and store the data
    curr->isLeaf = 1;
    if (curr->data == NULL)
    {
        curr->data = (struct Paths *)malloc(sizeof(struct Paths));
        strncpy(curr->data->path, path, MAX_PATH_LENGTH);
        curr->data->storageNumber = storageNumber;
    }
    printf("insertion successful\n");
    return;
}

// Iterative function to search a string in a Trie. It returns 1
// if the string is found in the Trie; otherwise, it returns 0.
int search(struct Trie *head, char *str)
{
    // return 0 if Trie is empty
    if (head == NULL)
    {
        return -1;
    }

    struct Trie *curr = head;
    while (*str)
    {
        // go to the next node
        curr = curr->character[*str - 'a' + 100];

        // if the string is invalid (reached end of a path in the Trie)
        if (curr == NULL)
        {
            return -1;
        }

        // move to the next character
        str++;
    }

    // return the storage server number if the current node is a leaf, otherwise return 0
    return (curr->isLeaf) ? curr->data->storageNumber : -1;
}

// Returns 1 if a given Trie node has any children
int hasChildren(struct Trie *curr)
{
    for (int i = 0; i < CHAR_SIZE; i++)
    {
        if (curr->character[i])
        {
            return 1; // child found
        }
    }

    return 0;
}

int deletion(struct Trie **curr, char *str)
{
    // return 0 if Trie is empty
    if (*curr == NULL)
    {
        return 0;
    }

    // if the end of the string is not reached
    if (*str)
    {
        // recur for the node corresponding to the next character in
        // the string and if it returns 1, delete the current node
        // (if it is non-leaf)
        if (*curr != NULL && (*curr)->character[*str - 'a' + 100] != NULL &&
            deletion(&((*curr)->character[*str - 'a' + 100]), str + 1))
        {
            if ((*curr)->isLeaf == 0 && !hasChildren(*curr))
            {
                free(*curr);
                (*curr) = NULL;
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }

    // if the end of the string is reached
    if (*str == '\0')
    {
        // if the current node is a leaf node, delete it
        if ((*curr)->isLeaf)
        {
            free(*curr);
            (*curr) = NULL;
            return 1;
        }
    }

    return 0;
}

// testing code
// int main()
// {
//     head = getNewTrieNode();

//     insert(head, "hi",0);
//     insert(head, "hip",1);
//     insert(head, "hipp",2);
//     insert(head, "hippo",3);

//     printf("%d: Should be 1\n", search(head, "hippo"));
//     printf("%d: Should be 0\n", search(head, "h"));

//     deletion(&head,"hippo");
//     printf("%d: Should be 0\n", search(head, "hippo"));

//     //deletion(&head,"hip");
//     printf("%d: Should be 1\n", search(head, "hipp"));
//     printf("%d: Should be 0\n", search(head, "hip"));

//     return 0;
// }