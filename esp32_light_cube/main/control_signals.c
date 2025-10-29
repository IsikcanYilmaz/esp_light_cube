#include "control_signals.h"
#include "esp_timer.h"
#include <math.h>
#include <stdint.h>

float CtrlSig_Sin(float freq, float phase)
{
	float now = (float) fmodf(esp_timer_get_time() / 1000000.0, 100);
	return sin(2.0 * M_PI * freq * now + phase);
}
