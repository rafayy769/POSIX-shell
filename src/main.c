/**
 * @file main.c
 * @author Abdul Rafay (24100173@lums.edu.pk)
 * @brief This is the main file for the shell. It contains the main function and the loop that runs the shell.
 * @version 0.1
 * @date 2023-06-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "utils.h"
#include "command.h"
#include "parser.h"
#include "hashtable.h"

#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

// This macro is defined when the shell runs a process, so if this shell is run inside another instance of the same shell, the macro gets defined. It is used to avoid any sort of conflicts. 
#ifdef CHILD_PROCESS
#undef CHILD_PROCESS
#endif

// Number of buckets used by the alias hashtable.
#define NUMBER_OF_BUCKETS 101

/* The shell supports three different modes:
 * 1. Interactive: The default usage. An interactive command line.
 * 2. Non_interactive: When the input is not via a terminal but by any other mean.
 * 3. Script: Runs a list of commands specified in a file.
*/
#define INTERACTIVE_MODE 1
#define NON_INTERACTIVE_MODE 2
#define SCRIPT_MODE 3

#define SHELL_STATES_LIST \
    X(INTERACTIVE),       \
    X(NON_INTERACTIVE),   \
    X(SCRIPT)             \

#define CAT(X,Y) X##Y
#define STR(X) #X

// Global variables
int lastExitStatus = 0;
hashtable* aliases = NULL;

FILE* script = NULL;
int mode = 0;

int originalStdoutFD;
int originalStdinFD;
int dup_originalIn;
int dup_originalOut;

// Useful functions
char* getInput(void);

/**
 * @brief This is the main function for the shell. It contains the main loop that runs the shell.
 * 
 * @return int 
 */
int main(int argc, char** argv)
{
    if (argc > 2)
    {
        LOG_ERROR("Usage: %s [script]\n", argv[0]);
        exit(1);
    }

    // If a script is provided, run it and exit
    if (argc == 2)
    {
        mode = SCRIPT_MODE;
        script = fopen(argv[1], "r");
        if (!script)
        {
            LOG_ERROR("Error opening script %s: %s\n", argv[1], strerror(errno));
            exit(1);
        }
    }
    else
    {
        // Check if shell is running in interactive mode
        if (isatty(STDIN_FILENO))
        {
            mode = INTERACTIVE_MODE;
            // Configure readline to auto-complete paths when the tab key is hit.
            rl_bind_key('\t', rl_complete);

            // Enable history
            using_history();
        }
        else
        {
            mode = NON_INTERACTIVE_MODE;
        }
    }

    // Initialize aliases hashtable
    aliases = createHashtable(NUMBER_OF_BUCKETS);
    if (!aliases)
    {
        LOG_DEBUG("Error creating hashtable for aliases\n");
        exit(1);
    }

    // save the original file descriptors
    originalStdinFD = STDIN_FD;
    originalStdoutFD = STDOUT_FD;

    LOG_DEBUG("Starting shell\n");
    LOG_DEBUG("Shell's state:\n");
    if (mode == INTERACTIVE_MODE)
        LOG_DEBUG("-- Running in INTERACTIVE mode.\n");
    else if (mode == SCRIPT_MODE)
        {LOG_DEBUG("-- Running in SCRIPT mode.\n");LOG_DEBUG("-- -- Script: %s\n", argv[1]);}
    else if (mode == NON_INTERACTIVE_MODE)
        LOG_DEBUG("-- Running in NON_INTERACTIVE_MODE mode.\n");

    LOG_PRINT("Original STDIN: %d\n", originalStdinFD);
    LOG_PRINT("Original STDOUT: %d\n", originalStdoutFD);

    char delimiter = ' ';

    while (1) 
    {
        // read input
        char* input = getInput();

        // Check for EOF.
        if (!input)
            break;
        if (strcmp(input, "") == 0) 
        {
            free(input);
            continue;
        }
        if (strcmp(input, "exit") == 0)
        {
            free(input);
            deleteHashtable(aliases);
            break;
        }

        // Add input to readline history.
        if (mode == INTERACTIVE_MODE)
            add_history(input);

        // simple whitespace tokenizer
        char** tokens = tokenizeString(input, delimiter);

        for (int i = 0; tokens[i] != NULL; i++) {
            LOG_DEBUG("Token %d: [%s]\n", i, tokens[i]);
        }

        // generate the command from tokens
        CommandChain* commandChain = parseTokens(tokens);

        // display the command chain
        printCommandChain(commandChain);
        
        // execute the command
        int status = executeCommandChain(commandChain);
        LOG_DEBUG("Command executed with status %d\n", status);

        // Free tokens
        freeTokens(tokens);

        // free the command chain
        cleanUpCommandChain(commandChain);

        // Free buffer that was allocated by readline
        free(input);
    }
    return 0;
}

char* getInput()
{
    char* input = NULL;

    switch (mode)
    {
        case INTERACTIVE_MODE:
            char prompt_buffer[MAX_STRING_LENGTH];
            char* cwd_res = getcwd(prompt_buffer, MAX_STRING_LENGTH);
            if (!cwd_res)
            {
                LOG_ERROR("Error getting current working directory: %s\n", strerror(errno));
                exit(1);
            }

            input = readline(strcat(prompt_buffer, " $ "));
            break;
        case NON_INTERACTIVE_MODE:
            input = malloc(sizeof(char) * MAX_STRING_LENGTH);
            if (!input)
            {
                LOG_ERROR("Error allocating memory for input: %s\n", strerror(errno));
                exit(1);
            }
            if (fgets(input, MAX_STRING_LENGTH, stdin) == NULL)
            {
                free(input);
                return NULL;
            }
            // Remove trailing newline
            if (input[strlen(input) - 1] == '\n')
                input[strlen(input) - 1] = '\0';
            break;
        case SCRIPT_MODE:
            size_t len = 0;
            ssize_t read;
            read = getline(&input, &len, script);
            if (read == -1)
            {
                free(input);
                return NULL;
            }
            // Remove trailing newline
            if (input[read - 1] == '\n')
                input[read - 1] = '\0';
            break;
        default:
            LOG_ERROR("Invalid mode %d\n", mode);
            exit(1);
    }

    return input;
}