#include <stdint.h>

#ifndef M_PI // TODO where to put this
    #define M_PI 3.14159265358979323846
    #define M_TWO_PI (2 * 3.14159265358979323846)
#endif

#define DEFAULT_FPS (60.0) // For calculating phase increments // TODO find better way of doing this
#define DEFAULT_DT  (M_TWO_PI / DEFAULT_FPS)

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
	float freqHz;
  float currPhase; 
  float magnitude; // Current output value
  float dt; // phase increase per update call. in radians
} Oscillator_t;

typedef struct 
{
  float magnitude;
  float dt;
} Impulse_t;


// Sinusoidal Simple Harmonic Oscillator
// y(t)=Asin(ωt+ϕ)
// Where
// A is Amplitude
// ω is Angular frequency in radians per second. ω = 2 * pi * f
// ϕ is Phase
Oscillator_t Oscillator_Create(OscillatorType_e type, double freqHz, double initialPhaseRadians);

void Oscillator_Update(Oscillator_t *osc);
double Oscillator_GetMagnitude(Oscillator_t *osc);
void Oscillator_SetFreq(Oscillator_t *osc, double freqHz);

float CtrlSig_Sin(float freq, float phase);
