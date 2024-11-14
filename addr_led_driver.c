#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "addr_led_driver.h"
#include "usr_commands.h"

#include "ztimer.h"
#include "xtimer.h"
#include "thread.h"

// WS2812B Related 
ws281x_t neopixelHandle;
uint8_t pixelBuffer[NEOPIXEL_SIGNAL_BUFFER_LEN];
ws281x_params_t neopixelParams = {
	.buf = pixelBuffer,
	.numof = NUM_LEDS,
	.pin = NEOPIXEL_SIGNAL_GPIO_PIN
};

// Cube related
Pixel_t ledStrip0Pixels[NUM_LEDS];
AddrLedStrip_t ledStrip0;
AddrLedPanel_t ledPanels[NUM_PANELS];
bool pixelChanged = true;
bool addrLedDriverInitialized = false;

char *positionStrings[NUM_SIDES] = {
	[NORTH] = "NORTH",
	[EAST] = "EAST",
	[SOUTH] = "SOUTH",
	[WEST] = "WEST",
	[TOP] = "TOP"
};

static void InitPanel(AddrLedPanel_t *p)
{
  // Set local coordinates of this panel
  Pixel_t *pixels = p->strip->pixels;
  for (int i = 0; i < p->numLeds; i++)
  {
    Pixel_t *pixel = &pixels[p->stripRange[0] + i];
    pixel->x = NUM_LEDS_PER_PANEL_SIDE - (i / NUM_LEDS_PER_PANEL_SIDE);
    pixel->y = NUM_LEDS_PER_PANEL_SIDE - (i % NUM_LEDS_PER_PANEL_SIDE);
    
    // TODO // HANDLE GLOBAL COORDINATES
  }

  // Set global coordinates of this panel
}

void AddrLedDriver_Init(void)
{
	// First init our data structures 
	// Initialize the strip(s). This initializes one continuous strip. 
  // If multiple panels are daisychained, that counts as one strip.
  ledStrip0 = (AddrLedStrip_t) {
    .numLeds                 = NUM_LEDS,
    .pixels                  = (Pixel_t *) &ledStrip0Pixels,
		.neopixelHandle          = &neopixelHandle
  };

  // Initialize the panel structures
  for (int panelIdx = 0; panelIdx < NUM_PANELS; panelIdx++)
  {
    Position_e pos = (Position_e) panelIdx;
    AddrLedPanel_t p = {
      .strip = &ledStrip0,
      .numLeds = NUM_LEDS_PER_PANEL,
      .stripRange = {(panelIdx * NUM_LEDS_PER_PANEL), ((panelIdx + 1) * NUM_LEDS_PER_PANEL - 1)},
      .position = pos,
      .neighborPanels = {NULL, NULL, NULL, NULL}, // TODO
    };
    p.stripFirstPixel = &(p.strip->pixels[p.stripRange[0]]);
    InitPanel(&p);
    ledPanels[pos] = p;

		// Initialize the neighbor fields of our pixels. lets see if it works
		for (int x = 0; x < NUM_LEDS_PER_PANEL_SIDE; x++)
		{
			for (int y = 0; y < NUM_LEDS_PER_PANEL_SIDE; y++)
			{
				Pixel_t *pix = AddrLedDriver_GetPixelInPanel(pos, x, y);
				
				// NSWE neighbors
				if (x > 0)
				{

				}

				if (x < NUM_LEDS_PER_PANEL_SIDE-1)
				{

				}

			}
		}
  }

	// Initialize our pixels // TODO find a better solution to this? this is for compatibility w the RIOT ws281x module
	// Lets see if its even needed lol
	for (int pIdx = 0; pIdx < NUM_LEDS; pIdx++)
	{
		ledStrip0Pixels[pIdx].stripIdx = pIdx;
	}

	// Init the ws281x module 
	if(ws281x_init(&neopixelHandle, &neopixelParams) != 0) 
	{
		printf("Failed to initialize ws281\n");
		return;
	}
	else 
	{
		printf("Initialized ws281x. Data length %d one length %d zero length %d\n", WS281X_T_DATA_NS, WS281X_T_DATA_ONE_NS, WS281X_T_DATA_ZERO_NS);
	}

	addrLedDriverInitialized = true;
}

static void teststrips(ws281x_t *strip)
{
	// Color_t testHsv = Color_CreateFromHsv(50, 0.5, 0.5);
	color_rgb_t color2 = {10, 0, 0};
	color_rgb_t color1 = {10, 0, 10};
	static bool onColor1 = true;
	for(int i = 0; i < strip->params.numof; i++)
	{
		if (i % 2 == 0)
		{
			onColor1 ? ws281x_set(strip, i, color1) : ws281x_set(strip, i, color2);
		}
		else
		{
			onColor1 ? ws281x_set(strip, i, color2) : ws281x_set(strip, i, color1);
		}
	}
	// ws281x_set(strip, 0, color1);
	onColor1 = !onColor1;
}

static void testsetpixel(ws281x_t *strip)
{
	Pixel_t *northTop = AddrLedDriver_GetPixelInPanel(SOUTH, 1, 1);
	AddrLedDriver_SetPixelRgbInPanel(NORTH, 0, 0, 20, 0, 0);
	AddrLedDriver_SetPixelRgbInPanel(SOUTH, 0, 0, 20, 0, 0);
	AddrLedDriver_SetPixelRgbInPanel(WEST, 0, 0, 20, 0, 0);
	AddrLedDriver_SetPixelRgbInPanel(EAST, 0, 0, 20, 0, 0);
	AddrLedDriver_SetPixelRgbInPanel(TOP, 0, 0, 20, 0, 0);
}

void AddrLedDriver_Test(void)
{
	teststrips(&neopixelHandle);
	// ws281x_write(&neopixelHandle);

	// testsetpixel(&neopixelHandle);
	AddrLedDriver_DisplayStrip(&ledStrip0);
}

void AddrLedDriver_DisplayStrip(AddrLedStrip_t *l)
{
	ws281x_write(&neopixelHandle);
	pixelChanged = false;
}

void AddrLedDriver_DisplayCube(void)
{
	// ztimer_now_t t0 = ztimer_now(ZTIMER_USEC); // Takes around 140 ms which is too goddam much. pico takes 5ms because it dmas the data TODO do that here
	AddrLedDriver_DisplayStrip(&ledStrip0);
	// ztimer_now_t t1 = ztimer_now(ZTIMER_USEC);
	// printf("Display strip took %d ms\n", (t1-t0));
}

void AddrLedDriver_SetPixelRgb(Pixel_t *p, uint8_t r, uint8_t g, uint8_t b)
{
  p->red = r;
  p->green = g;
  p->blue = b;
	ws281x_set(&neopixelHandle, p->stripIdx, (color_rgb_t){r, g, b});
	pixelChanged = true;
}

// ! Below depends on the LED structs that this file initializes. these could reside in manager code? but not now
// fuck it. this _is_ the manager code. until further notice.
void AddrLedDriver_SetPixelRgbInPanel(Position_e pos, uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
  // sanity checks
  if (pos >= NUM_SIDES || x > NUM_LEDS_PER_PANEL_SIDE || y > NUM_LEDS_PER_PANEL_SIDE)
  {
    printf("Incorrect args to SetPixelRgb %d %d %d\n", pos, x, y);
    return;  // TODO have an error type
  }
  AddrLedPanel_t *panel = AddrLedDriver_GetPanelByLocation(pos);
  Pixel_t *pixel = AddrLedDriver_GetPixelInPanel(pos, x, y);
	AddrLedDriver_SetPixelRgb(pixel, r, g, b);
}

void AddrLedDriver_Clear(void)
{
	for (int i = 0; i < NUM_LEDS; i++)
	{
		AddrLedDriver_SetPixelRgb(&ledStrip0.pixels[i], 0, 0, 0);
	}
}

Pixel_t* AddrLedDriver_GetPixelInPanel(Position_e pos, uint8_t x, uint8_t y)
{
  AddrLedPanel_t *panel = AddrLedDriver_GetPanelByLocation(pos);
  AddrLedStrip_t *strip = panel->strip;
  uint8_t ledIdx;
#if LEDS_BEGIN_AT_BOTTOM
  y = NUM_LEDS_PER_PANEL_SIDE - y - 1;
#endif
	y = NUM_LEDS_PER_PANEL_SIDE - 1 - y;// HACK ! y seems inverted. flip it
	
  if (y % 2 == 0)
  {
    ledIdx = x + (NUM_LEDS_PER_PANEL_SIDE * y);
  }
  else
  {
    ledIdx = (NUM_LEDS_PER_PANEL_SIDE - 1 - x) + (NUM_LEDS_PER_PANEL_SIDE * y);
  }
  return &(panel->stripFirstPixel[ledIdx]);
}

// Idea behind this function is to retrieve a pixel from the point of view of a different position
// i.e. instead of looking at the panel from the south position, imagine transforming to another position
Pixel_t* AddrLedDriver_GetPixelInPanelRelative(Position_e pos, Position_e relativePos, uint8_t x, uint8_t y)
{
	uint8_t absX, absY;
	switch(relativePos)
	{
		case EAST:
		{
			absX = y;
			absY = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
			break;
		}
		case NORTH:
		{
			absX = y;
			absY = x;
			break;
		}
		case WEST:
		{
			absX = NUM_LEDS_PER_PANEL_SIDE - 1 - y;
			absY = x;
			break;
		}
		case SOUTH:
		default:
		{
			absX = x;
			absY = y;
			break;
		}
	}
	return AddrLedDriver_GetPixelInPanel(pos, absX, absY);
}

AddrLedPanel_t* AddrLedDriver_GetPanelByLocation(Position_e pos)
{
	if (pos >= NUM_SIDES)
	{
		printf("Bad pos %d for %s\n", pos, __FUNCTION__);
		return NULL;
	}
  return &ledPanels[pos];
}

bool AddrLedDriver_ShouldRedraw(void)
{
	return true;
	return pixelChanged;
}

char * AddrLedDriver_GetPositionString(Position_e pos)
{
	if (pos >= NUM_SIDES)
	{
		printf("Bad pos %d to %s\n", pos, __FUNCTION__);
		return NULL;
	}
	return positionStrings[pos];
}

AddrLedStrip_t* AddrLedDriver_GetStrip(void)
{
	return &ledStrip0;
}

void AddrLedDriver_TakeUsrCommand(int argc, char **argv)
{
	if (!addrLedDriverInitialized)
	{
		return;
	}
	ASSERT_ARGS(2);
	if (strcmp(argv[1], "clear") == 0)
	{
		AddrLedDriver_Clear();
	}
	// else if (strcmp(argv[1], "relset") == 0)
	// {
	// 	// aled relset <pos> <relpos> <x> <y> <r> <g> <b>
	// 	ASSERT_ARGS(9);
	// 	Position_e pos = NUM_SIDES;
	// 	if (strcmp(argv[2], "n") == 0)
	// 	{
	// 		pos = NORTH;
	// 	}
	// 	else if (strcmp(argv[2], "e") == 0)
	// 	{
	// 		pos = EAST;
	// 	}
	// 	else if (strcmp(argv[2], "s") == 0)
	// 	{
	// 		pos = SOUTH;
	// 	}
	// 	else if (strcmp(argv[2], "w") == 0)
	// 	{
	// 		pos = WEST;
	// 	}
	// 	else if (strcmp(argv[2], "t") == 0)
	// 	{
	// 		pos = TOP;
	// 	}
	// 	else
	// 	{
	// 		printf("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
	// 		return;
	// 	}
	// }
	else if (strcmp(argv[1], "set") == 0)
	{
		// aled set <pos> <x> <y> <r> <g> <b>
		ASSERT_ARGS(8);
		Position_e pos = NUM_SIDES;
		if (strcmp(argv[2], "n") == 0)
		{
			pos = NORTH;
		}
		else if (strcmp(argv[2], "e") == 0)
		{
			pos = EAST;
		}
		else if (strcmp(argv[2], "s") == 0)
		{
			pos = SOUTH;
		}
		else if (strcmp(argv[2], "w") == 0)
		{
			pos = WEST;
		}
		else if (strcmp(argv[2], "t") == 0)
		{
			pos = TOP;
		}
		else
		{
			printf("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
			return;
		}
		uint8_t x = atoi(argv[3]);
		uint8_t y = atoi(argv[4]);
		uint8_t r = atoi(argv[5]);
		uint8_t g = atoi(argv[6]);
		uint8_t b = atoi(argv[7]);
		printf("Setting pixel %s %d %d to %d %d %d\n", AddrLedDriver_GetPositionString(pos), x, y, r, g, b);
		AddrLedDriver_SetPixelRgbInPanel(pos, x, y, r, g, b);
	}
}
