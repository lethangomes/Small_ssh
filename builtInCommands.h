#ifndef BUILTINCOMMANDSH
#define BUILTINCOMMANDSH
#include "command.h"

int changeDirectory(Command * command, char * directory);
void printStatus(int status);
void closeProcesses(int numProcesses, int * processes);


#endif