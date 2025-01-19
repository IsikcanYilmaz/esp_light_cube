#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "addr_led_driver.h"
#include "button.h"
#include "usr_commands.h"

#include "ztimer.h"
#include "xtimer.h"
#include "thread.h"

#include "mutex.h"
#include "logger.h"

mutex_t lock;

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

char *directionStrings[NUM_DIRECTIONS] = {
	[UP] = "UP",
	[UP_LEFT] = "UP_LEFT",
	[LEFT] = "LEFT",
	[DOWN_LEFT] = "DOWN_LEFT",
	[DOWN] = "DOWN",
	[DOWN_RIGHT] = "DOWN_RIGHT",
	[RIGHT] = "RIGHT",
	[UP_RIGHT] = "UP_RIGHT"
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
  }

	// Set neighbors of our pixels. this needs to be done better. thankfully we do it once 
  for (int panelIdx = 0; panelIdx < NUM_PANELS; panelIdx++)
	{
    Position_e pos = (Position_e) panelIdx;

		// Initialize the neighbor fields of our pixels. lets see if it works
		for (int x = 0; x < NUM_LEDS_PER_PANEL_SIDE; x++)
		{
			for (int y = 0; y < NUM_LEDS_PER_PANEL_SIDE; y++)
			{
				Pixel_t *pix = AddrLedDriver_GetPixelInPanel(pos, x, y);
				pix->x = x; 
				pix->y = y;
				pix->pos = pos;
				memset(pix->neighborPixels, 0, sizeof(Pixel_t *) * NUM_DIRECTIONS);
				pix->neighborPixels;
				Position_e panelToTheRight = (pos + 1) % TOP;
				Position_e panelToTheLeft = (pos + 1) % TOP;
				// TODO refactor the block below.
				if (pos != TOP) // SIDE PANELS
				{
					if (x == 0) // leftmost column
					{
						// left neighbors are going to be in the panel to the left
						Position_e leftPanelPos = ((uint8_t) pos > 0) ? ((uint8_t) pos - 1) : EAST;
						pix->neighborPixels[LEFT] = AddrLedDriver_GetPixelInPanel(leftPanelPos, NUM_LEDS_PER_PANEL_SIDE-1, y);

						// if we're above the bottom
						if (y > 0)
						{
							pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(leftPanelPos, NUM_LEDS_PER_PANEL_SIDE-1, y-1);
							pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(pos, 1, y-1);
						}

						// if we're a corner, there is no top left neighbor
						// if we're not in a corner then there is a top left neighbor
						if (y < NUM_LEDS_PER_PANEL_SIDE-1)
						{
							pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(leftPanelPos, NUM_LEDS_PER_PANEL_SIDE-1, y+1);
							pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(pos, 1, y+1);
						}

						pix->neighborPixels[RIGHT] = AddrLedDriver_GetPixelInPanel(pos, x+1, y);	
					}
					else if (x == NUM_LEDS_PER_PANEL_SIDE-1) // rightmost column
					{
						// right neighbors are going to be in the panel to the right 
						Position_e rightPanelPos = ((uint8_t) pos + 1) % TOP;
						pix->neighborPixels[RIGHT] = AddrLedDriver_GetPixelInPanel(rightPanelPos, 0, y);

						// if we're above the bottom
						if (y > 0)
						{
							pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(rightPanelPos, 0, y-1);
							pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(pos, 2, y-1);
						}

						// if we're a corner, there is no top right neighbor
						// if we're not in a corner then there is a top right neighbor
						if (y < NUM_LEDS_PER_PANEL_SIDE-1)
						{
							pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(rightPanelPos, 0, y+1);
							pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(pos, 2, y+1);
						}

						pix->neighborPixels[LEFT] = AddrLedDriver_GetPixelInPanel(pos, 2, y);
					}
					else // middle two columns
					{
						pix->neighborPixels[LEFT] = AddrLedDriver_GetPixelInPanel(pos, x-1, y);
						pix->neighborPixels[RIGHT] = AddrLedDriver_GetPixelInPanel(pos, x+1, y);

						if (y < NUM_LEDS_PER_PANEL_SIDE-1)
						{
							pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(pos, x-1, y+1);
							pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(pos, x+1, y+1);
						}
						if (y > 0)
						{
							pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(pos, x-1, y-1);
							pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(pos, x+1, y-1);
						}
					}

					if (y == NUM_LEDS_PER_PANEL_SIDE - 1) // top
					{
						pix->neighborPixels[UP_LEFT] = (x > 0) ? AddrLedDriver_GetPixelInPanelRelative(TOP, pos, x-1, 0): NULL;
						pix->neighborPixels[UP_RIGHT] = (x < NUM_LEDS_PER_PANEL_SIDE - 1) ? AddrLedDriver_GetPixelInPanelRelative(TOP, pos, x+1, 0) : NULL;
					}

					// Upper neighbor
					pix->neighborPixels[UP] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? AddrLedDriver_GetPixelInPanel(pos, x, y+1) : AddrLedDriver_GetPixelInPanelRelative(TOP, pos, x, 0);

					// Lower neighbor
					pix->neighborPixels[DOWN] = (y > 0) ? AddrLedDriver_GetPixelInPanel(pos, x, y-1) : NULL;
				}
				else // TOP PANEL 
				{
					// we assume we're looking at the cube from the pov of S
					// So
					//   N   ^
					// W T E |
					//   S   |
					if (x == 0) // leftmost column
					{
						// Regardless of row, the left neighbor will be on the WEST panel and neighbor to the right will be on the TOP panel
						pix->neighborPixels[LEFT] = AddrLedDriver_GetPixelInPanel(WEST, NUM_LEDS_PER_PANEL_SIDE-1-y, NUM_LEDS_PER_PANEL_SIDE-1);
						pix->neighborPixels[RIGHT] = AddrLedDriver_GetPixelInPanel(pos, 1, y);
						pix->neighborPixels[DOWN_LEFT] = (y > 0) ? AddrLedDriver_GetPixelInPanel(WEST, NUM_LEDS_PER_PANEL_SIDE-y, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;
						pix->neighborPixels[UP_LEFT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? AddrLedDriver_GetPixelInPanel(WEST, NUM_LEDS_PER_PANEL_SIDE-2-y, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;
						if (y == 0) // bottom row
						{
							pix->neighborPixels[DOWN] = AddrLedDriver_GetPixelInPanel(SOUTH, x, NUM_LEDS_PER_PANEL_SIDE-1);
							pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(SOUTH, x+1, NUM_LEDS_PER_PANEL_SIDE-1);
							pix->neighborPixels[UP] = AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
							pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y+1);
						}
						else if (y < NUM_LEDS_PER_PANEL_SIDE-1) // mid rows
						{
							pix->neighborPixels[DOWN] = AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
							pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y-1);
							// pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y-1);
							pix->neighborPixels[UP] = AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
							pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y+1);
							// pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y+1);
						}
						else // top row
						{
							pix->neighborPixels[DOWN] = AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
							pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y-1);
							pix->neighborPixels[UP] = AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x, NUM_LEDS_PER_PANEL_SIDE-1);
							pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-2, NUM_LEDS_PER_PANEL_SIDE-1);
						}
					}
					else if (x < NUM_LEDS_PER_PANEL_SIDE-1) // mid columns 
					{
						pix->neighborPixels[LEFT] = AddrLedDriver_GetPixelInPanel(pos, x-1, y);
						pix->neighborPixels[RIGHT] = AddrLedDriver_GetPixelInPanel(pos, x+1, y);
						pix->neighborPixels[DOWN] = (y > 0) ? AddrLedDriver_GetPixelInPanel(pos, x, y-1) : AddrLedDriver_GetPixelInPanel(SOUTH, x, NUM_LEDS_PER_PANEL_SIDE-1);
						pix->neighborPixels[DOWN_RIGHT] = (y > 0) ? AddrLedDriver_GetPixelInPanel(pos, x+1, y-1) : AddrLedDriver_GetPixelInPanel(SOUTH, x+1, NUM_LEDS_PER_PANEL_SIDE-1);
						pix->neighborPixels[DOWN_LEFT] = (y > 0) ? AddrLedDriver_GetPixelInPanel(pos, x-1, y-1) : AddrLedDriver_GetPixelInPanel(SOUTH, x-1, NUM_LEDS_PER_PANEL_SIDE-1);
						pix->neighborPixels[UP] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? AddrLedDriver_GetPixelInPanel(pos, x, y+1) : AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x, NUM_LEDS_PER_PANEL_SIDE-1);
						pix->neighborPixels[UP_RIGHT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? AddrLedDriver_GetPixelInPanel(pos, x+1, y+1) : AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x-1, NUM_LEDS_PER_PANEL_SIDE-1);
						pix->neighborPixels[UP_LEFT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? AddrLedDriver_GetPixelInPanel(pos, x-1, y+1) : AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x+1, NUM_LEDS_PER_PANEL_SIDE-1);
					}
					else // rightmost column
					{
						// Regardless of row, the right neighbor will be on the EAST panel and neighbor to the left will be on the TOP panel
						pix->neighborPixels[LEFT] = AddrLedDriver_GetPixelInPanel(pos, x-1, y);
						pix->neighborPixels[RIGHT] = AddrLedDriver_GetPixelInPanel(EAST, y, NUM_LEDS_PER_PANEL_SIDE-1);
						pix->neighborPixels[DOWN_RIGHT] = (y > 0) ? AddrLedDriver_GetPixelInPanel(EAST, y-1, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;
						pix->neighborPixels[UP_RIGHT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? AddrLedDriver_GetPixelInPanel(EAST, y+1, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;

						if (y == 0) // bottom row
						{
							pix->neighborPixels[DOWN] = AddrLedDriver_GetPixelInPanel(SOUTH, x, NUM_LEDS_PER_PANEL_SIDE-1);
							pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(SOUTH, x-1, NUM_LEDS_PER_PANEL_SIDE-1);
							pix->neighborPixels[UP] = AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
							pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y+1);
						}
						else if (y < NUM_LEDS_PER_PANEL_SIDE-1) // mid rows
						{
							pix->neighborPixels[DOWN] = AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
							// pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y-1);
							pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y-1);
							pix->neighborPixels[UP] = AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
							// pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y+1);
							pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y+1);
						}
						else // top row
						{
							pix->neighborPixels[DOWN] = AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
							pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y-1);
							pix->neighborPixels[UP] = AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x, NUM_LEDS_PER_PANEL_SIDE-1);
							pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(NORTH, 1, NUM_LEDS_PER_PANEL_SIDE-1);
						}
					}
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
		logprint("Failed to initialize ws281\n");
		return;
	}
	else 
	{
		logprint("Initialized ws281x. Data length %d one length %d zero length %d\n", WS281X_T_DATA_NS, WS281X_T_DATA_ONE_NS, WS281X_T_DATA_ZERO_NS);
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

static void testfirstled(ws281x_t *strip)
{
	color_rgb_t color2 = {10, 0, 0};
	color_rgb_t color1 = {10, 0, 10};
	color_rgb_t colorBlank = {0,0,0};
	color_rgb_t colorBasic = {1,0,0};
	static bool onColor1 = true;
	// ws281x_set(strip, 0, (onColor1) ? color1 : color2);
	ws281x_set(strip, 0, colorBlank);
	ws281x_set(strip, 1, colorBasic);
	ws281x_set(strip, 2, colorBlank);
	ws281x_set(strip, 3, colorBasic);
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

static Position_e CharToPosEnum(char c)
{
	Position_e pos = NUM_SIDES;
	switch (c)
	{
		case 'n':
			pos = NORTH;
			break;
		case 'e':
			pos = EAST;
			break;
		case 's':
			pos = SOUTH;
			break;
		case 'w':
			pos = WEST;
			break;
		case 't':
			pos = TOP;
			break;
		default:
			logprint("BAD SIDE DESCRIPTOR! %c\n", c);
	}
	// else if (c == 'e')
	// {
	// 	pos = EAST;
	// }
	// else if (c == 's')
	// {
	// 	pos = SOUTH;
	// }
	// else if (c == 'w')
	// {
	// 	pos = WEST;
	// }
	// else if (c == 't')
	// {
	// 	pos = TOP;
	// }
	// else
	// {
	// 	logprint("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
	// }
	return pos;
}

void AddrLedDriver_Test(void)
{
	// teststrips(&neopixelHandle);
	// ws281x_write(&neopixelHandle);

	// testsetpixel(&neopixelHandle);
	while(true)
	{
		testfirstled(&neopixelHandle);
		AddrLedDriver_DisplayStrip(&ledStrip0);
		ztimer_sleep(ZTIMER_USEC, 1 * US_PER_SEC);
	}
}

void AddrLedDriver_DisplayStrip(AddrLedStrip_t *l)
{
	ws281x_write(&neopixelHandle);
}

void AddrLedDriver_DisplayCube(void)
{
	// mutex_lock(&lock);
	// static uint32_t avg = 0;
	// static uint32_t sum = 0;
	// static uint32_t ctr = 0;
	// uint32_t t0 = ztimer_now(ZTIMER_USEC);
	AddrLedDriver_DisplayStrip(&ledStrip0);
	// uint32_t t1 = ztimer_now(ZTIMER_USEC);
	// ctr++;
	// sum += (t1-t0);
	// avg = sum/ctr;
	// logprint("Display Strip took %d us\n", t1-t0);
	// logprint("avg %d us\n", avg);
	// pixelChanged = false;
	// mutex_unlock(&lock);
}

void AddrLedDriver_SetPixelRgb(Pixel_t *p, uint8_t r, uint8_t g, uint8_t b)
{
	if (p == NULL)
	{
		logprint("%s received null pixel_t pointer!\n", __FUNCTION__);
		return;
	}
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
  if (pos >= NUM_SIDES || x >= NUM_LEDS_PER_PANEL_SIDE || y >= NUM_LEDS_PER_PANEL_SIDE)
  {
    logprint("Incorrect args to SetPixelRgb %d %d %d\n", pos, x, y);
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

Pixel_t* AddrLedDriver_GetPixelByIdx(uint16_t idx) // a bit strip/situaiton dependent but oh well
{
	if (idx >= NUM_LEDS)
	{
		logprint("%s bad pixel idx %d\n", idx);
		return NULL;
	}
	return &ledStrip0Pixels[idx];
}

Pixel_t* AddrLedDriver_GetPixelInPanel(Position_e pos, uint8_t x, uint8_t y)
{
  AddrLedPanel_t *panel = AddrLedDriver_GetPanelByLocation(pos);
  AddrLedStrip_t *strip = panel->strip;
  uint8_t ledIdx;

	if (pos != TOP) // HACK !
	{
// #if LEDS_BEGIN_AT_BOTTOM
// 		y = NUM_LEDS_PER_PANEL_SIDE - y - 1;
// #endif
// 		y = NUM_LEDS_PER_PANEL_SIDE - 1 - y;// HACK ! y seems inverted. flip it
		y = NUM_LEDS_PER_PANEL_SIDE - y - 1;
	}
	else
	{
		// HAAAACK!
		uint8_t tmpx = y;
		y = x;
		x = tmpx;
	}

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
	// HACK. meh it works 
	if (pos == TOP)
	{
		switch(relativePos)
		{
			case SOUTH:
			{
				absX = x;
				absY = y;
				break;
			}
			case EAST:
			{
				absX = NUM_LEDS_PER_PANEL_SIDE - 1 - y;;
				absY = x;
				break;
			}
			case NORTH:
			{
				absX = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
				absY = NUM_LEDS_PER_PANEL_SIDE - 1 - y;
				break;
			}
			case WEST:
			default:
			{
				absX = y;
				absY = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
				break;
			}
		}
		// absY = NUM_LEDS_PER_PANEL_SIDE - 1 - absY; // HACK i really should refactor this stuff
	}
	else
	{
		switch(relativePos)
		{
			case EAST:
				{
					absX = NUM_LEDS_PER_PANEL_SIDE - 1 - y;
					absY = x;
					break;
			}
			case TOP:
			case NORTH:
				{
					absX = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
					absY = NUM_LEDS_PER_PANEL_SIDE - 1 - y;
					break;
			}
			case WEST:
				{
					absX = y;
					absY = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
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
	}
	return AddrLedDriver_GetPixelInPanel(pos, absX, absY);
}

Position_e AddrLedDriver_GetOppositePanel(Position_e pos)
{
	Position_e oppositePos;
	switch(pos)
	{
		case NORTH:
			oppositePos = SOUTH;
			break;
		case SOUTH:
			oppositePos = NORTH;
			break;
		case EAST:
			oppositePos = WEST;
			break;
		case WEST:
			oppositePos = EAST;
			break;
		case TOP:
			oppositePos = TOP; // TODO? Should this return, like, BOTTOM or something? whjich i dont have rn? 
			break;
	}
	return oppositePos;
}

AddrLedPanel_t* AddrLedDriver_GetPanelByLocation(Position_e pos)
{
	if (pos >= NUM_SIDES)
	{
		logprint("Bad pos %d for %s\n", pos, __FUNCTION__);
		return NULL;
	}
  return &ledPanels[pos];
}

void AddrLedDriver_PrintPixel(Pixel_t *p)
{
	logprint("Pixel panel:%s x:%d y:%d\n neighbors: \n", AddrLedDriver_GetPositionString(p->pos), p->x, p->y);
	for (uint8_t dir = 0; dir < NUM_DIRECTIONS; dir++)
	{
		Pixel_t *n = p->neighborPixels[dir];
		if (n == NULL)
		{
			logprint("%s \n", AddrLedDriver_GetDirectionString(dir));
			continue;
		}
		logprint("%s -> %s x:%d y:%d\n", AddrLedDriver_GetDirectionString(dir), AddrLedDriver_GetPositionString(n->pos), n->x, n->y);
	}
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
		logprint("Bad pos %d to %s\n", pos, __FUNCTION__);
		return NULL;
	}
	return positionStrings[pos];
}

char * AddrLedDriver_GetDirectionString(Direction_e dir)
{
	if (dir >= NUM_DIRECTIONS)
	{
		logprint("Bad dir %d to %s\n", dir, __FUNCTION__);
	}
	return directionStrings[dir];
}

AddrLedStrip_t* AddrLedDriver_GetStrip(void)
{
	return &ledStrip0;
}

int AddrLedDriver_TakeUsrCommand(int argc, char **argv)
{
	if (!addrLedDriverInitialized)
	{
		return 1;
	}
	ASSERT_ARGS(2);
	if (strcmp(argv[1], "clear") == 0)
	{
		AddrLedDriver_Clear();
	}
	else if (strcmp(argv[1], "relset") == 0)
	{
		// aled relset <pos> <relpos> <x> <y> <r> <g> <b>
		ASSERT_ARGS(9);
		Position_e pos = CharToPosEnum(argv[2][0]);
		Position_e relPos = CharToPosEnum(argv[3][0]);
		if (pos < NUM_SIDES && relPos < NUM_SIDES)
		{
			uint8_t x = atoi(argv[4]);
			uint8_t y = atoi(argv[5]);
			uint8_t r = atoi(argv[6]);
			uint8_t g = atoi(argv[7]);
			uint8_t b = atoi(argv[8]);
			logprint("Setting pixel %s relative to %s %d %d to %d %d %d\n", \
					AddrLedDriver_GetPositionString(pos), \
					AddrLedDriver_GetPositionString(relPos), \
					x, y, r, g, b);
			Pixel_t *relPixel = AddrLedDriver_GetPixelInPanelRelative(pos, relPos, x, y);
			AddrLedDriver_SetPixelRgb(relPixel, r, g, b);
		}
	}
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
			logprint("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
			return 1;
		}
		uint8_t x = atoi(argv[3]);
		uint8_t y = atoi(argv[4]);
		uint8_t r = atoi(argv[5]);
		uint8_t g = atoi(argv[6]);
		uint8_t b = atoi(argv[7]);
		// logprint("Setting pixel %s %d %d to %d %d %d\n", AddrLedDriver_GetPositionString(pos), x, y, r, g, b);
		AddrLedDriver_SetPixelRgbInPanel(pos, x, y, r, g, b);
	}
	else if (strcmp(argv[1], "nei") == 0)
	{
		// aled print <pos> <x> <y>
		ASSERT_ARGS(5);
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
			logprint("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
			return 1;
		}
		uint8_t x = atoi(argv[3]);
		uint8_t y = atoi(argv[4]);
		AddrLedDriver_Clear();
		Pixel_t *p = AddrLedDriver_GetPixelInPanel(pos, x, y);
		AddrLedDriver_SetPixelRgb(p, 10, 0, 0);
		for (uint8_t i = 0; i < NUM_DIRECTIONS; i++)
		{
			if (p->neighborPixels[i] == NULL)
			{
				continue;
			}
			AddrLedDriver_SetPixelRgb(p->neighborPixels[i], 10, 0, 10);
		}
	}
	else if (strcmp(argv[1], "print") == 0)
	{
		// aled print <pos> <x> <y>
		ASSERT_ARGS(5);
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
			logprint("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
			return 1;
		}
		uint8_t x = atoi(argv[3]);
		uint8_t y = atoi(argv[4]);
		AddrLedDriver_PrintPixel(AddrLedDriver_GetPixelInPanel(pos, x, y));
	}
	return 0;
}

bool AddrLedDriver_IsInitialized(void)
{
	return addrLedDriverInitialized;
}
