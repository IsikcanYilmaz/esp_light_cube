#include "usr_commands.h"
#include "thread.h"

#include "animation_manager.h"
#include "addr_led_driver.h"

char line_buf[SHELL_BUFFER_SIZE];

static int test_command(int argc, char **argv)
{
	char *str = "Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet \n \
Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet \n \
Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet \n \
Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet \n \
Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet \n \
Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet \n \
Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet Lorem ipsum dolor sit amet\n";
	logprint("%s\n", str);
	AddrLedDriver_SetPixelRgbInPanel(SOUTH, 0, 0, 10, 0, 0);
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

Position_e UserCommand_PositionStringToVal(char *str)
{
	Position_e pos;
	if (strcmp(str, "n") == 0)
	{
		pos = NORTH;
	}
	else if (strcmp(str, "e") == 0)
	{
		pos = EAST;
	}
	else if (strcmp(str, "s") == 0)
	{
		pos = SOUTH;
	}
	else if (strcmp(str, "w") == 0)
	{
		pos = WEST;
	}
	else if (strcmp(str, "t") == 0)
	{
		pos = TOP;
	}
	else
	{
		pos = NUM_SIDES;
		logprint("BAD SIDE DESCRIPTOR! %s %s\n", __FUNCTION__, str);
	}
	return pos;
}

void UserCommand_CoordStringToVal(char *xy, uint8_t *x, uint8_t y)
{
}

void UserCommand_RgbStringToVal(char *rgb, uint8_t *r, uint8_t *g, uint8_t *b)
{
}

