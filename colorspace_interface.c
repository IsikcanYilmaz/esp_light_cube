#include "colorspace_interface.h"
#include "logger.h"
#include <stdio.h>
#include <math.h>

// rgb values from 0 to 255
// hsv values h 0.0 - 360.0, s 0.0 - 1.0, v 0.0 - 1.0

// i straight up use some dude's color conversion code. there are wrappers on that code in color.c/h
// said code is here: https://github.com/dystopiancode/colorspace-conversions thanks dystopiancode.
Color_t Color_CreateFromRgb(uint8_t r, uint8_t g, uint8_t b)
{
	Color_t c = {.red = r, .green = g, .blue = b};
	HsvColor hsv = Hsv_CreateFromRgbF((double) r/255, (double) g/255, (double) b/255); // TODO #define these
	c.hue = hsv.H;
	c.saturation = hsv.S;
	c.value = hsv.V;
	return c;
}

Color_t Color_CreateFromHsv(double h, double s, double v)
{
	if (h < 0)
	{
		h = HUE_UPPER_LIMIT + h;
	}
	if (h > HUE_UPPER_LIMIT)
	{
		h = h - HUE_UPPER_LIMIT;
	}
	if (s < 0)
	{
		s = 0;
	}
	if (v < 0)
	{
		v = 0;
	}
	Color_t c = {.hue = fmod(h,HUE_UPPER_LIMIT), .saturation = fmod(s,PER_UPPER_LIMIT), .value = fmod(v,PER_UPPER_LIMIT)};
	RgbFColor rgb = RgbF_CreateFromHsv(h, s, v);
	RgbIColor rgbi = RgbI_CreateFromRealForm(rgb.R, rgb.G, rgb.B);
	c.red = rgbi.R;
	c.green = rgbi.G;
	c.blue = rgbi.B;
	return c;
}

void Color_PrintColor(Color_t c)
{
	logprint("r:%d g:%d b:%d | h:%f s:%f v:%f\n", c.red, c.green, c.blue, c.hue, c.saturation, c.value);
}

Color_t Color_GenerateRandomColor(llH, ulH, llS, ulS, llV, ulV)
{
	double h = fmod(rand(), (ulH - llH + 1)) + llH;
	double s = (double) llS + fmod((rand() % 1000) / 100, (ulS - llS + 0.01));
	double v = (double) llV + fmod((rand() % 1000) / 100, (ulV - llV + 0.01));
	// logprint("%f %f %f\n", h, s, v);
	Color_t c = Color_CreateFromHsv(h, s, v);
	// Color_PrintColor(c);
	return c;
}

