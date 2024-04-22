/**
 * @file command.c
 * @author Abdul Rafay (24100173@lums.edu.pk)
 * @brief Contains the function definitions for the functions defined in command.h
 * @version 0.1
 * @date 2023-06-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "command.h"

// simple macro to check if this command is chained with a certain operator  with the last command(just a hack for readability)
#define CHAINED_WITH(opr) (prevCommand ? (prevCommand->chainingOperator ? (strcmp(prevCommand->chainingOperator, opr) == 0) : 0) : 0)

/*----------------------------------------------------------------------------------------*/

/*-------------------------------Initializers--------------------------------------------*/

// initializes a simple command with default values
SimpleCommand* initSimpleCommand()
{
    SimpleCommand* simpleCommand = (SimpleCommand*) malloc(sizeof(SimpleCommand));

    if (!simpleCommand)
        return NULL;
    
    simpleCommand->commandName = NULL;
    simpleCommand->args        = NULL;
    simpleCommand->argc        = 0;
    simpleCommand->inputFD     = STDIN_FD;
    simpleCommand->outputFD    = STDOUT_FD;
    simpleCommand->execute     = NULL;
    simpleCommand->pid         = -1;

    return simpleCommand;
}

// initializes a command with default values
Command* initCommand()
{
    Command* command = (Command*)malloc(sizeof(Command));

    if (!command)
        return NULL;

    command->simpleCommands   = NULL;
    command->nSimpleCommands  = 0;
    command->background       = false;
    command->chainingOperator = NULL;
    command->next             = NULL;

    return command;
}

// initializes a command chain with default values
CommandChain* initCommandChain()
{
    CommandChain* chain = (CommandChain*)malloc(sizeof(CommandChain));

    if (!chain)
        return NULL;

    chain->head = NULL;
    chain->tail = NULL;

    return chain;
}

/*-------------------------------Setters (push functions)--------------------------------*/

// adds a command to the command chain
int addCommandToChain(CommandChain* chain, Command* command)
{
    if (!chain)
    {
        LOG_DEBUG("Invalid command chain passed\n");
        return -1;
    }

    // If the chain is empty, add the command as the head
    if (!chain->head)
    {
        chain->head = command;
        chain->tail = command;
    }
    // Otherwise, add the command to the tail
    else
    {
        chain->tail->next = command;
        chain->tail = command;
    }

    return 0;
}

// adds a simpleCommand to the current command chain link
int addSimpleCommand(Command* command, SimpleCommand* simpleCommand)
{
    if (!command)
    {
        LOG_DEBUG("Invalid command passed. It's NULL\n");
        return -1;
    }

    if (!simpleCommand)
    {
        LOG_DEBUG("Invalid simpleCommand passed. It's NULL\n");
        return -1;
    }

    // we need to realloc the array containing the commands.
    SimpleCommand** temp = (SimpleCommand**)realloc(command->simpleCommands, (command->nSimpleCommands + 1) * sizeof(SimpleCommand*));

    if (!temp)
    {
        LOG_DEBUG("Realloc error. Failed to reallocate memory for the array.\n");
        return -1;
    }

    // update the pointer
    command->simpleCommands = temp;
    temp = NULL;

    // add the new command at the end.
    command->simpleCommands[command->nSimpleCommands] = simpleCommand;

    // update the number of commands
    command->nSimpleCommands++;

    return 0;
}

// pushes an arg to the simpleCommand's args array. makes sure the args array is always null terminated.
int pushArgs(char* arg, SimpleCommand* simpleCommand)
{
    if (!simpleCommand)
    {
        LOG_DEBUG("Invalid simpleCommand passed. It's NULL\n");
        return -1;
    }

    // when NULL ptr given, realloc behaves like malloc
    char** temp = (char**)realloc(simpleCommand->args, (simpleCommand->argc + 2) * sizeof(char*));

    if (!temp)
    {
        LOG_DEBUG("Realloc error. Failed to reallocate memory for the array.\n");
        return -1;
    }

    simpleCommand->args = temp;
    temp = NULL;

    simpleCommand->args[simpleCommand->argc] = COPY(arg);
    simpleCommand->args[simpleCommand->argc + 1] = NULL;
    simpleCommand->argc++;

    if (simpleCommand->argc == 1)
    {
        simpleCommand->commandName = COPY(arg);
    }

    return 0;
}

/*-------------------------------Command Execution functions------------------------------*/

// executes a command chain
int executeCommandChain(CommandChain* chain)
{
    if (!chain)
    {
        LOG_DEBUG("Invalid command chain passed\n");
        return -1;
    }

    Command* command = chain->head;
    Command* prevCommand = NULL;

    // lastStatus is the exit status of the last command in the chain, used by logical chaining operators
    int lastStatus = 0;

    // always execute the first command in the chain
    if (!command)
    {
        LOG_DEBUG("Commmad chain is empty\n");
        return -1;
    }

    lastStatus = executeCommand(command);

    prevCommand = command;
    command = command->next;

    while (command)
    {
        if (CHAINED_WITH("&&"))
        {
            // Only execute the current command if the previous command succeeded, else skip it
            if (lastStatus == 0)
            {
                lastStatus = executeCommand(command);
            }
        }
        else if (CHAINED_WITH("||"))
        {
            // Only execute the current command if the previous command failed, else skip it
            if (lastStatus != 0)
            {
                lastStatus = executeCommand(command);
            }
        }
        else if (CHAINED_WITH(";"))
        {
            // Always execute the current command
            lastStatus = executeCommand(command);
        }
        else
        {
            LOG_DEBUG("Invalid chaining operator\n");
            return -1;
        }

        // update the pointers
        prevCommand = command;
        command = command->next;
    }

    return lastStatus;
}

// executes a Command (with or without IO redirs)
int executeCommand(Command* command)
{
    if (!command)
    {
        LOG_DEBUG("Invalid command passed\n");
        return -1;
    }

    // If the command is empty, return an error
    if (!command->simpleCommands || command->nSimpleCommands == 0)
    {
        LOG_DEBUG("Invalid command. It's empty\n");
        return -1;
    }

    for (int i = 0; i < command->nSimpleCommands; i++)
    {
        LOG_DEBUG("Executing command : %s\n", command->simpleCommands[i]->commandName);
        SimpleCommand* simpleCommand = command->simpleCommands[i];

        // If the command name is empty, return an error
        if (!simpleCommand->commandName)
        {
            LOG_DEBUG("Invalid command name. It's empty\n");
            return -1;
        }
        
        // non-zero status means the command execution failed (both for built-in and external commands)
        int status = simpleCommand->execute(simpleCommand);
        LOG_DEBUG("Command executing with pid: %d\n", simpleCommand->pid);

        // If the command failed, return the status
        if (status)
        {
            return status;
        }

        // if the command succeeded, simply close the file descriptors
        if (simpleCommand->inputFD != STDIN_FD)
            close(simpleCommand->inputFD);
        
        if (simpleCommand->outputFD != STDOUT_FD)
            close(simpleCommand->outputFD);
    }

    return 0;
}

/*-------------------------------Clean up functions---------------------------------------*/

// cleans up a simple command and frees memeory
void cleanUpSimpleCommand(SimpleCommand* simpleCommand)
{
    if (!simpleCommand)
        return;

    LOG_DEBUG("Cleaning up simple command: %s\n", simpleCommand->commandName);

    // free the commandName. It was allocated with strdup, so this is the only pointer to that string. The source for the string was the input token, which is freed in the main loop.
    if (simpleCommand->commandName)
    {
        free(simpleCommand->commandName);
        simpleCommand->commandName = NULL;
    }

    // free the args. They were allocated with strdup, so this is the only pointer to that string. The source for the string was the input token, which is freed in the main loop.
    if (simpleCommand->args)
    {
        for (int i = 0; i < simpleCommand->argc; i++)
        {
            if (simpleCommand->args[i])
            {
                free(simpleCommand->args[i]);
                simpleCommand->args[i] = NULL;
            }
        }

        free(simpleCommand->args);
        simpleCommand->args = NULL;
    }

    // free the  simpleCommand
    free(simpleCommand);
    simpleCommand = NULL;
}

// cleans up a command
void cleanUpCommand(Command* command)
{
    if (!command)
        return;

    // free the simpleCommands
    if (command->simpleCommands)
    {
        for (int i = 0; i < command->nSimpleCommands; i++)
        {
            cleanUpSimpleCommand(command->simpleCommands[i]);
        }
    }

    free(command->simpleCommands);

    // free the chainingOperator, it was allocated with strndup
    if (command->chainingOperator)
    {
        free(command->chainingOperator);
        command->chainingOperator = NULL;
    }

    // don't need to free the next and current command, they will be handled by the chain cleanup
}

// clean up the command chain linked list
void cleanUpCommandChain(CommandChain* chain)
{
    if (!chain)
        return;

    // free the commands
    if (chain->head)
    {
        Command* command = chain->head;
        while (command)
        {
            Command* nextCommand = command->next;
            cleanUpCommand(command);
            free(command);
            command = nextCommand;
        }
    }

    // free the chain
    free(chain);
    chain = NULL;
}

/*-------------------------------Utility functions----------------------------------------*/

void printCommandChain(CommandChain* chain)
{
    LOG_DEBUG("Printing command chain\n");
    if (!chain)
        return;

    Command* command = chain->head;
    int counter = 1;
    while (command)
    {
        LOG_DEBUG("[Link %d]\n", counter);
        for (int i = 0; i < command->nSimpleCommands; i++)
        {
            printSimpleCommand(command->simpleCommands[i]);
        }
        counter++;
        command = command->next;
    }
}

void printSimpleCommand(SimpleCommand* simpleCommand)
{
    if (!simpleCommand)
        return;

    LOG_PRINT("-- name: %s\n", simpleCommand->commandName);
    LOG_PRINT("-- args:\n");
    for (int i = 0; i < simpleCommand->argc; i++)
    {
        LOG_PRINT("-- -- %s \n", simpleCommand->args[i]);
    }

    LOG_PRINT("-- Input FD: %d\n", simpleCommand->inputFD);
    LOG_PRINT("-- Output FD: %d\n", simpleCommand->outputFD);
    LOG_PRINT("--------------------\n");
}

/*----------------------------------------------------------------------------------------*/