#ifndef SMALLSHH
#define SMALLSHH
#include "command.h"

void processCommandLine(struct Command * command, char commandLine[2049]);
void freeCommand(struct Command * command);
void printCommand(struct Command * command);
int processCommand(struct Command * command);
void pidExpander(char commandLine[2049]);
void executeCommand(Command * command);
void removePID(int pid);
void checkProcesses();

#endif