#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void display_prompt();
void expand_environment_variables(char *input);

int main()
{
    while (1) {
        display_prompt();

        /* input contains the whole command
         * tokens contains substrings from input split by spaces
         */

        char *input = get_input();
        printf("whole input: %s\n", input);

        expand_environment_variables(input);
        
        printf("whole input: %s\n", input);

        tokenlist *tokens = get_tokens(input);
        for (int i = 0; i < tokens->size; i++) {
            printf("token %d: (%s)\n", i, tokens->items[i]);
        }

        free(input);
        free_tokens(tokens);
    }

    return 0;
}

char *get_input(void) {
    char *buffer = NULL;
    int bufsize = 0;
    char line[5];
    while (fgets(line, 5, stdin) != NULL)
    {
        int addby = 0;
        char *newln = strchr(line, '\n');
        if (newln != NULL)
            addby = newln - line;
        else
            addby = 5 - 1;
        buffer = (char *)realloc(buffer, bufsize + addby);
        memcpy(&buffer[bufsize], line, addby);
        bufsize += addby;
        if (newln != NULL)
            break;
    }
    buffer = (char *)realloc(buffer, bufsize + 1);
    buffer[bufsize] = 0;
    return buffer;
}

tokenlist *new_tokenlist(void) {
    tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
    tokens->size = 0;
    tokens->items = (char **)malloc(sizeof(char *));
    tokens->items[0] = NULL; /* make NULL terminated */
    return tokens;
}

void add_token(tokenlist *tokens, char *item) {
    int i = tokens->size;

    tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
    tokens->items[i] = (char *)malloc(strlen(item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);

    tokens->size += 1;
}

tokenlist *get_tokens(char *input) {
    char *buf = (char *)malloc(strlen(input) + 1);
    strcpy(buf, input);
    tokenlist *tokens = new_tokenlist();
    char *tok = strtok(buf, " ");
    while (tok != NULL)
    {
        add_token(tokens, tok);
        tok = strtok(NULL, " ");
    }
    free(buf);
    return tokens;
}

void free_tokens(tokenlist *tokens) {
    for (int i = 0; i < tokens->size; i++)
        free(tokens->items[i]);
    free(tokens->items);
    free(tokens);
}


void display_prompt()
{
    // Get the user name
    char *user = getenv("USER");
    if (user == NULL)
    {
        perror("Error getting USER environment variable");
        exit(EXIT_FAILURE);
    }

    // Get the machine name
    char *machine = getenv("MACHINE");
    if (machine == NULL)
    {
        perror("Error getting MACHINE environment variable");
        exit(EXIT_FAILURE);
    }

    // Get the absolute working directory
    char *pwd = getenv("PWD");
    if (pwd == NULL)
    {
        perror("Error getting PWD environment variable");
        exit(EXIT_FAILURE);
    }

    // Display the prompt
    printf("%s@%s:%s> ", user, machine, pwd);
}

void expand_environment_variables(char *input)
{
    int counter = 0;
    char *token = strtok(input, " ");
    char modified_input[1000];  // Creating string in case we need to change.
    modified_input[0] = '\0';    // Initialize the modified string

    while (token != NULL)
    {
        if (token[0] == '$')
        {
            // Expand environment variable
            char *env_variable = getenv(token + 1);
            if(env_variable != NULL)
            {
                // Concatenate the environment variable value to the modified string
                strcat(modified_input, env_variable);
                strcat(modified_input, " ");
            } else
            {
                // If the environment variable doesn't exist, keep the original token
                strcat(modified_input, token);
                strcat(modified_input, " ");
            }
        }
        else
        {
            // Keep non-environment variable tokens unchanged
            strcat(modified_input, token);
            strcat(modified_input, " ");
        }
        token = strtok(NULL, " ");

    }
    // Remove the trailing space
    modified_input[strlen(modified_input) - 1] = '\0';

    // Copy the modified input back to the original input
    strcpy(input, modified_input);
}
