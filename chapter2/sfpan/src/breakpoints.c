#include "breakpoints.h"

/*
 * Read breakpoint values from a file
 * fp - pointer to file conatining breakpoints
 * psize [out] - size of breakpoint array
 */
BREAKPOINT *get_breakpoints(FILE *fp, size_t *psize)
{
    long npoints = 0; // Number of breakpoints
    long size = 64;   // Initial size of breakpoint array
    double last_time = 0.0;
    BREAKPOINT *points = NULL;
    char line[80];

    if (fp == NULL)
        return NULL;

    points = malloc(size * sizeof(BREAKPOINT));
    if (points == NULL)
        return NULL;

    while (fgets(line, 80, fp))
    {
        // Get values of line in format: time value
        int got = sscanf(line, "%lf%lf", &points[npoints].time,
                     &points[npoints].value);
        if (got < 0) // line empty
            continue;
        else if (got == 0)
        {
            printf("Line %ld has non-numeric data\n", npoints + 1);
            break;
        }
        else if (got == 1)
        {
            printf("Line %ld has an incomplete breakpoint\n", npoints + 1);
            break;
        }

        // Breakpoints must be in increasing order by time
        if (points[npoints].time < last_time)
        {
            printf("Breakpoint at line %ld not increasing in time\n",
                   npoints + 1);
            break;
        }
        last_time = points[npoints].time;

        if (++npoints == size) // Reallocate more memory if size hit
        {
            BREAKPOINT *tmp;
            size += npoints;
            tmp = realloc(points, size * sizeof(BREAKPOINT));
            if (tmp == NULL) // Not enough memory
            {
                npoints = 0;
                free(points);
                points = NULL;
                break;
            }
            points = tmp;
        }
    }
    if (npoints)
        *psize = npoints;
    return points;
}

/*
 * Checks that all breakpoints values are between min_val and max_val.
 * Returns true, if every point is in range, false otherwise.
 */
bool in_range(const BREAKPOINT *points, double min_val, double max_val,
              size_t size)
{
    bool range_ok = true;
    for (size_t i = 0; i < size; i++)
    {
        double value = points[i].value;
        if (value < min_val || value > max_val)
        {
            range_ok = false;
            break;
        }
    }
    return range_ok;
}

/*
 * Calculate value at specified time (between or at breakpoints).
 * Returns value at specified time
 */
double val_at_brktime(const BREAKPOINT *points, size_t npoints, double time)
{
    BREAKPOINT left, right;
    static size_t i = 1;
    for (; i < npoints; i++) // Find breakpoints that surround time
    {
        if (time <= points[i].time)
            break;
    }

    if (i == npoints) // Retain value if after last breakpoint
    {
        return points[i - 1].value;
    }

    left = points[i - 1];
    right = points[i];
    const double width = right.time - left.time;
    if (width == 0.0) // if breakpoints at same time
        return right.value;

    // Get value from span using linear interpolation
    const double fraction = (time - left.time) / width;
    return left.value + ((right.value - left.value) * fraction);
}

/*
 * Get minimum and maximum value of breakpoints
 */
MINMAX_PAIR get_minmax(const BREAKPOINT *points, size_t size)
{
    if (!points)
        return (MINMAX_PAIR){NAN, NAN};
    double min = points[0].value;
    double max = min;
    for (size_t i = 1; i < size; i++)
    {
        double value = points[i].value;
        min = value < min ? value : min;
        max = value > max ? value : max;
    }
    return (MINMAX_PAIR){min, max};
}
