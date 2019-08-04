#include "wave.h"
#include <errno.h>
#include <stdio.h>

#define NFRAMES 1024

enum
{
    ARG_PROGNAME,
    ARG_OUTFILE,
    ARG_DURATION,
    ARG_SRATE,
    ARG_FREQUENCY,
    ARG_AMPLITUDE,
    ARG_NARGS
};

int main(int argc, char *argv[])
{
    printf("sinetest: test for sinewave oscillator\n");
    int error = 0; // positive if errors present
    PSF_PROPS outprops;
    OSCIL *osc = NULL;
    float *outframe = NULL;

    // Convert and validate arguments
    if (argc < ARG_NARGS)
    {
        printf("Error: insufficient arguments\nUsage: sinetest outfile "
               "duration sample_rate frequency amplitude");
        return EXIT_FAILURE;
    }

    if (psf_init())
    {
        printf("Error initializing portsf\n");
        return EXIT_FAILURE;
    }

    outprops.chans = 1;
    outprops.samptype = PSF_SAMP_IEEE_FLOAT;
    outprops.chformat = STDWAVE;
    outprops.format = PSF_STDWAVE;
    outprops.srate = (int)strtol(argv[ARG_SRATE], NULL, 10);
    if (outprops.srate == 0 || errno != 0)
    {
        printf("Error parsing sample rate argument (%d)\n", outprops.srate);
        error++;
        goto cleanup;
    }
    printf("%d %d %d %d %d\n", outprops.chans, outprops.chformat,
           outprops.format, outprops.samptype, outprops.srate);

    int ofd = // Output file descriptor
        psf_sndCreate(argv[ARG_OUTFILE], &outprops, 0, 0, PSF_CREATE_RDWR);
    if (ofd < 0)
    {
        printf("Error: unable to create outfile %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    double duration = strtod(argv[ARG_DURATION], NULL);
    if (duration <= 0.0 || errno != 0)
    {
        printf(
            "Error: duration must be positive (not including zero), was %lf\n",
            duration);
        error++;
        goto cleanup;
    }

    double amplitude = strtod(argv[ARG_AMPLITUDE], NULL);
    if (amplitude <= 0.0 || errno != 0)
    {
        printf("Error: amplitude must be over 0.0, was %lf\n", amplitude);
        error++;
        goto cleanup;
    }

    double frequency = strtod(argv[ARG_FREQUENCY], NULL);
    if (frequency <= 0.0 || errno != 0)
    {
        printf("Error: frequency must be over 0.0, was %lf\n", frequency);
        error++;
        goto cleanup;
    }

    osc = new_oscil(outprops.srate);

    size_t outframes =
        (size_t)(duration * outprops.srate + 0.5);  // Number of output frames
    size_t nbufs = outframes / NFRAMES;             // Number of buffers to fill
    size_t remainder = outframes - nbufs * NFRAMES; // Count of remaining frames
    if (remainder > 0)
        ++nbufs;

    outframe = malloc(NFRAMES * sizeof(float));

    // Processing
    unsigned int nframes = NFRAMES; // Number of frames in buffer
    for (size_t i = 0; i < nbufs; i++)
    {
        if (i == nbufs - 1) // make only remainder amount of samples on last run
            nframes = remainder;
        for (unsigned int j = 0; j < nframes; j++)
            outframe[j] = (float)(amplitude * sinetick(osc, frequency));

        int written_frames = psf_sndWriteFloatFrames(ofd, outframe, nframes);
        if (written_frames != (int)nframes)
        {
            printf("Error writing to outfile\n");
            error++;
            goto cleanup;
        }
    }

    printf("Successfully wrote %lu frames to %s\n", outframes,
           argv[ARG_OUTFILE]);

cleanup:
    if (ofd > 0)
        psf_sndClose(ofd);
    if (outframe)
        free(outframe);
    if (osc)
        free(osc);
    psf_finish();
    return error;
}