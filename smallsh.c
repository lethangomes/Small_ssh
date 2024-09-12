#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "smallsh.h"
#include "builtInCommands.h"

static int lastStatus = 0;
static char directory[1000];
static int processes[500];
static int numProcesses = 0;
static int foregroundMode = 0;

void SIGTSTPhandler()
{
    foregroundMode = !foregroundMode;

    if(foregroundMode)
    {
        write(STDOUT_FILENO, "\nEntering foreground-only mode (& is ignored)\n:", 47);
    } else
    {
        write(STDOUT_FILENO, "\nExiting foreground-only mode\n:", 31);
    }
}

int main()
{
    //set up SIGINT handler
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &SIGINT_action, NULL);

    //set up SIGTSTP handler
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = SIGTSTPhandler;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    //set current path
    getcwd(directory, 1000);

    char commandLine[2049];

    Command command;
    command.inputFile = NULL;
    command.outputFile = NULL;
    command.command = NULL;
    command.numArguments = 0;
    while(1)
    {
        //check processes
        checkProcesses();

        //prompt
        printf(":");
        fflush(stdout);

        //get command
        fgets(commandLine, 2048, stdin);
        processCommandLine(&command, commandLine);

        //process command
        if(!command.isComment)
        {
            if(processCommand(&command))
            {
                freeCommand(&command);
                break;
            } 
        }

        //printCommand(&command);
        freeCommand(&command);
    }
    
}

//processes command
int processCommand(Command * command)
{
    if(!strcmp(command->command, "exit"))
    {
        //close all running processes
        closeProcesses(numProcesses, processes);
        return 1;
    }
    else if(!strcmp(command->command, "cd"))
    {
        //change directory
        changeDirectory(command, directory);
    }
    else if(!strcmp(command->command, "status"))
    {
        printStatus(lastStatus);
    }
    else
    {
        executeCommand(command);
    }

    return 0;
}

//converts commandline input to command struct
void processCommandLine(Command * command, char commandLine[2049])
{
    //remove last character (it's a newline char or something)
    commandLine[strlen(commandLine) - 1] = '\0';

    //set isbackground, input, and output files to default values
    command->command = NULL;
    command->inputFile = NULL;
    command->outputFile = NULL;
    command->isBackground = 0;
    command->numArguments = 0;
    command->isComment = 0;

    //initialize argument array
    int i;
    for(i = 0; i < 512; i++)
    {
        command->arguments[i] = NULL;
    }

    //check if command line is empty or is a comment
    if(strlen(commandLine) == 0 || commandLine[0] == '#')
    {
        command->isComment = 1;
        return;
    }

    //expand $$
    pidExpander(commandLine);

    //separate by space
    char * token = strtok(commandLine, " ");

    //first word is command
    command->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(command->command, token);

    //add command to arguments
    command->arguments[command->numArguments] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(command->arguments[command->numArguments++], token);
    token = strtok(NULL, " ");

    //go through each other word
    while(token != NULL)
    {
        if(!strcmp(token, ">")) //output redirection
        {
            token = strtok(NULL, " ");
            command->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->outputFile, token);
        } else if(!strcmp(token, "<")) //input redirection
        {
            token = strtok(NULL, " ");
            command->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->inputFile, token);
        } else //regular argument
        {
            command->arguments[command->numArguments] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(command->arguments[command->numArguments], token);
            command->numArguments++;
        }
        token = strtok(NULL, " ");
    }

    //check if foreground only mode is active
    if(!strcmp(command->arguments[command->numArguments - 1], "&"))
    {
        if(!foregroundMode)
        {
            command->isBackground = 1;
        }
        free(command->arguments[--(command->numArguments)]);
        command->arguments[command->numArguments] = NULL;
    }
}

//expands $$ into pid
void pidExpander(char commandLine[2049])
{
    int i;
    char newCommandLine[2049] = "";
    int lastIndex = 0;

    //finds all instances of $$ in command
    for(i = 0; i < strlen(commandLine) - 1; i++)
    {
        if(commandLine[i] == '$' && commandLine[i+1] == '$')
        {
            //gets the string before the $$
            char temp[2049] = "";
            strncpy(temp, commandLine + lastIndex, i - lastIndex);
            lastIndex = i + 2;

            //add string to newCommandline
            strcat(newCommandLine, temp);

            //append pid to newCommandline
            snprintf(temp, 10, "%d", getpid());
            strcat(newCommandLine, temp);
            i++;
        }
    }

    //append leftover characters to newCommandLine
    char temp[2049] = "";
    strncpy(temp, commandLine + lastIndex, strlen(commandLine) - lastIndex);
    strcat(newCommandLine, temp);

    //printf(newCommandLine);
    //fflush(stdout);

    //return modified commandline
    strcpy(commandLine, newCommandLine);
}


//frees command struct
void freeCommand(Command * command)
{
    //free members
    if(command->command != NULL) free(command->command);
    if(command->outputFile != NULL) free(command->outputFile);
    if(command->inputFile != NULL) free(command->inputFile);
    
    //free arguments
    int i;
    for(i = 0; i < command->numArguments; i++)
    {
        free(command->arguments[i]);
    }
}

//prints command struct
void printCommand(Command * command)
{
    if(!command->isComment)
    {
        printf("Command: \t%s\n", command->command);
        printf("Out file: \t%s\n", command->outputFile);
        printf("In file: \t%s\n", command->inputFile);
        printf("Is background: \t%d\n", command->isBackground);
        printf("Num arguments: \t%d\n", command->numArguments);

        int i;
        for(i = 0; i < command->numArguments + 3; i++)
        {
            printf("Argument %d: \t%s\n", i, command->arguments[i]);
        }
        fflush(stdout);
    }
}

//executes non-built-in commands
void executeCommand(Command * command)
{
    //fork child
    int pid = fork();
    int status;
    int inputFd = -1;
    int outputFd = -1;

    struct sigaction child_SIGTSTP_ACTION = {0};

    switch(pid)
    {
        case 0: //child
            //set up signal handler to ignore SIGSTP
            child_SIGTSTP_ACTION.sa_handler = SIG_IGN;
            sigfillset(&child_SIGTSTP_ACTION.sa_mask);
            sigaction(SIGTSTP, &child_SIGTSTP_ACTION, NULL);

            //background command
            if(command->isBackground)
            {
                //set up signal handler to not kill background processes on SIG_INT
                struct sigaction child_SIGINT_ACTION = {0};
                child_SIGINT_ACTION.sa_handler = SIG_IGN;
                sigfillset(&child_SIGINT_ACTION.sa_mask);
                sigaction(SIGINT, &child_SIGINT_ACTION, NULL);

                if(command->inputFile == NULL)
                {
                    command->inputFile = calloc(strlen("/dev/null"), sizeof(char));
                    command->inputFile = "/dev/null";
                }
                if(command->outputFile == NULL)
                {
                    command->outputFile = calloc(strlen("/dev/null"), sizeof(char));
                    command->outputFile = "/dev/null";
                }
            }
            

            //input redirection
            if(command->inputFile != NULL)
            {
                //try to open file
                inputFd = open(command->inputFile, O_RDONLY, 0640);

                if(inputFd == -1)
                {
                    printf("Could not open %s for input\n", command->inputFile);
                    fflush(stdout);
                    exit(1);
                }

                dup2(inputFd, STDIN_FILENO);
            }

            //output redirection
            if(command->outputFile != NULL)
            {
                //try to open file
                outputFd = open(command->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);

                if(outputFd == -1)
                {
                    printf("Could not open %s for output\n", command->outputFile);
                    fflush(stdout);
                    exit(1);
                }

                dup2(outputFd, STDOUT_FILENO);
            }

            status = execvp(command->command, command->arguments);

            if(status == -1)
            {
                printf("%s: No such file or directory\n", command->command);
                fflush(stdout);
                exit(1);
            }
            break;
        case -1:
            //error
            printf("Fork error\n");
            fflush(stdout);
            break;
        default:
            //parent
            if(!command->isBackground)
            {
                //foreground command

                //block sigtstp signal until process completes
                __sigset_t signalSet;
                __sigaddset(&signalSet, SIGTSTP);
                sigprocmask(SIG_BLOCK,  &signalSet, NULL);

                waitpid(pid, &status, WCONTINUED);

                //unblock sigtstp
                sigprocmask(SIG_UNBLOCK,  &signalSet, NULL);

                if(status != 0)
                {
                    if(status != 256 && status )
                    {
                        //process was interrupted by signal
                        printf("Terminated by signal %d\n", status);
                    }
                    lastStatus = 1;

                } else 
                {
                    lastStatus = 0;
                }
            }
            else
            {
                //background command
                processes[numProcesses++] = pid;
                printf("Background pid is %d\n", pid);

            }
            
            //removePID(pid);
    } 

}

void removePID(int pid)
{
    int i;
    for(i = 0; i < numProcesses; i++)
    {
        if(processes[i] == pid)
        {
            processes[i] = processes[--numProcesses];
        }
    }
}

void checkProcesses()
{
    int i;
    for(i = 0; i < numProcesses; i++)
    {
        int status = 0;
        if(waitpid(processes[i], &status, WNOHANG))
        {
            if(WIFEXITED(status))
            {
                printf("Background pid %d is done: exit value:  %d\n", processes[i], WEXITSTATUS(status));
            }
            else if(WIFSIGNALED(status))
            {
                printf("Background pid %d is done: terminated by signal:  %d\n", processes[i], WTERMSIG(status));
            }
            removePID(processes[i]);
        }
        
    }
}