/*
 * Apply amplitude envelope on a sound file
 * Usage: sfenv [-n] infile outfile brkfile
 * -n: normalize breakpoint values to 1.0
 */
#include "breakpoints.h"
#include "portsf.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NFRAMES 1024

enum
{
    ARG_PROGNAME,
    ARG_INFILE,
    ARG_OUTFILE,
    ARG_BREAKPOINT_FILE,
    ARG_NARGS
};

int main(int argc, char *argv[])
{
    PSF_PROPS inprops, outprops;
    long frames_read, total_read;
    int ifd = -1, ofd = -1; // input & output file descriptors
    int error = 0;
    psf_format outformat = PSF_FMT_UNKNOWN;
    float *inframe = NULL;
    // Breakpoints
    FILE *fp = NULL;
    size_t points_count = 0;
    BREAKPOINT *points = NULL;
    bool normalize = false;

    printf("sfenv: apply amplitude envelope on a soundfile\n");

    if (argc < ARG_NARGS)
    {
        printf("Insufficient arguments\nUsage: sfenv [-n] infile outfile "
               "breakpointfile\nBreakpoint file contains time value value "
               "pairs between 0.0 and 1.0 (inclusive)\n-n:\tnormalize "
               "breakpoint values to 1.0\n");
        return EXIT_FAILURE;
    }

    // Get command line flags
    for (int i = 1; i < ARG_NARGS; i++)
    {
        if (argv[i][0] == '-')
        {
            char flag = argv[i][1];
            switch (flag)
            {
            case 'n':
                normalize = true;
                argc--;
                argv++;
                break;
            default:
                printf("Unknown flag -%c\n", flag);
                return EXIT_FAILURE;
                break;
            }
        }
    }

    if (psf_init())
    {
        printf("Unable to start portsf\n");
        return EXIT_FAILURE;
    }

    ifd = psf_sndOpen(argv[ARG_INFILE], &inprops, 0);
    if (ifd < 0)
    {
        printf("Error: unable to open inputfile %s\n", argv[ARG_INFILE]);
        return EXIT_FAILURE;
    }

    inprops.samptype = PSF_SAMP_IEEE_FLOAT;
    outformat = psf_getFormatExt(argv[ARG_OUTFILE]);
    if (outformat == PSF_FMT_UNKNOWN)
    {
        printf("Outfile name %s has unknown format\nUse any of .wav, .aiff\n",
               argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }
    inprops.format = outformat;

    outprops = inprops;
    ofd = psf_sndCreate(argv[ARG_OUTFILE], &outprops, 0, 0, PSF_CREATE_RDWR);
    if (ofd < 0)
    {
        printf("Error: unable to create outfile %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    // Read breakpoint file and validate
    fp = fopen(argv[ARG_BREAKPOINT_FILE], "r");
    if (fp == NULL)
    {
        printf("Error: unable to open file %s\n", argv[ARG_BREAKPOINT_FILE]);
        error++;
        goto cleanup;
    }
    points = get_breakpoints(fp, &points_count);
    if (points == NULL)
    {
        printf("No breakpoints read\n");
        error++;
        goto cleanup;
    }
    else if (points_count < 2)
    {
        printf("Minimum of 2 breakpoints required\n");
        error++;
        goto cleanup;
    }
    else if (points[0].time != 0.0)
    {
        printf("First breakpoint's time must be 0.0 (got %f)\n",
               points[0].time);
        error++;
        goto cleanup;
    }

    MINMAX_PAIR p = get_minmax(points, points_count);
    printf("\tmax %lf\n", p.max_val);
    normalize_breakpoints(points, points_count, p.max_val, 1.0);
    p = get_minmax(points, points_count);
    if (normalize)
        printf("\tnormalized %lf\n", p.max_val);

    if (!in_range(points, 0.0, 1.0, points_count))
    {
        printf("Error: out of range value breakpoints\n");
        error++;
        goto cleanup;
    }

    // Allocate memory for reading and writing frames
    inframe = malloc(NFRAMES * inprops.chans * sizeof(float));
    if (inframe == NULL)
    {
        printf("No memory\n");
        error++;
        goto cleanup;
    }

    printf("Processing...\n");

    total_read = 0;          // total amount of frames read from input file
    int update_interval = 0; // essentially a loop counter
    // amount of seconds between samples
    const double time_incr = 1.0 / inprops.srate;
    double sample_time = 0.0; // current time on sample
    size_t i_left = 0,
           i_right = 1; // counter values for left and right breakpoint
    BREAKPOINT left_point = points[i_left];
    BREAKPOINT right_point = points[i_right];
    double width = right_point.time - left_point.time;
    double height = right_point.value - left_point.value;
    bool more_points = true; // false if end of data
    double thisamp = 0.0;
    double fraction = 0.0;
    while ((frames_read = psf_sndReadFloatFrames(ifd, inframe, NFRAMES)) > 0)
    {
        for (int i = 0; i < frames_read; i++)
        {
            if (more_points)
            {
                if (width == 0.0)
                    thisamp = right_point.value;
                else
                {
                    fraction = (sample_time - left_point.time) / width;
                    thisamp = left_point.value + (height * fraction);
                }
                // Move to next sample
                sample_time += time_incr;
                // Step to next span, if available
                if (sample_time > right_point.time)
                {
                    i_left++, i_right++;
                    if (i_right < points_count) // another span available
                    {
                        left_point = points[i_left];
                        right_point = points[i_right];
                        width = right_point.time - left_point.time;
                        height = right_point.value - left_point.value;
                    }
                    else // otherwise end of data
                        more_points = false;
                }
            }
            inframe[i] = (float)(inframe[i] * thisamp);
        }

        if (psf_sndWriteFloatFrames(ofd, inframe, frames_read) != frames_read)
        {
            printf("Error writing to outfile\n");
            error++;
            goto cleanup;
        }

        total_read += frames_read;
        if (update_interval++ % 1024 == 0)
            printf("%ld samples processed\r", total_read);
    }

    if (frames_read < 0)
    {
        printf("Error reading infile. Outfile is incomplete\n");
        error++;
    }
    else
        printf("Done. %ld sample frames written to %s\n", total_read,
               argv[ARG_OUTFILE]);

// do all cleanup
cleanup:
    if (ifd >= 0)
        psf_sndClose(ifd);
    if (ofd >= 0)
        psf_sndClose(ofd);
    if (inframe)
        free(inframe);
    if (points)
        free(points);
    if (fp)
        fclose(fp);
    psf_finish();
    return error;
}
