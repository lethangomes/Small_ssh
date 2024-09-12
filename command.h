#ifndef COMMANDH
#define COMMANDH

struct Command{
    char * command;
    char * arguments[512];
    int numArguments;
    char * inputFile;
    char * outputFile;
    int isBackground;
    int isComment;
};
typedef struct Command Command;
#endif