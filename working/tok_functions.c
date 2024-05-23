#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_SIZE 30

int get_num_tokens(char *string)
{
    // count the number of tokens in string using strtok_r
    // return the number of tokens
    char *token;
    int count = 0;
    char *saveptr;

    char *string_copy = (char *)malloc(strlen(string) + 1);
    strcpy(string_copy, string);

    token = strtok_r(string_copy, " ", &saveptr);
    while (token != NULL)
    {
        count++;
        token = strtok_r(NULL, " ", &saveptr);
    }
    free(string_copy);
    return count;
}

void tokenize(char *string, char **tokens)
{
    // tokenize the string using strtok_r
    // store the tokens in the tokens array
    char *token;
    char *saveptr;

    char *string_copy = (char *)malloc(strlen(string) + 1);
    strcpy(string_copy, string);

    token = strtok_r(string_copy, " ", &saveptr);
    int i = 0;
    while (token != NULL)
    {
        tokens[i] = (char *)malloc(sizeof(token) + 1);
        strcpy(tokens[i], token);
        i++;
        token = strtok_r(NULL, " ", &saveptr);
    }
    free(string_copy);
}

void run_command(char **tokens)
{
    // run the command specified by the tokens
    // use execvp

    // pass the command and the arguments to read function
    if (!strcmp(tokens[0], "read31"))
    {
        printf("read31\n");
    }
    else if (!strcmp(tokens[0], "write31"))
    {
        printf("write31\n");
    }
    else if (!strcmp(tokens[0], "info31"))
    {
        printf("info31\n");
    }
    else
    {
        printf("Invalid command\n");
    }

    // add stuff as required

    return;
}
