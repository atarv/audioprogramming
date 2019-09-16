#pragma once
#include "portsf.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI (3.1415926535897932)
#endif
#define TWOPI (2.0 * M_PI)

typedef struct oscil_t
{
    double two_pi_over_srate, current_freq, current_phase, phase_increment;
} OSCIL;
typedef double (*tickfunc)(OSCIL *osc, double freq);
typedef double (*pwmtickfunc)(OSCIL *osc, double freq, double pwmod);

void oscil_init(OSCIL *osc, size_t sample_rate);
OSCIL *new_oscil(size_t sample_rate);
double sinetick(OSCIL *osc, double freq);
double sqrtick(OSCIL *osc, double freq);
double pwmtick(OSCIL *osc, double freq, double pwmod);
double sawdtick(OSCIL *osc, double freq);
double sawutick(OSCIL *osc, double freq);
double tritick(OSCIL *osc, double freq);