#include "shell.h"
#define SHELL_BUFFER_SIZE (SHELL_DEFAULT_BUFSIZE)
#define ASSERT_ARGS(argcExpected) {if (argc < argcExpected) {printf("Bad args! argc %d expected %d\n", argc, argcExpected); return;}}
#define BAD_ARG() {printf("Bad arg!\n"); UserCommand_PrintCommand(argc, argv);}

void UserCommand_Init(void);
// void UserCommand_ThreadHandler(void *args);
