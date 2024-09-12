smallsh : smallsh.o builtInCommands.o 
	gcc smallsh.o builtInCommands.o -g -o smallsh

smallsh.o : smallsh.c smallsh.h command.h builtInCommands.h 
	gcc -g -c smallsh.c

builtInCommands.o : builtInCommands.c builtInCommands.h command.h 
	gcc -g -c builtInCommands.c 
