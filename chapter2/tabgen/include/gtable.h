#pragma once
#include "wave.h"

typedef struct t_gtable
{
    double *table; // pointer to array containing the waveform
    size_t length; // excluding guard point
} GTABLE;

/* Table lookup oscillator */
typedef struct t_tab_oscil
{
    OSCIL osc;
    const GTABLE *gtable;
    double dtablen;
    double size_over_srate;
} OSCILT;

typedef double (*oscilt_tickfunc)(OSCILT *osc, double freq);

typedef enum saw_direction
{
    SAW_DOWN = 0,
    SAW_UP
} SAW_DIRECTION;

GTABLE *new_gtable(size_t length);
GTABLE *new_sine(size_t length);
GTABLE *new_triangle(size_t length, unsigned nharmonics);
GTABLE *new_square(size_t length, unsigned nharmonics);
GTABLE *new_saw(size_t length, size_t nharmonics, SAW_DIRECTION direction);
void gtable_free(GTABLE **gtable);
OSCILT *new_oscilt(double srate, const GTABLE *gtable, double phase);
double tabtick_trunc(OSCILT *p_osc, double freq);
double tabtick_interp(OSCILT *p_osc, double freq);