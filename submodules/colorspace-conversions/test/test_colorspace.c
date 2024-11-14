#include "../colorspace.h"
#include "color.h"
#include <stdio.h>

int main()
{
	RgbFColor rgbf = RgbF_CreateFromHsv(160.0, 0.64, 0.73);
	printf("r %f g %f b %f \n", rgbf.R, rgbf.G, rgbf.B);
	RgbIColor rgbi = RgbI_CreateFromRealForm(rgbf.R, rgbf.G, rgbf.B);
	printf("r %d g %d b %d \n", rgbi.R, rgbi.G, rgbi.B);
	return 0;
}
