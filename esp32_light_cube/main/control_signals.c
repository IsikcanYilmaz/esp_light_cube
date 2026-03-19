#include "control_signals.h"
#include "esp_timer.h"
#include <math.h>
#include <stdint.h>

Oscillator_t CtrlSig_NewOscillator(OscillatorType_e type, double freqHz, double initialPhaseRadians)
{
  Oscillator_t osc;
  osc = (Oscillator_t) {.type = type, 
                        .freqHz = freqHz, 
                        .currPhase = fmod(initialPhaseRadians, (M_PI * 2.0)), 
                        .dt = M_TWO_PI * freqHz / DEFAULT_FPS, 
                        .magnitude = 0};
  CtrlSig_OscillatorUpdate(&osc);
  return osc;
}

void CtrlSig_OscillatorUpdate(Oscillator_t *osc)
{
  // Increment oscillator's phase and get output value

  // osc->currPhase = fmod(osc->currPhase + osc->dt, M_TWO_PI);
  osc->currPhase = osc->currPhase + osc->dt;

  switch(osc->type)
  {
    case SIN:
      {
        osc->magnitude = (sin(osc->currPhase) + 1.0) / 2.0;
        // return Math.sin(2 * PI * this.freq * this.t + this.initPhase);
        break;
      }
    default:
      {
        break;
        // Not implemented or wrong
      }
  }
}

void CtrlSig_OscillatorSetFreq(Oscillator_t *osc, double freqHz)
{
  osc->freqHz = freqHz;
  osc->dt = M_TWO_PI * freqHz / DEFAULT_FPS;
}

double CtrlSig_OscillatorGetMagnitude(Oscillator_t *osc)
{
  return osc->magnitude;
}

float CtrlSig_Sin(float freq, float phase)
{
	float now = (float) fmodf(esp_timer_get_time() / 1000000.0, 100);
	return sin(2.0 * M_PI * freq * now + phase);
}
