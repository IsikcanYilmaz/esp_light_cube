#include "visual_util.h"
#include "addr_led_driver.h"
#include "colorspace_interface.h"
#include <stdio.h>
#include <math.h>

// Dump all misc visual util functions here

// This goes thru ALL pixels and applies the function passed to it $fn. fn takes a ptr to a Pixel_t
void Visual_ApplyFnToAllPixels(void (*fn)(Pixel_t *p, void *args))
{
	for (uint8_t pos = 0; pos < NUM_SIDES; pos++)
	{
		for (uint8_t row = 0; row < NUM_LEDS_PER_PANEL_SIDE; row++)
		{
			for (uint8_t col = 0; col < NUM_LEDS_PER_PANEL_SIDE; col++)
			{
				Pixel_t *p = AddrLedDriver_GetPixelInPanel(pos, col, row);
				// fn(p); // TODO
			}
		}
	}
}

// This goes thru ALL pixels and INCREMENTS/DECREMENTS their hsv values by what's passed. 
// Meaning, it doesnt set them but INCREMENT/DECREMENT their current values
void Visual_IncrementAllByHSV(double h, double s, double v)
{
	for (uint8_t pos = 0; pos < NUM_SIDES; pos++)
	{
		for (uint8_t row = 0; row < NUM_LEDS_PER_PANEL_SIDE; row++)
		{
			for (uint8_t col = 0; col < NUM_LEDS_PER_PANEL_SIDE; col++)
			{
				Pixel_t *p = AddrLedDriver_GetPixelInPanel(pos, col, row);
				Color_t c = Color_CreateFromRgb(p->red, p->green, p->blue);
				
				c.value += v;
				if (c.value >= PER_UPPER_LIMIT)
				{
					c.value = PER_UPPER_LIMIT;
				}
				if (c.value < PER_LOWER_LIMIT)
				{
					c.value = PER_LOWER_LIMIT;
				}
				
				c.saturation += s;
				if (c.saturation >= PER_UPPER_LIMIT)
				{
					c.saturation = PER_UPPER_LIMIT;
				}
				if (c.saturation < PER_LOWER_LIMIT)
				{
					c.saturation = PER_LOWER_LIMIT;
				}

				c.hue = remainder(h + c.hue, HUE_UPPER_LIMIT);
				if (c.hue < 0)
				{
					c.hue = HUE_UPPER_LIMIT + c.hue;
				}
				
				Color_t dc = Color_CreateFromHsv(c.hue, c.saturation, c.value);
				AddrLedDriver_SetPixelRgb(p, dc.red, dc.green, dc.blue);
			}
		}
	}
}

bool Visual_IsDark(Pixel_t *p)
{
	return (p->red == 0 && p->green == 0 && p->blue == 0);
}

bool Visual_IsAllDark(void)
{
	AddrLedStrip_t *s = AddrLedDriver_GetStrip();
	for (int i = 0; i < s->numLeds; i++)
	{
		Pixel_t *p = &(s->pixels[i]);
		bool dark = Visual_IsDark(p);
		if (!dark)
		{
			return false;
		}
	}
	return true;
}
