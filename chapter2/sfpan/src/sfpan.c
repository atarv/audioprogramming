/**
 * Do panning on a stereo sound file
 * Usage: sfpan infile outfile breakpointfile
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

typedef struct panpos
{
    double left, right;
} PANPOS;

/*
 * Calculates volumes for left and right channel
 */
PANPOS simplepan(double position)
{
    PANPOS pan;
    position *= 0.5;
    pan.left = position - 0.5;
    pan.right = position + 0.5;
    return pan;
}

/*
 * Constant-power panning function
 */
PANPOS constpower_pan(double position)
{
    PANPOS pan;
    // pi/2: 1/4 cycle of a sinusoid
    const double pi_over2 = 4 * atan(1) / 2.0;
    const double root2_over2 = sqrt(2.0) / 2.0;
    double thispos = position * pi_over2;
    double angle = thispos * 0.5;

    pan.left = root2_over2 * (cos(angle) - sin(angle));
    pan.right = root2_over2 * (cos(angle) + sin(angle));
    return pan;
}

int main(int argc, char *argv[])
{
    PSF_PROPS inprops, outprops;
    long frames_read, total_read;
    int ifd = -1, ofd = -1; // input & output file descriptors
    int error = 0;
    psf_format outformat = PSF_FMT_UNKNOWN;
    float *inframe = NULL, *outframe = NULL;
    // Breakpoints
    FILE *fp = NULL;
    size_t points_count = 0;
    BREAKPOINT *points = NULL;

    printf("sfpan: pan a soundfile\n");

    // Validate arguments
    if (argc < ARG_NARGS)
    {
        printf("Insufficient arguments\nUsage: sfpan infile outfile "
               "breakpointfile\nBreakpoint file contains time value value "
               "pairs between -1.0 and 1.0 (inclusive)\n");
        return EXIT_FAILURE;
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

    if (inprops.chans != 1)
    {
        printf("Error: input file must be mono\n");
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
    outprops.chans = 2;
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
    if (points_count < 2)
    {
        printf("Minimum of 2 breakpoints required\n");
        error++;
        goto cleanup;
    }
    if (points[0].time != 0.0)
    {
        printf("First breakpoint's time must be 0.0 (got %f)\n",
               points[0].time);
        error++;
        goto cleanup;
    }

    if (!in_range(points, -1.0, 1.0, points_count))
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

    outframe = malloc(NFRAMES * outprops.chans * sizeof(float));
    if (outframe == NULL)
    {
        printf("No memory\n");
        error++;
        goto cleanup;
    }

    printf("Processing...\n");

    total_read = 0;          // total amount of frames read from input file
    int update_interval = 0; // essentially a loop counter
    const double time_incr =
        1.0 / inprops.srate;  // amount of seconds between samples
    double sample_time = 0.0; // current time of sample
    while ((frames_read = psf_sndReadFloatFrames(ifd, inframe, NFRAMES)) > 0)
    {
        // Panning
        for (int i = 0, out_i = 0; i < frames_read; i++)
        {
            double stereopos =
                val_at_brktime(points, points_count, sample_time);
            PANPOS thispos = constpower_pan(stereopos);
            outframe[out_i++] = inframe[i] * thispos.left;
            outframe[out_i++] = inframe[i] * thispos.right;
            sample_time += time_incr;
        }

        if (psf_sndWriteFloatFrames(ofd, outframe, frames_read) != frames_read)
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
        printf("Done. %ld sample frames copied to %s\n", total_read,
               argv[ARG_OUTFILE]);

// do all cleanup
cleanup:
    if (ifd >= 0)
        psf_sndClose(ifd);
    if (ofd >= 0)
        psf_sndClose(ofd);
    if (inframe)
        free(inframe);
    if (outframe)
        free(outframe);
    if (points)
        free(points);
    if (fp)
        fclose(fp);
    psf_finish();
    return error;
}
