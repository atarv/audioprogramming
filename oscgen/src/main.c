#include "breakpoints.h"
#include "wave.h"
#include "macros.h"

#define NFRAMES 1024

enum
{
    ARG_PROGNAME,
    ARG_OUTFILE,
    ARG_DURATION,
    ARG_SAMPLE_RATE,
    ARG_NCHANNELS,
    ARG_AMPLITUDE,
    ARG_FREQUENCY,
    ARG_WAVEFORM,
    ARG_NOSCS,
    ARG_NARGS
};

enum
{
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_SAW_DOWN,
    WAVE_SAW_UP
};

int main(int argc, char const *argv[])
{
    int error = 0;
    PSF_PROPS outprops;

    printf("oscgen - generate tones with additive synthesis\n");

    // Handle commandline arguments
    if (argc < ARG_NARGS)
    {
        printf("Error: insufficient number of arguments\n");
        printf("Usage: oscgen outfile duration srate nchannels amp freq "
               "waveform noscs\nwaveform:\t0 - square\n\t\t\1 - "
               "triangle\n\t\t2 - saw (down)\n\t\t3 - saw (up)\n");
        return EXIT_FAILURE;
    }

    double duration = strtod(argv[ARG_DURATION], NULL);
    if (duration <= 0.0)
    {
        printf("Error: duration must be positive (was %lf)\n", duration);
        return EXIT_FAILURE;
    }

    outprops.srate = (int)strtol(argv[ARG_SAMPLE_RATE], NULL, 10);
    if (outprops.srate <= 0)
    {
        printf("Error: sample rate must be positive (was %d)\n",
               outprops.srate);
        return EXIT_FAILURE;
    }

    outprops.chans = (int)strtol(argv[ARG_NCHANNELS], NULL, 10);
    if (outprops.chans <= 0)
    {
        printf("Error: number of channels must be positive (was %d)\n",
               outprops.chans);
        return EXIT_FAILURE;
    }

    double amplitude = strtod(argv[ARG_AMPLITUDE], NULL);
    if (amplitude <= 0.0 || amplitude > 1.0)
    {
        printf("Error: amplitude must be between (0.0, 1.0] (was %lf\n",
               amplitude);
        return EXIT_FAILURE;
    }

    double frequency = strtod(argv[ARG_FREQUENCY], NULL);
    if (frequency <= 0)
    {
        printf("Error: frequency must be positive (was %lf)\n", frequency);
    }

    size_t oscillator_count = strtoul(argv[ARG_NOSCS], NULL, 10);
    if (oscillator_count == 0)
    {
        printf("Error: number of oscillators must be positive (was %lu)\n",
               oscillator_count);
    }

    outprops.samptype = PSF_SAMP_IEEE_FLOAT;
    outprops.chformat = STDWAVE;
    outprops.format = PSF_STDWAVE;
    int ofd =
        psf_sndCreate(argv[ARG_OUTFILE], &outprops, 0, 0, PSF_CREATE_RDWR);
    if (ofd < 0)
    {
        printf("Error: unable to create outfile %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    if (psf_init())
    {
        printf("Error: failed to initialize psf");
        error++;
        goto cleanup;
    }

    // Reserve memory
    OSCIL **oscillators = malloc(oscillator_count * sizeof(OSCIL));
    ON_MALLOC_ERROR(oscillators);

    double *osc_amps = malloc(oscillator_count * sizeof(double));
    ON_MALLOC_ERROR(osc_amps);

    double *osc_freqs = malloc(oscillator_count * sizeof(double));
    ON_MALLOC_ERROR(osc_freqs);
    // TODO: jatka...

cleanup:
    if (ofd)
        if (psf_sndClose(ofd))
            printf("Error: failed to close file %s\n", argv[ARG_OUTFILE]);
    if (osc_amps)
        free(osc_amps);
    if (osc_freqs)
        free(osc_freqs);
    return error;
}
