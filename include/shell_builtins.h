/**
 * @file builtins.h
 * @author Abdul Rafay (24100173@lums.edu.pk)
 * @brief Represents the builtins of the shell.
 * @version 0.1
 * @date 2023-06-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef BUILTINS_H
#define BUILTINS_H

#include "command.h"

#define HOME_DIR getenv("HOME")
#define MAX_PATH_LENGTH 1024

typedef int (*ExecutionFunction)(SimpleCommand*);

/**
 * @brief Returns the execution function for the given command.
 * 
 * @param commandName The name of the command.
 * @return ExecutionFunction The execution function for the given command.
*/
ExecutionFunction getExecutionFunction(char* commandName);

/**
 * @brief This function is the builtin for the cd command.
 * 
 * @param command The command to be executed.
 * @return int Returns 0 on success, -1 on failure.
 */
int cd(SimpleCommand* command);

/**
 * @brief This function is the builtin for the exit command.
 * 
 * @param command The command to be executed.
 * @return int Returns 0 on success, -1 on failure.
 */
int exitShell(SimpleCommand* command);

/**
 * @brief This function is the builtin for the pwd command.
 * 
 * @param command The command to be executed.
 * @return int Returns 0 on success, -1 on failure.
 */
int pwd(SimpleCommand* command);

/**
 * @brief This function is the builtin for the echo command.
 * 
 * @param command The command to be executed.
 * @return int Returns 0 on success, -1 on failure.
 */
int echo(SimpleCommand* command);

/**
 * @brief This function is the builtin for the alias command.
 * 
 * @param command The command to be executed.
 * @return int Returns 0 on success, -1 on failure.
 */
int alias(SimpleCommand* command);

/**
 * @brief This function is the builtin for the unalias command.
 * 
 * @param command The command to be executed.
 * @return int Returns 0 on success, -1 on failure.
 */
int unalias(SimpleCommand* command);

/**
 * @brief This function is the builtin for the history command.
 * 
 * @param command The command to be executed.
 * @return int Returns 0 on success, -1 on failure.
 */
int history(SimpleCommand* command);

/**
 * @brief This function executes a process.
 * 
 * The process is executed by forking a child process, and then executing the command in the child process.
 * 
 * @param command The command to be executed.
 * @return int Returns non-zero status on failue. else returns 0 on success
 */
int executeProcess(SimpleCommand* command);

#endif // BUILTINS_H