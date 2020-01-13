#include "gtable.h"
#include <stdbool.h>

// Create new gtable filled with zeroes
GTABLE *new_gtable(size_t length)
{
    if (length == 0)
        return NULL;
    GTABLE *gtable = malloc(sizeof(GTABLE));
    if (gtable == NULL)
        return NULL;
    gtable->table = calloc(length + 1, sizeof(double));
    if (gtable->table == NULL)
    {
        free(gtable);
        return NULL;
    }
    gtable->length = length;
    return gtable;
}

// Create new gtable filled with a sine wave
GTABLE *new_sine(size_t length)
{
    if (length == 0)
        return NULL;
    GTABLE *gtable = malloc(sizeof(GTABLE));
    if (gtable == NULL)
        return NULL;
    gtable->table = malloc((length + 1) * sizeof(double));
    if (gtable->table == NULL)
    {
        free(gtable);
        return NULL;
    }
    gtable->length = length;
    double step = TWOPI / length;
    // Fill table with one cycle of a sine wave
    for (size_t i = 0; i < length; ++i)
        gtable->table[i] = sin(step * i);
    gtable->table[length] = gtable->table[0]; // guard point
    return gtable;
}

// Destructor for GTABLE object
void gtable_free(GTABLE **gtable)
{
    if (gtable && *gtable && (*gtable)->table)
    {
        free((*gtable)->table);
        free(*gtable);
        *gtable = NULL;
    }
}

static void normalize_gtable(GTABLE *gtable)
{
    double value = 0.0, max_amp = 0.0;
    // Find maximum amplitude
    for (size_t i = 0; i < gtable->length; ++i)
    {
        value = fabs(gtable->table[i]);
        if (max_amp < value)
            max_amp = value;
    }
    max_amp = 1.0 / max_amp;
    // Normalize to max amplitude
    for (size_t i = 0; i < gtable->length; ++i)
        gtable->table[i] *= max_amp;
    gtable->table[gtable->length] = gtable->table[0];
}

// Create lookup table filled with triangle wave
GTABLE *new_triangle(size_t length, unsigned nharmonics)
{
    if (length == 0 || nharmonics == 0 || nharmonics >= length / 2)
        return NULL;
    GTABLE *gtable = new_gtable(length);
    // Generate triangle wave
    size_t harmonic = 1;
    const double step = TWOPI / length;
    for (unsigned i = 0; i < nharmonics; ++i)
    {
        double amplitude = 1.0 / (harmonic * harmonic);
        for (size_t j = 0; j < gtable->length; ++j)
            gtable->table[j] += amplitude * cos(j * step * harmonic);
        harmonic += 2; // triangle contains only odd harmonics
    }
    normalize_gtable(gtable);
    return gtable;
}

// Create lookup table filled with square wave
GTABLE *new_square(size_t length, unsigned nharmonics)
{
    if (length == 0 || nharmonics == 0 || nharmonics >= length / 2)
        return NULL;
    GTABLE *gtable = new_gtable(length);
    // Generate square wave
    size_t harmonic = 1;
    const double step = TWOPI / length;
    for (size_t i = 0; i < nharmonics; ++i)
    {
        double amplitude = 1.0 / harmonic;
        for (size_t j = 0; j < gtable->length; ++j)
            gtable->table[j] += amplitude * sin(j * step * harmonic);
        harmonic += 2; // square wave contains only odd harmonics
    }
    normalize_gtable(gtable);
    return gtable;
}

// Create lookup table filled with saw wave, direction defined with up parmeter
GTABLE *new_saw(size_t length, size_t nharmonics, SAW_DIRECTION direction)
{
    if (length == 0 || nharmonics == 0 || nharmonics >= length / 2)
        return NULL;
    GTABLE *gtable = new_gtable(length);
    // Generate square wave
    size_t harmonic = 1;
    const double step = TWOPI / length;
    double amplitude = 1.0;
    if (direction == SAW_UP)
        amplitude = -1.0;
    for (size_t i = 0; i < nharmonics; ++i)
    {
        double value = amplitude / harmonic;
        for (size_t j = 0; j < gtable->length; ++j)
            gtable->table[j] += value * sin(j * step * harmonic);
        ++harmonic;
    }
    normalize_gtable(gtable);
    return gtable;
}

// Constructor for OSCILT
OSCILT *new_oscilt(double srate, const GTABLE *gtable, double phase)
{
    // Check gtable validity
    if (gtable == NULL || gtable->table == NULL || gtable->length == 0)
        return NULL;
    OSCILT *p_osc = malloc(sizeof(OSCILT));
    if (p_osc == NULL)
        return NULL;

    // Initialize osc
    p_osc->osc.current_freq = 0.0;
    p_osc->osc.current_phase = gtable->length * phase;
    p_osc->osc.phase_increment = 0.0;
    p_osc->gtable = gtable;
    p_osc->dtablen = (double)gtable->length;
    p_osc->size_over_srate = p_osc->dtablen / (double)srate;
    return p_osc;
}

// Truncating tick function for table lookup oscillator
double tabtick_trunc(OSCILT *p_osc, double freq)
{
    int index = (int)p_osc->osc.current_phase;
    double dtablen = p_osc->dtablen, current_phase = p_osc->osc.current_phase;
    const double *table = p_osc->gtable->table;
    // Update oscillator's internal frequency to match
    if (p_osc->osc.current_freq != freq)
    {
        p_osc->osc.current_freq = freq;
        p_osc->osc.phase_increment =
            p_osc->size_over_srate * p_osc->osc.current_freq;
    }
    // Advance oscillator phase
    current_phase += p_osc->osc.phase_increment;
    while (current_phase >= dtablen)
        current_phase -= dtablen;
    while (current_phase < 0.0)
        current_phase += dtablen;
    p_osc->osc.current_phase = current_phase;
    return table[index];
}

// Interpolating tick function for table lookup oscillator
double tabtick_interp(OSCILT *p_osc, double freq)
{
    int base_index = (int)p_osc->osc.current_phase;
    size_t next_index = base_index + 1;
    double dtablen = p_osc->dtablen, current_phase = p_osc->osc.current_phase;
    const double *table = p_osc->gtable->table;
    // Update oscillator's internal frequency to match
    if (p_osc->osc.current_freq != freq)
    {
        p_osc->osc.current_freq = freq;
        p_osc->osc.phase_increment =
            p_osc->size_over_srate * p_osc->osc.current_freq;
    }

    double fraction = current_phase - base_index;
    double value = table[base_index];
    double slope = table[next_index] - value;
    value += (fraction * slope);

    // Advance oscillator phase
    current_phase += p_osc->osc.phase_increment;
    while (current_phase >= dtablen)
        current_phase -= dtablen;
    while (current_phase < 0.0)
        current_phase += dtablen;
    p_osc->osc.current_phase = current_phase;
    return value;
}