#include <stdint.h>
#include "colorspace.h"

typedef struct Color_t_
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	double hue;
	double saturation;
	double value;
} Color_t;


// rgb values from 0 to 255
// hsv values h 0.0 - 360.0, s 0.0 - 1.0, v 0.0 - 1.0
Color_t Color_CreateFromRgb(uint8_t r, uint8_t g, uint8_t b);
Color_t Color_CreateFromHsv(double h, double s, double v);
void Color_PrintColor(Color_t c);
Color_t Color_GenerateRandomColor(llH, ulH, llS, ulS, llV, ulV);
