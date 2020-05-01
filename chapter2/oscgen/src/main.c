#include "breakpoints.h"
#include "macros.h"
#include "wave.h"
#include <time.h>

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
    WAVE_SAW_UP,
    WAVE_NWAVEFORMS
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
        printf("Usage: oscgen outfile duration srate nchannels amplitude freq "
               "waveform noscs\nwaveform:\t0 - square\n\t\t1 - "
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

    int waveform = strtol(argv[ARG_WAVEFORM], NULL, 10);
    if (waveform < 0 || waveform >= WAVE_NWAVEFORMS)
    {
        printf("Error: invalid oscillator type: %d\n", waveform);
        return EXIT_FAILURE;
    }

    size_t oscillator_count = strtoul(argv[ARG_NOSCS], NULL, 10);
    if (oscillator_count == 0)
    {
        printf("Error: number of oscillators must be positive (was %zu)\n",
               oscillator_count);
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
    int ofd =
        psf_sndCreate(argv[ARG_OUTFILE], &outprops, 0, 0, PSF_CREATE_RDWR);
    if (ofd < 0)
    {
        printf("Error: unable to create outfile %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    // Reserve memory for oscillators
    OSCIL **oscillators = malloc(oscillator_count * sizeof(OSCIL));
    ON_MALLOC_ERROR(oscillators);

    double *osc_amps = malloc(oscillator_count * sizeof(double));
    ON_MALLOC_ERROR(osc_amps);

    double *osc_freqs = malloc(oscillator_count * sizeof(double));
    ON_MALLOC_ERROR(osc_freqs);

    // Initialize oscillators according to waveform
    double amp_factor = 1.0, freq_factor = 1.0, amp_adjust = 0.0, phase = 0.0;
    switch (waveform)
    {
    case WAVE_SQUARE:
        for (size_t i = 0; i < oscillator_count; i++)
        {
            amp_factor = 1.0 / freq_factor;
            osc_amps[i] = amp_factor;
            osc_freqs[i] = freq_factor;
            freq_factor += 2.0; // Odd harmonics
            amp_adjust += amp_factor;
        }
        break;
    case WAVE_TRIANGLE:
        for (size_t i = 0; i < oscillator_count; i++)
        {
            amp_factor = 1.0 / (freq_factor * freq_factor);
            osc_amps[i] = amp_factor;
            osc_freqs[i] = freq_factor;
            freq_factor += 2.0; // Odd harmonics
            amp_adjust += amp_factor;
        }
        phase = 0.25; // Set cos phase for true triangle shape
        break;
    case WAVE_SAW_UP:
    case WAVE_SAW_DOWN:
        for (size_t i = 0; i < oscillator_count; i++)
        {
            amp_factor = 1.0 / freq_factor;
            osc_amps[i] = amp_factor;
            osc_freqs[i] = freq_factor;
            freq_factor += 1.0; // Even harmonics
            amp_adjust += amp_factor;
        }
        if (waveform == WAVE_SAW_UP)
            amp_adjust = -amp_adjust; // Inverts waveform
        break;
    }

    for (size_t i = 0; i < oscillator_count; i++)
    {
        oscillators[i] = new_oscilp(outprops.srate, phase);
        ON_MALLOC_ERROR(oscillators[i]);
    }

    // Rescale amplitudes to add up to 1.0
    for (size_t i = 0; i < oscillator_count; i++)
        osc_amps[i] /= amp_adjust;

    float *outframe =
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
            double val = 0.0; // Sample value
            for (size_t j = 0; j < oscillator_count; j++)
                val += osc_amps[j] *
                       sinetick(oscillators[j], frequency * osc_freqs[j]);
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
    printf("Successfully wrote %zu frames to %s in %.3f seconds\n", outframes,
           argv[ARG_OUTFILE], (endtime - starttime) / (double)CLOCKS_PER_SEC);

cleanup:
    if (ofd)
        if (psf_sndClose(ofd))
            printf("Error: failed to close file %s\n", argv[ARG_OUTFILE]);
    if (osc_amps)
        free(osc_amps);
    if (osc_freqs)
        free(osc_freqs);
    if (oscillators)
    {
        for (size_t i = 0; i < oscillator_count; i++)
            free(oscillators[i]);
        free(oscillators);
    }
    if (outframe)
        free(outframe);
    psf_finish();
    return error;
}
