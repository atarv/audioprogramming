#include "breakpoints.h"
#include "gtable.h"
#include "macros.h"
#include "wave.h"
#include <time.h>

#define NFRAMES 1024u
#define LOOKUP_TABLE_LENGTH 1024lu  // May be changed to vary quality of output
#define TRUNCATING_TICK false       // Use interpolating tick if false

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
    ARG_NHARMONICS,
    ARG_NARGS
};

enum
{
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_SAW_DOWN,
    WAVE_SAW_UP,
    WAVE_SINE,
    WAVE_NWAVEFORMS
};

int main(int argc, char const *argv[])
{
    int error = 0;
    int ofd = -1;
    float *outframe = NULL;
    OSCILT *osc = NULL;
    PSF_PROPS outprops;

    printf("tabgen - generate tones with table lookup oscillator\n");

    // Handle commandline arguments
    if (argc < ARG_NARGS)
    {
        printf("Error: insufficient number of arguments\n");
        printf("Usage: tabgen outfile duration srate nchannels amplitude freq "
               "waveform nharmonics\nAvailable waveforms:\n"
               "       0 - square\n"
               "       1 - triangle\n"
               "       2 - saw (down)\n"
               "       3 - saw (up)\n"
               "       4 - sine\n");
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

    int waveform = strtol(argv[ARG_WAVEFORM], NULL, 10);
    if (waveform < 0 || waveform >= WAVE_NWAVEFORMS)
    {
        printf("Error: invalid oscillator type: %d\n", waveform);
        return EXIT_FAILURE;
    }

    unsigned nharmonics = strtol(argv[ARG_NHARMONICS], NULL, 10);
    if (nharmonics < 1)
    {
        printf("Error: nharmonics must be at least 1, was %u", nharmonics);
        return EXIT_FAILURE;
    }

    if (psf_init())
    {
        printf("Error: failed to initialize psf");
        error++;
        goto cleanup;
    }

    outprops.samptype = PSF_SAMP_IEEE_FLOAT;
    outprops.chformat = STDWAVE;
    outprops.format = PSF_STDWAVE;
    ofd = psf_sndCreate(argv[ARG_OUTFILE], &outprops, 0, 0, PSF_CREATE_RDWR);
    if (ofd < 0)
    {
        printf("Error: unable to create outfile %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    // Initialize lookup table according to waveform
    GTABLE *gtable = NULL;
    switch (waveform)
    {
    case WAVE_SQUARE:
        gtable = new_square(LOOKUP_TABLE_LENGTH, nharmonics);
        break;
    case WAVE_TRIANGLE:
        gtable = new_triangle(LOOKUP_TABLE_LENGTH, nharmonics);
        break;
    case WAVE_SAW_UP:
        gtable = new_saw(LOOKUP_TABLE_LENGTH, nharmonics, SAW_UP);
        break;
    case WAVE_SAW_DOWN:
        gtable = new_saw(LOOKUP_TABLE_LENGTH, nharmonics, SAW_DOWN);
        break;
    case WAVE_SINE:
        gtable = new_sine(LOOKUP_TABLE_LENGTH);
        break;
    }

    osc = new_oscilt(outprops.srate, gtable, 0.0);
    if (osc == NULL)
    {
        printf(
            "Error: failed to initialize oscillator. Make sure that "
            "nharmonics (%u) is less than half of lookup table length (%lu)\n",
            nharmonics, LOOKUP_TABLE_LENGTH);
        return EXIT_FAILURE;
    }

    outframe =
        malloc((unsigned long)outprops.chans * NFRAMES * sizeof(float));
    ON_MALLOC_ERROR(outframe);

    size_t outframes =
        (size_t)(duration * outprops.srate + 0.5);  // Number of output frames
    size_t nbufs = outframes / (NFRAMES);           // Number of buffers to fill
    size_t remainder = outframes - nbufs * NFRAMES; // Count of remaining frames
    if (remainder > 0)
        ++nbufs;

    // Generate sound
    unsigned nframes =
        (unsigned)outprops.chans * NFRAMES; // Number of frames in buffer
    time_t starttime = clock();
    for (size_t i = 0; i < nbufs; i++)
    {
        if (i == nbufs - 1) // Make only remainder amount of samples on last run
            nframes = remainder;

        for (unsigned k = 0; k < nframes; k += (unsigned)outprops.chans)
        {
            double val = 0.0;
            if (TRUNCATING_TICK)
                val = tabtick_trunc(osc, frequency);
            else
                val = tabtick_interp(osc, frequency);
            for (unsigned chan = 0; chan < (unsigned)outprops.chans; chan++)
            {
                outframe[k + chan] = (float)(amplitude * val);
            }
        }

        int written_frames = psf_sndWriteFloatFrames(
            ofd, outframe, nframes / (unsigned)outprops.chans);
        if (written_frames != (int)nframes / outprops.chans)
        {
            printf("Error writing to outfile\n");
            error++;
            goto cleanup;
        }
    }
    time_t endtime = clock();
    printf("Successfully wrote %lu frames to %s in %.3f seconds\n", outframes,
           argv[ARG_OUTFILE], (endtime - starttime) / (double)CLOCKS_PER_SEC);

cleanup:
    if (ofd > 0)
        if (psf_sndClose(ofd))
            printf("Error: failed to close file %s\n", argv[ARG_OUTFILE]);
    if (osc)
        free(osc);
    if (gtable)
        gtable_free(&gtable);
    if (outframe)
        free(outframe);
    psf_finish();
    return error;
}
