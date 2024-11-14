#include <stdint.h>
#include "../colorspace.h"

typedef struct Color_t_
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	double hue;
	double saturation;
	double value;
} Color_t;

Color_t Color_CreateFromRgb(uint8_t r, uint8_t g, uint8_t b);
Color_t Color_CreateFromHsv(double h, double s, double v);
void Color_PrintColor(Color_t c);
