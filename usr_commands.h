#include "addr_led_driver.h"
#include "shell.h"
#define SHELL_BUFFER_SIZE (SHELL_DEFAULT_BUFSIZE)
#define ASSERT_ARGS(argcExpected) {if (argc < argcExpected) {printf("Bad args! argc %d expected %d\n", argc, argcExpected); return 1;}}
#define BAD_ARG() {printf("Bad arg!\n"); UserCommand_PrintCommand(argc, argv);}

void UserCommand_Init(void);
Position_e UserCommand_PositionStringToVal(char *str);
void UserCommand_CoordStringToVal(char *xy, uint8_t *x, uint8_t y);
void UserCommand_RgbStringToVal(char *rgb, uint8_t *r, uint8_t *g, uint8_t *b);
// void UserCommand_ThreadHandler(void *args);
