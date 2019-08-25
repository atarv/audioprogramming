#include "breakpoints.h"
#include "wave.h"
#include <errno.h>
#include <stdio.h>

#define NFRAMES 1024
#define ON_FOPEN_ERROR(file, filename)                                         \
    {                                                                          \
        if (freq_file == NULL)                                                 \
        {                                                                      \
            printf("Error: unable to read %s\n", argv[ARG_FREQ_BRKFILE]);      \
            error++;                                                           \
            goto cleanup;                                                      \
        }                                                                      \
    }

enum
{
    ARG_PROGNAME,
    ARG_OUTFILE,
    ARG_WAVEFORM,
    ARG_DURATION,
    ARG_SRATE,
    ARG_CHANNELS,
    ARG_FREQ_BRKFILE,
    ARG_AMP_BRKFILE,
    ARG_PWMOD_BRKFILE,
    ARG_NARGS
};

enum
{
    WAVE_SINE,
    WAVE_TRIANGLE,
    WAVE_SAW_UP,
    WAVE_SAW_DOWN,
    WAVE_SQUARE,
    WAVE_PWM_SQUARE,
    WAVE_NFORMS
};

tickfunc tick_functions[] = {sinetick, tritick, sawutick, sawdtick, sqrtick};

int main(int argc, char *argv[])
{
    printf("siggen: generate simple tones\n");
    int error = 0; // positive if errors present
    PSF_PROPS outprops;
    OSCIL *osc = NULL;
    float *outframe = NULL;

    // Convert and validate arguments
    if (argc < ARG_NARGS - 1)
    {
        printf(
            "Error: insufficient arguments\nUsage: siggen outfile waveform"
            " duration sample_rate freq_brkfile amp_brkfile "
            "[pwmod_brkfile]\nWhere waveform is one of:\n0 - sine\n1 - "
            "triangle\n2 - sawtooth (up)\n 3 - sawtooth (down)\n4 - "
            "square\n5 - square w/PWM\nIf 5 is chosen, pwmod must be given\n");
        return EXIT_FAILURE;
    }

    if (psf_init())
    {
        printf("Error initializing portsf\n");
        return EXIT_FAILURE;
    }

    int waveform_type = (int)strtol(argv[ARG_WAVEFORM], NULL, 10);
    if (waveform_type < WAVE_SINE || waveform_type >= WAVE_NFORMS)
    {
        printf(
            "Error: invalid waveform type %d. Waveform types are:\n0 - sine\n1 "
            "- triangle\n2 - sawtooth (up)\n 3 - sawtooth (down)\n4 - "
            "square\n5 - square w/PWM\n",
            waveform_type);
        return EXIT_FAILURE;
    }

    tickfunc tick = NULL;
    // PWM square wave has a different function signature
    if (waveform_type != WAVE_PWM_SQUARE)
        tick = tick_functions[waveform_type];

    outprops.chans = (int)strtol(argv[ARG_CHANNELS], NULL, 10);
    if (outprops.chans < 1)
    {
        printf("Error: channel count must be positive, was %d\n",
               outprops.chans);
        error++;
        goto cleanup;
    }
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

    FILE *freq_file = fopen(argv[ARG_FREQ_BRKFILE], "r");
    ON_FOPEN_ERROR(freq_file, argv[ARG_FREQ_BRKFILE]);

    FILE *amp_file = fopen(argv[ARG_AMP_BRKFILE], "r");
    ON_FOPEN_ERROR(amp_file, argv[ARG_AMP_BRKFILE]);

    // pwmod used only if PWM square wave is selected
    FILE *pwm_file = fopen(argv[ARG_PWMOD_BRKFILE], "r");
    ON_FOPEN_ERROR(pwm_file, argv[ARG_PWMOD_BRKFILE]);

    // Get frequency breakpoints from file
    size_t freq_brk_size = 0;
    BRKSTREAM *freq_stream =
        bps_newstream(freq_file, outprops.srate, &freq_brk_size);
    if (freq_stream == NULL)
    {
        // Error message printed in bps_newstream()
        error++;
        goto cleanup;
    }

    // Validate frequency breakpoints
    MINMAX_PAIR freq_bounds =
        get_minmax(freq_stream->points, freq_stream->npoints);
    if (freq_bounds.min_val <= 0.0)
    {
        printf("Error: frequency breakpoint values must be positive, minimum "
               "was %lf in file %s\n",
               freq_bounds.min_val, argv[ARG_FREQ_BRKFILE]);
        error++;
        goto cleanup;
    }

    // Get amplitude breakpoint stream  from file
    size_t amp_brk_size = 0;
    BRKSTREAM *ampstream =
        bps_newstream(amp_file, outprops.srate, &amp_brk_size);
    if (ampstream == NULL)
    {
        // Error message printed in bps_newstream()
        error++;
        goto cleanup;
    }

    // Validate amplitude breakpoints
    MINMAX_PAIR amp_bounds = get_minmax(ampstream->points, amp_brk_size);
    if (amp_bounds.max_val > 1.0 || amp_bounds.min_val < 0.0)
    {
        printf("Error: amplitude values out of range in file %s\nAllowed "
               "values [0.0... 1.0]\n",
               argv[ARG_AMP_BRKFILE]);
        error++;
        goto cleanup;
    }

    // Get pulse width modulation breakpoints from file
    size_t pwm_brk_size = 0;
    BRKSTREAM *pwm_stream =
        bps_newstream(pwm_file, outprops.srate, &pwm_brk_size);
    if (pwm_stream == NULL)
    {
        // Error message printed in bps_newstream()
        error++;
        goto cleanup;
    }

    osc = new_oscil(outprops.srate);

    size_t outframes =
        (size_t)(duration * outprops.srate + 0.5);  // Number of output frames
    size_t nbufs = outframes / (NFRAMES);           // Number of buffers to fill
    size_t remainder = outframes - nbufs * NFRAMES; // Count of remaining frames
    if (remainder > 0)
        ++nbufs;

    outframe = malloc(outprops.chans * NFRAMES * sizeof(float));
    if (outframe == NULL)
    {
        printf("No memory\n");
        error++;
        goto cleanup;
    }

    // Processing
    unsigned int nframes =
        outprops.chans * NFRAMES; // Number of frames in buffer
    for (size_t i = 0; i < nbufs; i++)
    {
        if (i == nbufs - 1) // make only remainder amount of samples on last run
            nframes = remainder;
        for (unsigned int j = 0; j < nframes; j += outprops.chans)
        {
            double amplitude = bps_tick(ampstream);
            double frequency = bps_tick(freq_stream);
            double pwmod = bps_tick(pwm_stream);
            float sample_value =
                waveform_type == WAVE_PWM_SQUARE
                    ? (float)(amplitude * pwmtick(osc, frequency, pwmod))
                    : (float)(amplitude * tick(osc, frequency));
            for (unsigned chan = 0; chan < (unsigned)outprops.chans; chan++)
                outframe[j + chan] = sample_value;
        }

        int written_frames =
            psf_sndWriteFloatFrames(ofd, outframe, nframes / outprops.chans);
        if (written_frames != (int)nframes / outprops.chans)
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
    if (ampstream)
    {
        bps_freepoints(ampstream);
        free(ampstream);
    }
    if (freq_stream)
    {
        bps_freepoints(freq_stream);
        free(freq_stream);
    }
    if (amp_file)
        if (fclose(amp_file))
            printf("Error closing file %s\n", argv[ARG_AMP_BRKFILE]);
    if (freq_file)
        if (fclose(freq_file))
            printf("Error closing file %s\n", argv[ARG_FREQ_BRKFILE]);
    if (pwm_file)
        if (fclose(pwm_file))
            printf("Error: closing file %s\n", argv[ARG_PWMOD_BRKFILE]);
    psf_finish();
    return error;
}