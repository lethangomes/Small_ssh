To compile the code, enter the "make" command in the terminal while in the directory with the code. If that doesn't
work, enter this command:

gcc smallsh.c builtInCommands.c -g -o smallsh

or enter these commands in sequence

gcc -g -c builtInCommands.c 
gcc -g -c smallsh.c
gcc smallsh.o builtInCommands.o -g -o smallsh