#include "wave.h"
#include <stdio.h> // DEBUG:

/*
 * Initialize an oscillator object
 */
void oscil_init(OSCIL *osc, size_t sample_rate)
{
    osc->two_pi_over_srate = TWOPI / (double)sample_rate;
    osc->current_phase = 0.0;
    osc->current_freq = 0.0;
    osc->phase_increment = 0.0;
}

/*
 * Create and initialize new oscillator object.
 * Must be freed by user!
 */
OSCIL *new_oscil(size_t sample_rate)
{
    OSCIL *p_osc = malloc(sizeof(OSCIL));
    if (p_osc == NULL)
        return NULL;
    oscil_init(p_osc, sample_rate);
    return p_osc;
}

/**
 * Tick an oscillator object to generate value of next sample
 */
double sinetick(OSCIL *osc, double freq)
{
    double val = sin(osc->current_phase);
    if (osc->current_freq != freq)
    {
        osc->current_freq = freq;
        osc->phase_increment = osc->two_pi_over_srate * freq;
    }
    osc->current_phase += osc->phase_increment;
    if (osc->current_phase >= TWOPI)
        osc->current_phase -= TWOPI;
    if (osc->current_phase < 0.0)
        osc->current_phase += TWOPI;
    return val;
}