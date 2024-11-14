#include "animation_manager.h"
#include <stdbool.h>

void Visual_IncrementByHSV(double h, double s, double v);
void Visual_IncrementAllByHSV(double h, double s, double v);
// void Visual_ApplyFnToAllPixels(void (*fn)(Pixel_t *p, void *args));
// bool Visual_IsDark(Pixel_t *p);
bool Visual_IsAllDark(void);
