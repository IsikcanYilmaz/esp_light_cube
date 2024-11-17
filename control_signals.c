#include "control_signals.h"
#include "ztimer.h"
#include "xtimer.h"
#include <math.h>
#include <stdint.h>

float CtrlSig_Sin(float freq, float phase)
{
	float now = (float) fmodf(ztimer_now(ZTIMER_USEC) / 1000000.0, 100);
	return sin(2.0 * M_PI * freq * now + phase);
}
