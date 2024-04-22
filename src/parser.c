#ifndef PARSER_H_
#define PARSER_H_

#include "parser.h"
#include "shell_builtins.h"
#include "hashtable.h"

#include <fcntl.h>
#include <glob.h>

extern hashtable* aliases;

#define COMPARE_TOKEN(token, string) (token && strcmp(token, string) == 0)

// Parses an array of tokens and generates a command chain, where each link is a table of commands to be executed.
CommandChain* parseTokens(char** tokens)
{
    CommandChain* chain = initCommandChain();
    if (!chain)
    {
        LOG_DEBUG("Failed to allocate memory for command chain\n");
        return NULL;
    }

    int currentIndexInTokens = 0;

    while (tokens[currentIndexInTokens] != NULL)
    {
        // the main loop adds commands to the chain
        Command* command = initCommand();
        if (!command)
        {
            LOG_DEBUG("Failed to allocate memory for command\n");
            cleanUpCommandChain(chain);
            return NULL;
        }

        // the simple commands are added to the command using this temporary
        SimpleCommand* simpleCommand = initSimpleCommand();
        if (!simpleCommand)
        {
            LOG_DEBUG("Failed to allocate memory for simple command\n");
            cleanUpCommandChain(chain);
            cleanUpCommand(command);
            return NULL;
        }

        // processing the tokens, until we have a chaining operator
        for (; !IS_NULL(tokens[currentIndexInTokens]) && !IS_CHAINING_OPERATOR(tokens[currentIndexInTokens]); currentIndexInTokens++)
        {
            if (IS_NULL(tokens[currentIndexInTokens]))
            {
                // push the simpleCommand to the command's simple commands
                if (!simpleCommand->commandName)
                {
                    LOG_DEBUG("Parse error. Null command encountered\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }
                simpleCommand->execute = getExecutionFunction(simpleCommand->commandName);
                addSimpleCommand(command, simpleCommand);
                simpleCommand = NULL; // no more simple commands
                break;
            }
            else if (IS_PIPE(tokens[currentIndexInTokens]))
            {
                // create a pipe, and update the current simple command's outputFD. push the simple command to the command's simple commands, and then create a new simple command, setting its inputFD to the pipe's read end

                // if there's two pipes in a row, or no command before the pipe, that is a grammar error
                // if there's two pipes, the current simple command will be empty
                if (!simpleCommand->commandName)
                {
                    LOG_DEBUG("Parse error near \'%s\'\n", tokens[currentIndexInTokens]);
                    cleanUpCommandChain(chain);           // cleans up the chain built so far. note that this chain does not contain the current command, and simple command temporaries, so we can clean them up separately
                    cleanUpCommand(command);              // command chain link which hasn't been added to the chain yet, so we can clean it up separately
                    cleanUpSimpleCommand(simpleCommand);  // probably a newly initialized simple command, so it hasn't been added to the command yet, so we can clean it up separately
                    return NULL;
                }

                // only update the outputFD if it is not stdout, if its not stdout, that indicates that the simple command already has an outputFD, and we cannot pipe to multiple commands
                if (simpleCommand->outputFD != STDOUT_FD)
                {
                    LOG_DEBUG("Parse error. Cannot pipe to multiple commands\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                int pipeFD[2];
                if (pipe(pipeFD) == -1)
                {
                    LOG_DEBUG("Failed to create pipe\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                simpleCommand->outputFD = pipeFD[PIPE_WRITE_END];
                simpleCommand->execute = getExecutionFunction(simpleCommand->commandName);
                addSimpleCommand(command, simpleCommand);

                // start with a new simple command
                simpleCommand = initSimpleCommand();
                if (!simpleCommand)
                {
                    LOG_DEBUG("Failed to allocate memory for simple command\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    return NULL;
                }

                // update the new simple command's inputFD, to connect the previous simpleCommand and the new simpleCommand via pipe
                simpleCommand->inputFD = pipeFD[PIPE_READ_END];
            }
            else if (IS_FILE_OUT_REDIR(tokens[currentIndexInTokens]))
            {
                // open the file, and update the current simple command's outputFD.

                // note that the following comparison is safe, because the last token in the tokens array is always NULL, and the current token is not NULL, so we can safely access the next memory location
                if (!simpleCommand->commandName)
                {
                    LOG_DEBUG("Parse error. Output redirection encountered before command\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                if (tokens[currentIndexInTokens + 1] == NULL)
                {
                    LOG_DEBUG("No file specified for output redirection\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                // check if the current outputFD is not stdout
                if (simpleCommand->outputFD != STDOUT_FD)
                {
                    LOG_DEBUG("Cannot redirect output to multiple files\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                int fileFD = -1;
                if (IS_APPEND(tokens[currentIndexInTokens]))
                {
                    fileFD = open(tokens[currentIndexInTokens + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                }
                else
                {
                    fileFD = open(tokens[currentIndexInTokens + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }

                if (fileFD == -1)
                {
                    LOG_DEBUG("Failed to open file for output redirection\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                simpleCommand->outputFD = fileFD;
                currentIndexInTokens++;
            }
            else if (IS_FILE_IN_REDIR(tokens[currentIndexInTokens]))
            {
                // what to do with file input redirection
                if (tokens[currentIndexInTokens + 1] == NULL)
                {
                    LOG_DEBUG("No file specified for input redirection\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                // check if the current inputFD is not stdin

                int fileFD = open(tokens[currentIndexInTokens + 1], O_RDONLY);
                if (fileFD == -1)
                {
                    LOG_DEBUG("Failed to open file for input redirection\n");
                    cleanUpCommandChain(chain);
                    return NULL;
                }

                if (simpleCommand->inputFD != STDIN_FD)
                {
                    LOG_DEBUG("Cannot redirect input from multiple files\n");
                    cleanUpCommandChain(chain);
                    cleanUpCommand(command);
                    cleanUpSimpleCommand(simpleCommand);
                    return NULL;
                }

                simpleCommand->inputFD = fileFD;
                currentIndexInTokens++;
            }
            else if (IGNORE(tokens[currentIndexInTokens]))
            {
                continue;
            }
            else
            {
                // modify the token to remove the quotes (if any)
                tokens[currentIndexInTokens] = removeQuotes(tokens[currentIndexInTokens]);

                // check if this token is an alias
                const char* key = tokens[currentIndexInTokens];
                const char* value = get(aliases, key);

                // if the token is an alias, then we need to expand it, and push the expanded tokens to the args array
                // the alias only needs to be expanded when its used as a command, and not as an argument to a command
                if (!simpleCommand->commandName && value)
                {
                    char** aliasTokens = tokenizeString(value, ' ');
                    if (!aliasTokens)
                    {
                        LOG_DEBUG("Failed to tokenize alias value\n");
                        freeTokens(aliasTokens);
                        cleanUpCommandChain(chain);
                        cleanUpCommand(command);
                        cleanUpSimpleCommand(simpleCommand);
                        return NULL;
                    }

                    for (int i = 0; aliasTokens[i] != NULL; i++)
                    {
                        if (pushArgs(aliasTokens[i], simpleCommand) != 0)
                        {
                            LOG_DEBUG("Failed to push argument to simple command\n");
                            freeTokens(aliasTokens);
                            cleanUpCommandChain(chain);
                            cleanUpCommand(command);
                            cleanUpSimpleCommand(simpleCommand);
                            return NULL;
                        }
                    }

                    freeTokens(aliasTokens);
                }
                else 
                {
                    // expand any wildcards, in case there are any, if there's none return the same token
                    glob_t globbuf;
                    int globReturn = glob(tokens[currentIndexInTokens], GLOB_NOCHECK | GLOB_TILDE, NULL, &globbuf);

                    if (globReturn != 0)
                    {
                        LOG_DEBUG("Failed to expand glob\n");
                        globfree(&globbuf);
                        cleanUpCommandChain(chain);
                        cleanUpCommand(command);
                        cleanUpSimpleCommand(simpleCommand);
                        return NULL;
                    }

                    // if the glob was successful, then we need to push the expanded tokens to the args array, note if there was no expansion, then the globbuf.gl_pathc will be 1
                    for (size_t i = 0; i < globbuf.gl_pathc; i++)
                    {
                        if (pushArgs(globbuf.gl_pathv[i], simpleCommand) != 0)
                        {
                            LOG_DEBUG("Failed to push argument to simple command\n");
                            globfree(&globbuf);
                            cleanUpCommandChain(chain);
                            cleanUpCommand(command);
                            cleanUpSimpleCommand(simpleCommand);
                            return NULL;
                        }
                    }

                    globfree(&globbuf);
                }
            }
        }
        
        // push the last simple command to the command's simple commands
        if (simpleCommand && simpleCommand->commandName)
        {
            // add the simple command to the command's simple commands
            simpleCommand->execute = getExecutionFunction(simpleCommand->commandName);
            addSimpleCommand(command, simpleCommand);
            simpleCommand = NULL; // no more simple commands
        }

        // update the chain operator
        command->chainingOperator = COPY(tokens[currentIndexInTokens]);

        // add the command to the chain
        addCommandToChain(chain, command);

        // increment the counter if current token is not NULL
        if (tokens[currentIndexInTokens])
        {
            currentIndexInTokens++;
        }
    }

    return chain;
}

#endif /* PARSER_H_ */