#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>

void display_prompt();
void expand_environment_variables(char *input);
void searchPath(const tokenlist *tokens);
void executeCommand(const char *fullPath, const tokenlist *tokens);
char **tokenlist_to_argv(const tokenlist *tokens) ;

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

        searchPath(tokens);

        free(input);
        free_tokens(tokens);
    }

    return 0;
}

void display_prompt()
{
    // Get environment variables
    char* user = getenv("USER");

    // Use a buffer for the machine name
    char machine[256];  // Choose a reasonable buffer size
    gethostname(machine, sizeof(machine));

    char* pwd = getcwd(NULL, 0);
    if (pwd == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    // Display the prompt
    printf("%s@%s:%s> ", user, machine, pwd);

    // Free memory allocated by getcwd
    free(pwd);
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
        if(token[0] == '~')
        {
            // Expand tilde (~) to the value of $HOME
            char *home_directory = getenv("HOME");

            // If tilde is followed by a specific path, append it to the home directory
            if(token[1] == '/')
            {
                strcat(modified_input, home_directory);
                strcat(modified_input, token + 1);
                strcat(modified_input, " ");
            }
            else
            {
                // Concatenate the home directory path to the modified string
                strcat(modified_input, home_directory);
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

void searchPath(const tokenlist *tokens) {
    // Check if tokens is empty
    if (tokens == NULL || tokens->size == 0) {
        fprintf(stderr, "No command provided\n");
        return;
    }

    // Get the first token as the command
    const char *command = tokens->items[0];

    // Get the PATH environment variable
    const char *path = getenv("PATH");

    // Check if PATH is not set
    if (path == NULL) {
        fprintf(stderr, "PATH environment variable is not set\n");
        return;
    }

    // Create a copy of PATH to avoid modifying the original string
    char *pathCopy = strdup(path);

    // Tokenize the PATH string using colon as the delimiter
    char *token = strtok(pathCopy, ":");

    bool foundPath = false;
    // Loop through each directory in PATH
    while (token != NULL) {
        // Construct the full path to the command in the current directory
        char fullPath[256];  // Adjust the size as needed
        snprintf(fullPath, sizeof(fullPath), "%s/%s", token, command);

        // Check if the file exists
        if (access(fullPath, X_OK) == 0) {
            // The file exists and is executable, execute it
            printf("Executing: %s\n", fullPath);
            executeCommand(fullPath,tokens);
            foundPath = true;
            break;  // Stop searching after the first match
        }

        // Move to the next directory in PATH
        token = strtok(NULL, ":");
    }

    //Change to error message
    if(!foundPath){
        printf("Command not found\n");
    }
    // Free the allocated memory for the copied PATH string
    free(pathCopy);
}

// Function to execute an external command with arguments
void executeCommand(const char *fullPath, const tokenlist *tokens) {
    pid_t pid = fork(); // Create a child process

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // This code is executed by the child process

        // Execute the command in the child process
        execv(fullPath,tokenlist_to_argv(tokens));

        // If execv returns, an error occurred
        perror("execv");
        exit(EXIT_FAILURE);
    } else {
        // This code is executed by the parent process

        // Wait for the child process to complete
        int status;
        waitpid(pid, &status, 0);
    }
}

// Convert tokenlist to execv-compatible array
char **tokenlist_to_argv(const tokenlist *tokens) {
    char **argv = (char **)malloc((tokens->size + 1) * sizeof(char *));
    if (argv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < tokens->size; ++i) {
        argv[i] = tokens->items[i];
    }

    argv[tokens->size] = NULL;  // Ensure the last element is NULL

    return argv;
}
