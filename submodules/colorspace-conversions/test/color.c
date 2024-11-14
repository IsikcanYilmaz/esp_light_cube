#include "color.h"
#include <stdio.h>
#include <math.h>

// we straight up use some dude's color conversion code. there are wrappers on that code in color.c/h
Color_t Color_CreateFromRgb(uint8_t r, uint8_t g, uint8_t b)
{
	Color_t c = {.red = r, .green = g, .blue = b};
	HsvColor hsv = Hsv_CreateFromRgbF((double) r/RGBI_LOWER_LIMIT, (double) g/RGBI_LOWER_LIMIT, (double) b/RGBI_LOWER_LIMIT);
	c.hue = hsv.H;
	c.saturation = hsv.S;
	c.value = hsv.V;
	return c;
}

Color_t Color_CreateFromHsv(double h, double s, double v)
{
	Color_t c = {.hue = remainder(h,HUE_UPPER_LIMIT), .saturation = s, .value = v};
	RgbFColor rgb = RgbF_CreateFromHsv(h, s, v);
	c.red = rgb.R;
	c.green = rgb.G;
	c.blue = rgb.B;
	// printf("%s %lf %lf %lf > %lf %lf %lf (%d %d %d)\n", __FUNCTION__, h, s ,v, rgb.R, rgb.G, rgb.B, c.red, c.green, c.blue);
	return c;
}

void Color_PrintColor(Color_t c)
{
	printf("r:%d g:%d b:%d | h:%f s:%f v:%f\n", c.red, c.green, c.blue, c.hue, c.saturation, c.value);
}
