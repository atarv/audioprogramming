#pragma once
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct breakpoint
{
    double time, value;
} BREAKPOINT;

typedef struct min_max_pair
{
    double min_val, max_val;
} MINMAX_PAIR;

typedef struct breakpoint_stream
{
    BREAKPOINT *points;
    BREAKPOINT leftpoint, rightpoint;
    unsigned long npoints;
    double curpos;
    double incr;
    double width, height;
    unsigned long ileft, iright;
    int more_points;
} BRKSTREAM;

BREAKPOINT *get_breakpoints(FILE *fp, size_t *psize);
bool in_range(const BREAKPOINT *points, double min_val, double max_val,
              size_t size);
double val_at_brktime(const BREAKPOINT *points, size_t npoints, double time);
MINMAX_PAIR get_minmax(const BREAKPOINT *points, size_t size);
BRKSTREAM *bps_newstream(FILE *file, unsigned long srate, unsigned long *size);
void bps_freepoints(BRKSTREAM *stream);
double bps_tick(BRKSTREAM *stream);
