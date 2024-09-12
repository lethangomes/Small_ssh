#include "builtInCommands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//cd command
int changeDirectory(Command * command, char * directory)
{
    if(command->numArguments > 1)
    {
        if(chdir(command->arguments[1]))
        {
            //failed to change directory
            return 1;
        }
    }
    else
    {
        chdir(getenv("HOME"));
    }

    //update directory variable
    getcwd(directory, 1000);
    printf(directory);
    fflush(stdout);
    return 0;
}

//prints status
void printStatus(int status)
{
    printf("exit value %d\n", status);
    fflush(stdout);
}

//exit command
void closeProcesses(int numProcesses, int * processes)
{
    int i;
    for(i = 0; i < numProcesses; i++)
    {
        kill(processes[i], 9);
    }
}