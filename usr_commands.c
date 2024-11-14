#include "usr_commands.h"
#include "thread.h"

#include "animation_manager.h"
#include "addr_led_driver.h"

char line_buf[SHELL_BUFFER_SIZE];

static int test_command(int argc, char **argv)
{
	printf("%d\n", argc);
	return 0;
}

static const shell_command_t commands[] = {
	{ "test", "test", test_command },
	{ "anim", "Animation man commands", AnimationMan_TakeUsrCommand},
	{ "aled", "Aled driver commands", AddrLedDriver_TakeUsrCommand},
	{ NULL, NULL, NULL }
};


void UserCommand_Init(void)
{
	memset(&line_buf, 0x0, SHELL_BUFFER_SIZE);
	shell_run(commands, &line_buf, SHELL_BUFFER_SIZE);
}

