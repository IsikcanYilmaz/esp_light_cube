#include <stdbool.h>
#include "addr_led_driver.h"

#define ASSERT_ARGS(argcExpected) {if (argc < argcExpected) {ESP_LOGE(TAG, "Bad args! argc %d expected %d\n", argc, argcExpected); return 1;}}
#define BAD_ARG() {ESP_LOGE(TAG,"Bad arg!\n");}

bool UserCommand_Init(void);
Position_e UserCommand_PositionStringToVal(char *str);
