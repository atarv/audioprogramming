#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct breakpoint
{
    double time, value;
} BREAKPOINT;

typedef struct min_max_pair
{
    double min_val, max_val;
} MINMAX_PAIR;

BREAKPOINT *get_breakpoints(FILE *fp, size_t *psize);
bool in_range(const BREAKPOINT *points, double min_val, double max_val,
              size_t size);
double val_at_brktime(const BREAKPOINT *points, size_t npoints, double time);
MINMAX_PAIR get_minmax(const BREAKPOINT *points, size_t size);
