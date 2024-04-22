/**
 * @file builtitns.c
 * @author Abdul Rafay (24100173@lums.edu.pk)
 * @brief Contains the function definitions for the builtin shell functions.
 * @version 0.1
 * @date 2023-06-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "shell_builtins.h"
#include "hashtable.h"

#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

// Global aliases table
extern hashtable* aliases;

// stores the original stdin and stdout fds
extern int originalStdoutFD;
extern int originalStdinFD;
extern int dup_originalIn;
extern int dup_originalOut;

/*-------------------------------File Desc Manipulators----------------------------------*/

/**
 * @brief Sets up the file descriptors for a command. Duplicates the file descriptors to stdin, and stdout, and if we are in the parent process, we also save the original stdin and stdout file descriptors.
 * 
 * Uses dup2 system call to set up the file descriptors. Returns 0 on success, -1 on failure. Only dups if the file descriptors are not the default ones.
 * 
 * @param inputFD The input file descriptor
 * @param outputFD The output file descriptor
 * @return int Status code (0 on success, -1 on failure)
 */
static int setUpFD(int inputFD, int outputFD)
{
    LOG_OUT("===== SETUPFD ENTER\n");
    LOG_OUT("===== pipe/io's inputFD: %d\n", inputFD);
    LOG_OUT("===== pipe/io's outputFD: %d\n", outputFD);

    if (inputFD != STDIN_FD)
    {
        originalStdinFD = dup(STDIN_FD);
        LOG_DEBUG("Saved copy of originalStdin: %d\n", originalStdinFD);

        if (dup2(inputFD, STDIN_FD) == -1)
        {
            LOG_DEBUG("dup2: %s\n", strerror(errno));
            return -1;
        }

        close(inputFD);
    }

    if (outputFD != STDOUT_FD)
    {
        originalStdoutFD = dup(STDOUT_FD);
        LOG_DEBUG("Saved copy of originalStdout: %d\n", originalStdoutFD);
        

        if (dup2(outputFD, STDOUT_FD) == -1)
        {
            LOG_DEBUG("dup2: %s\n", strerror(errno));
            return -1;
        }

        close(outputFD);
    }

    return 0;
}

/**
 * @brief Resets the file descriptors to the default ones (stdin and stdout).
 * 
 * @return void
 */
static void resetFD()
{
    if (originalStdinFD != STDIN_FD)
    {
    if (dup2(originalStdinFD, STDIN_FD) == -1)
    {
        LOG_ERROR("dup2: %s\n", strerror(errno));
        exit(1);
    }
    }

    if (originalStdoutFD != STDOUT_FD)
    {
    if (dup2(originalStdoutFD, STDOUT_FD) == -1)
    {
        LOG_ERROR("dup2: %s\n", strerror(errno));
        exit(1);
    }
    }
}

/*-------------------------------Builtins-----------------------------------------------*/

int cd(SimpleCommand* simpleCommand)
{
    if (simpleCommand->argc > 2)
    {
        LOG_ERROR("cd: Too many arguments\n");
        return -1;
    }

    // Don't think cd ever needs any input from stdin, neither it puts anything to stdout, so dont need to modify file descriptors
    const char* path = NULL;
    if (simpleCommand->argc == 1)
    {
        // No path specified, go to home directory
        path = HOME_DIR;
    }
    else
    {
        path = simpleCommand->args[1];
    }

    if (chdir(path) == -1)
    {
        LOG_ERROR("cd: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int pwd(SimpleCommand* simpleCommand)
{
    if (simpleCommand->argc > 1)
    {
        LOG_ERROR("pwd: Too many arguments\n");
        return -1;
    }

    char cwd[MAX_PATH_LENGTH];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        LOG_ERROR("pwd: %s\n", strerror(errno));
        return -1;
    }

    if (setUpFD(simpleCommand->inputFD, simpleCommand->outputFD))
    {
        return -1;
    }

    LOG_PRINT("%s\n", cwd);

    resetFD();
    return 0;
}

int echo(SimpleCommand* simpleCommand)
{
    LOG_OUT("===== ECHO PARAMS ENTER\n");
    // LOG_OUT("===== originalStdin: %d\n", originalStdinFD);
    // LOG_OUT("===== originalStdout: %d\n", originalStdoutFD);
    // LOG_OUT("===== dup_originalIn: %d\n", dup_originalIn);
    // LOG_OUT("===== dup_originalOut: %d\n", dup_originalOut);

    if (setUpFD(simpleCommand->inputFD, simpleCommand->outputFD))
    {
        return -1;
    }

    LOG_OUT("===== SETUP FDS\n");
    // LOG_OUT("===== originalStdin: %d\n", originalStdinFD);
    // LOG_OUT("===== originalStdout: %d\n", originalStdoutFD);
    // LOG_OUT("===== dup_originalIn: %d\n", dup_originalIn);
    // LOG_OUT("===== dup_originalOut: %d\n", dup_originalOut);

    printSimpleCommand(simpleCommand);

    for (int i = 1; i < simpleCommand->argc - 1; i++)
    {
        LOG_PRINT("%s ", simpleCommand->args[i]);
    }
    LOG_PRINT("%s\n", simpleCommand->args[simpleCommand->argc - 1]);

    if (simpleCommand->inputFD != STDIN_FD)
        resetFD();

    LOG_OUT("===== RESET FDS\n");
    LOG_OUT("===== originalStdin: %d\n", originalStdinFD);
    LOG_OUT("===== originalStdout: %d\n", originalStdoutFD);
    LOG_OUT("===== dup_originalIn: %d\n", dup_originalIn);
    LOG_OUT("===== dup_originalOut: %d\n", dup_originalOut);

    return 0;
}

int exitShell(SimpleCommand* simpleCommand)
{
    if (simpleCommand->argc > 1)
    {
        printf("exit: Too many arguments\n");
        return -1;
    }

    printf("Exiting shell\n");
    exit(0);
}

int alias(SimpleCommand* simpleCommand)
{
    // alias usage:
    // alias : lists all aliases
    // alias name : prints alias for name
    // alias name command : creates alias for name with command

    if (simpleCommand->argc > 3)
    {
        LOG_ERROR("alias: Too many arguments\n");
        return -1;
    }

    if (setUpFD(simpleCommand->inputFD, simpleCommand->outputFD))
    {
        return -1;
    }

    if (simpleCommand->argc == 1)
    {
        // No arguments, list all aliases
        printHashtable(aliases);
    }
    else if (simpleCommand->argc == 2)
    {
        // One argument, print alias for name
        const char* key = simpleCommand->args[1];
        const char* value = get(aliases, key);

        if (value)
            LOG_PRINT("%s=\'%s\'\n", key, value);
    }
    else
    {
        // Two arguments, create alias for name with command
        const char* key = simpleCommand->args[1];
        const char* value = simpleCommand->args[2];

        set(aliases, key, value);
    }

    resetFD();

    return 0;
}

int unalias(SimpleCommand* simpleCommand)
{
    // unalias usage:
    // unalias name : removes alias for name

    if (simpleCommand->argc > 2)
    {
        LOG_ERROR("unalias: Too many arguments\n");
        return -1;
    }
    
    if (simpleCommand->argc == 1)
    {
        LOG_ERROR("unalias: Too few arguments\n");
        return -1;
    }

    const char* key = simpleCommand->args[1];
    const char* value = get(aliases, key);

    if (!value)
    {
        LOG_ERROR("unalias: %s: not found\n", key); 
        return -1;
    }

    set(aliases, key, NULL);

    return 0;
}

int history(SimpleCommand* simpleCommand)
{
    if (simpleCommand->argc > 1)
    {
        LOG_ERROR("history: Too many arguments\n");
        return -1;
    }


    HIST_ENTRY** historyList = history_list();
    if (historyList == NULL)
    {
        LOG_ERROR("history: %s\n", strerror(errno));
        return -1;
    }

    if (setUpFD(simpleCommand->inputFD, simpleCommand->outputFD))
    {
        return -1;
    }

    for (int i = 0; historyList[i] != NULL; i++)
    {
        LOG_PRINT("%d %s\n", i + 1, historyList[i]->line);
    }

    resetFD();

    return 0;
}

int executeProcess(SimpleCommand* simpleCommand)
{
    int pid = fork();

    if (pid == -1)
    {
        // FO'KING ERROR (me when Bri'ish)

        LOG_DEBUG("fork: %s\n", strerror(errno));
        return -1;
    }
    else if (pid == 0)
    {
        // Duplicate the FDs. Default FDs are STDIN AND STDOUT but, if pipes or  < > are used, the FDs are updated in the parsing step, by opening the relevant file or creating relevant pipes
        setUpFD(simpleCommand->inputFD, simpleCommand->outputFD);

        // Execute the command
        if (execvp(simpleCommand->commandName, simpleCommand->args) == -1)
        {
            LOG_ERROR("%s: %s\n", simpleCommand->commandName, strerror(errno));
            exit(1);
        }

        // This should never be reached
        LOG_ERROR("This should never be reached\n");
        exit(0);
    }
    else
    {
        // Parent process
        simpleCommand->pid = pid;

        // waiting for the child process to finish
        int status;
        LOG_DEBUG("Waiting for child process, with command name %s\n", simpleCommand->commandName);
        if (waitpid(pid, &status, 0) == -1)
        {
            LOG_ERROR("waitpid: %s\n", strerror(errno));
            return -1;
        }

        // print the error (if any) from errno
        if (WEXITSTATUS(status) != 0)
        {
            // LOG_ERROR("%s: %s\n", simpleCommand->commandName, strerror(errno));
            LOG_DEBUG("Non zero exit status : %d\n", WEXITSTATUS(status));
            return WEXITSTATUS(status);
        }
    }

    LOG_DEBUG("Finished executing command %s\n", simpleCommand->commandName);
    return 0;
}

/**
 * @brief This struct represents the builtin commands of the shell, and their corresponding execution functions.
 * 
 */
typedef struct commandRegistry
{
    char* commandName;
    ExecutionFunction executionFunction;
} CommandRegistry;

/**
 * @brief Registry of all the commands supported by the shell, and their corresponding execution functions. If a command is not found in the registry, it is assumed to be a process to be executed and the executeProcess function is called. Add new commands here, with their appropriate functions.
 * 
 */
static const CommandRegistry commandRegistry[] = {
    {"cd", cd},
    {"pwd", pwd},
    {"echo", echo},
    {"exit", exitShell},
    {"alias", alias},
    {"unalias", unalias},
    {"history", history},
    {NULL, NULL}
};

ExecutionFunction getExecutionFunction(char* commandName)
{
    for (int i = 0; commandRegistry[i].commandName != NULL; i++)
    {
        if (strcmp(commandRegistry[i].commandName, commandName) == 0)
        {
            return commandRegistry[i].executionFunction;
        }
    }

    return executeProcess;
}