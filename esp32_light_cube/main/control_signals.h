#include <stdint.h>

#ifndef M_PI // TODO where to put this
    #define M_PI 3.14159265358979323846
#endif

typedef enum
{
	SIN,
	TRI,
	SQUARE,
	OSCILLATOR_TYPE_MAX
} OscillatorType_e;

typedef struct
{
	OscillatorType_e type;
	double freqHz;
	double initialPhase;
} Oscillator_t;

typedef struct
{
	// TODO
} Impulse_t;


// Sinusoidal Simple Harmonic Oscillator
// y(t)=Asin(ωt+ϕ)
// Where
// A is Amplitude
// ω is Angular frequency in radians per second. ω = 2 * pi * f
// ϕ is Phase
Oscillator_t CtrlSig_NewOscillator(OscillatorType_e type, double freqHz, double initialPhase);

float CtrlSig_Sin(float freq, float phase);
