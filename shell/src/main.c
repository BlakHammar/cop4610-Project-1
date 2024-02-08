#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_BACKGROUND_JOBS 10
#define MAX_RECENT_COMMANDS 3
#define MAX_COMMAND_LENGTH 200

void display_prompt();
void expand_environment_variables(char *input);
bool searchPath(const tokenlist *tokens, int runInBackground);
void executeCommand(const char *fullPath, const tokenlist *tokens);
char **tokenlist_to_argv(const tokenlist *tokens);
void redirection(char *fileIn, char *fileOut);
void executeCommandModified(const char *fullPath, const tokenlist *tokens, int runInBackground);
bool checkBgStatus();
void cd(tokenlist * tokens);
void exitShell();
void jobs();
void addToRecentCmds(char *command);
tokenlist *checkForPipes(char *input);
void setupPipe(const tokenlist *cmds);


typedef struct
{
    pid_t pid;
    int jobNum;
    char command[256];
    int active;
} backgroundJob;


backgroundJob bgProcess[MAX_BACKGROUND_JOBS];
int nextJob = 1;

int numCommands = 0;
char rCommands[MAX_RECENT_COMMANDS][MAX_COMMAND_LENGTH];

int main()
{
    while (1) {
        display_prompt();

        /* input contains the whole command
         * tokens contains substrings from input split by spaces
         */
        char *input = get_input();

        expand_environment_variables(input);

        tokenlist *cmds = checkForPipes(input);

        size_t numCmds = 0;
        // Iterate until a NULL pointer is encountered
        while (cmds->items[numCmds] != NULL) {
            printf("Command %zu: %s\n", numCmds + 1, cmds->items[numCmds]);
            numCmds++;
        }
        

        if(numCmds == 1){
            tokenlist *tokens = get_tokens(input, " ");
        
            int runInBackground = 0;

            if(strcmp(tokens->items[tokens->size - 1], "&") == 0 )
            {
                runInBackground = 1;
                free(tokens->items[tokens->size - 1]);
                tokens->size--;
            }

            //Check for commands
            if(strcmp(tokens->items[0], "cd") == 0){
                cd(tokens);
                addToRecentCmds(input);
            }
            else if(strcmp(tokens->items[0], "jobs") == 0){
                jobs();
                addToRecentCmds(input);
            }
            else if(strcmp(tokens->items[0], "exit") == 0)
                exitShell();
            //Other external command   
            else{
                if(searchPath(tokens, runInBackground))
                    addToRecentCmds(input);
            } 

            free_tokens(tokens);
        } 
        else if(numCmds == 2){
            setupPipe(cmds);
            addToRecentCmds(input);
        }
        
        checkBgStatus();
        
        free_tokens(cmds);
        free(input);
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
    char *token = strtok(input, " ");
    char modified_input[1000];  // Creating string in case we need to change.
    modified_input[0] = '\0';    // Initialize the modified string

    while (token != NULL)
    {
        if (token[0] == '$')
        {
            printf("Found $");
            // Expand environment variable
            char *env_variable = getenv(token + 1);
            if(env_variable != NULL)
            {
                printf("Env exists");
                // Concatenate the environment variable value to the modified string
                strcat(modified_input, env_variable);
                strcat(modified_input, " ");
            } else
            {
                printf("Env doesnt exist");
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

bool searchPath(const tokenlist *tokens, int runInBackground) {
    // Check if tokens is empty
    if (tokens == NULL || tokens->size == 0) {
        fprintf(stderr, "No command provided\n");
        return false;
    }

    // Get the first token as the command
    const char *command = tokens->items[0];

    // Get the PATH environment variable
    const char *path = getenv("PATH");

    // Check if PATH is not set
    if (path == NULL) {
        fprintf(stderr, "PATH environment variable is not set\n");
        return false;
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
            executeCommandModified(fullPath,tokens, runInBackground);
            foundPath = true;
            break;  // Stop searching after the first match
        }

        // Move to the next directory in PATH
        token = strtok(NULL, ":");
    }

    //Change to error message
    if(!foundPath){
        printf("Command not found\n");
        return false;
    }
    // Free the allocated memory for the copied PATH string
    free(pathCopy);

    return true;
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

//IO redirection
void redirection(char *fileIn, char *fileOut) {
    // Input redirection
    if (fileIn != NULL) {
        int inFd = open(fileIn, O_RDONLY);
        if (inFd == -1) {
            perror("Error opening input file for redirection");
            exit(EXIT_FAILURE);
        }
        if (dup2(inFd, STDIN_FILENO) == -1) {
            perror("Error duplicating file descriptor for input redirection");
            exit(EXIT_FAILURE);
        }
        close(inFd);
    }

    // Output redirection
    if (fileOut != NULL) {
        int outFd = open(fileOut, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (outFd == -1) {
            perror("Error opening output file for redirection");
            exit(EXIT_FAILURE);
        }
        if (dup2(outFd, STDOUT_FILENO) == -1) {
            perror("Error duplicating file descriptor for output redirection");
            exit(EXIT_FAILURE);
        }
        close(outFd);
    }
}

//Modified execute to work with IO redirection
void executeCommandModified(const char *fullPath, const tokenlist *tokens, int runInBackground) {
    pid_t pid = fork(); // Create a child process

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // This code is executed by the child process

        // Initialize file pointers for redirection
        char *fileIn = NULL;
        char *fileOut = NULL;
        char **argv = (char **)malloc((tokens->size + 1) * sizeof(char *));
        if (argv == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        int j = 0; // Index for argv
        for (int i = 0; i < tokens->size; i++) {
            if (strcmp(tokens->items[i], "<") == 0 && i + 1 < tokens->size) 
            {
                fileIn = tokens->items[i + 1];
                i++; // Skip the next token (filename for input redirection)
            } 
            else if (strcmp(tokens->items[i], ">") == 0 && i + 1 < tokens->size) 
            {
                fileOut = tokens->items[i + 1];
                i++; // Skip the next token (filename for output redirection)
            } 
            else 
            {
                // This token is part of the command
                argv[j++] = tokens->items[i];
            }
        }
        argv[j] = NULL; // NULL-terminate argv

        // Handle redirection
        redirection(fileIn, fileOut);

        // Execute the command in the child process
        execv(fullPath, argv);

        // If execv returns, an error occurred
        perror("execv");
        exit(EXIT_FAILURE);
    } else {
        // This code is executed by the parent process
        if(runInBackground)
        {
            // Find spot in the array for new bg job
            for(int i = 0; i < MAX_BACKGROUND_JOBS; i++)
            {
                if(!bgProcess[i].active)
                {
                    // Found
                    bgProcess[i].pid = pid;
                    bgProcess[i].jobNum = nextJob++;
                    strncpy(bgProcess[i].command, fullPath, sizeof(bgProcess[i].command));
                    bgProcess[i].active = 1;
                    printf("[%d] %d\n", bgProcess[i].jobNum, pid);
                    break;
                }
            }
        }
        else
        {
            // Wait for child process to complete
            int status; 
            waitpid(pid, &status, 0);
        }
    }
}

bool checkBgStatus()
{
    bool jobsRunning = false;
    for(int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        if(bgProcess[i].active)
        {
            int status;
            pid_t result = waitpid(bgProcess[i].pid, &status, WNOHANG);
            if(result == bgProcess[i].pid)
            {
                // Job is done
                printf("[%d] + done %s\n", bgProcess[i].jobNum, bgProcess[i].command);
                bgProcess[i].active = 0;
            }
            else if(result == 0) {
                /*Job is not finished*/
                jobsRunning = true;
            }
            else
            {
                // Error
                perror("waitpid");
                bgProcess[i].active = 0;
            }
        }
    }

    return jobsRunning;
}

// List background jobs
void jobs()
{
    // Bool for checking active jobs
    bool status = false;
    for(int i = 0; i <MAX_BACKGROUND_JOBS; i++)
    {
        if(bgProcess[i].active)
        {
            status = true;
            printf("[%d]+[%d] [%s]\n", bgProcess[i].jobNum, bgProcess[i].pid, bgProcess[i].command);
        }
    }
    // No background jobs in array
    if(!status)
        printf("No active background processes\n");

}
// Change directory
void cd(tokenlist * tokens)
{
    // Wrong usage of command
    if(tokens->size > 2)
    {
        fprintf(stderr, "too many arguments\n");
        return;
    }
    // If no destination is given, change path to home
    // else set path to second token
    char * path = tokens->size == 1 ? getenv("HOME") : tokens->items[1];
    // Return if there is no home directory
    if(path == NULL)
    {
        fprintf(stderr,"No HOME set\n");
        return;
    }
    //Change directory to path
    //if returns non-zero an error has occured
    if(chdir(path) != 0)
        perror("cd");
}

void exitShell()
{
    while(checkBgStatus()){
        //Wait until all jobs are finished
    }

    if(numCommands == 0){
         printf("No recent commands\n");
    }
    else if(numCommands>=3){
        printf("Last 3 valid commands:\n");
        printf("[1]: %s\n",rCommands[2]);
        printf("[2]: %s\n",rCommands[1]);
        printf("[3]: %s\n",rCommands[0]);
    }
    else{
        printf("Last valid command: %s",rCommands[numCommands-1]);
    }
    exit(0);
}

void addToRecentCmds(char *command){
    if(numCommands <= 2){
        snprintf(rCommands[numCommands], MAX_COMMAND_LENGTH, command);
    }
    else{
        snprintf(rCommands[0], MAX_COMMAND_LENGTH, rCommands[1]);
        snprintf(rCommands[1], MAX_COMMAND_LENGTH, rCommands[2]);
        snprintf(rCommands[2], MAX_COMMAND_LENGTH, command);
    }

    numCommands++;
}

tokenlist *checkForPipes(char *input){
    char *inputCopy = strdup(input);
    tokenlist *cmds = get_tokens(input, "|");

    free(inputCopy);
    return cmds;
}

void setupPipe(const tokenlist *cmds){
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid1 == 0) {
        // Child process 1
        close(pipefd[0]);  // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO);  // Redirect stdout to the write end of the pipe
        close(pipefd[1]);  // Close the write end

        tokenlist *tokens = get_tokens(cmds->items[0], " ");
        searchPath(tokens, 0);

        free_tokens(tokens);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();

    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        // Child process 2
        close(pipefd[1]);  // Close unused write end
        dup2(pipefd[0], STDIN_FILENO);  // Redirect stdin to the read end of the pipe
        close(pipefd[0]);  // Close the read end

        tokenlist *tokens = get_tokens(cmds->items[1], " ");
        searchPath(tokens, 0);

        free_tokens(tokens);
        exit(EXIT_SUCCESS);
    }

    // Parent process
    close(pipefd[0]);  // Close both ends in the parent
    close(pipefd[1]);

    // Wait for both child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}
