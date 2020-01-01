/*
 * Extract an amplitude envelope from a mono sound file
 */

#include "portsf.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define DEFAULT_WINDOW_MSECS (15)

enum
{
    ARG_PROGNAME,
    ARG_INFILE,
    ARG_OUTFILE,
    ARG_NARGS
};

/*
 * Find peak in buf. Returns value of the peak.
 */
double sample_peak(float *buf, size_t blocksize)
{
    double absval, peak = 0.0;
    for (size_t i = 0; i < blocksize; i++)
    {
        absval = fabs(buf[i]);
        peak = MAX(peak, absval);
    }
    return peak;
}

int main(int argc, char *argv[])
{
    int ifd = 0;
    long frames_read = 0;
    int error = 0;
    PSF_PROPS inprops;
    float *inframe = NULL;
    FILE *out_file = NULL;
    double window_duration = DEFAULT_WINDOW_MSECS;

    printf("enx: extract an amplitude envelope from a mono sound file.\n");

    // Validate arguments
    if (argc > 1)
    {
        while (argv[1][0] == '-')
        {
            char flag = argv[1][1];
            switch (flag)
            {
            case 'w':
                window_duration = strtod(&argv[1][2], NULL);
                if (window_duration < 0.0)
                {
                    printf("Error: window duration must be positive, was %lf\n",
                           window_duration);
                    return EXIT_FAILURE;
                }
                break;

            default:
                break;
            }
            argc--;
            argv++;
        }
    }

    if (argc < ARG_NARGS)
    {
        printf("Insufficient arguments\nUsage: envx [-wN] infile "
               "outfile\ninfile is "
               "a soundfile, extracted breakpoints will be output to outfile "
               "in plain text\n\t-wN: set extraction window size to N "
               "milliseconds (defualt 15)\n");
        return EXIT_FAILURE;
    }

    ifd = psf_sndOpen(argv[ARG_INFILE], &inprops, 0);
    if (ifd < 0)
    {
        printf("Error: unable to open input file %s\n", argv[ARG_INFILE]);
    }

    // Verify infile format
    if (inprops.chans > 1)
    {
        printf("Error: input file must be mono\n%s contains %d channels\n",
               argv[ARG_INFILE], inprops.chans);
        error++;
        goto cleanup;
    }

    // Create breakpoint output file
    out_file = fopen(argv[ARG_OUTFILE], "w");
    if (!out_file)
    {
        printf("Error: unable to open output file %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    // Set buffersize to the required envelope window size
    window_duration /= 1000; // Convert to seconds
    size_t window_size = (size_t)(window_duration * inprops.srate);
    inframe = malloc(window_size * sizeof(float));
    if (!inframe)
    {
        printf("Error: no memory\n");
        error++;
        goto cleanup;
    }

    // Processing
    double breakpoint_time = 0.0; // time of current breakpoint
    size_t npoints = 0; // number of generated breakpoints
    while ((frames_read = psf_sndReadFloatFrames(ifd, inframe, window_size)) > 0)
    {
        double amp = sample_peak(inframe, window_size);
        if (fprintf(out_file, "%f\t%f\n", breakpoint_time, amp) < 2)
        {
            printf("Error: failed to write to output file%s\n",
                   argv[ARG_OUTFILE]);
            error++;
            goto cleanup;
        }
        breakpoint_time += window_duration;
        npoints++;
    }

    if (frames_read < 0)
    {
        printf("Error reading infile. Output file is incomplete\n");
        error++;
    }
    else
    {
        printf("Done. %d errors\n", error);
        printf("%ld breakpoints written to %s\n", npoints, argv[ARG_OUTFILE]);
    }

cleanup:
    if (ifd > 0)
        if (psf_sndClose(ifd))
            printf("Error: failed to close input file%s\n", argv[ARG_INFILE]);
    if (out_file)
        if (fclose(out_file))
            printf("Error: failed to close output file%s\n", argv[ARG_OUTFILE]);
    if (inframe)
        free(inframe);
    return error;
}