#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct breakpoint
{
    double time, value;
} BREAKPOINT;

BREAKPOINT *get_breakpoints(FILE *fp, size_t *psize);
bool in_range(const BREAKPOINT *points, double min_val, double max_val,
              size_t size);
double val_at_brktime(const BREAKPOINT *points, size_t npoints, double time);
