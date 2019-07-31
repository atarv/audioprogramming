/**
 * Normalizes a sound file
 * Usage: sfnorm input_file output_file dBvalue
 */
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "portsf.h"

#define NFRAMES 1024
#define MAX(x, y) ((x) > (y) ? (x) : (y))

enum
{
    ARG_PROGNAME,
    ARG_INFILE,
    ARG_OUTFILE,
    ARG_DB_VAL,
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

/* Converts floating point value to decibels */
double float_to_db(float f)
{
    return 20.0 * log10(f);
}

int main(int argc, char *argv[])
{
    PSF_PROPS props;
    long frames_read, total_read;
    int ifd = -1, ofd = -1; // input & output file descriptors
    int error = 0;
    psf_format outformat = PSF_FMT_UNKNOWN;
    PSF_CHPEAK *peaks = NULL;
    float *frames = NULL;
    // dbval is decibel value from user
    // inpeak is peak of the input file
    double dbval = 0, inpeak = 0;
    // ampfac - amplification factor
    // scalefac - calculated scale factor
    float ampfac = 0.0, scalefac = 0.0;

    printf("sfnorm: Normalize a sound file\n");

    // Validate arguments
    if (argc < ARG_NARGS)
    {
        printf("Insufficient arguments\nUsage: sfnorm infile outfile dB\n");
        return EXIT_FAILURE;
    }

    // validate dB value
    dbval = strtof(argv[ARG_DB_VAL], NULL);
    if (dbval > 0.0 || isnan(dbval))
    {
        printf("dB must be negative\n");
        return EXIT_FAILURE;
    }
    ampfac = (float)pow(10.0, dbval / 20.0); // calculate apmlification factor

    if (psf_init())
    {
        printf("Unable to start portsf\n");
        return EXIT_FAILURE;
    }

    ifd = psf_sndOpen(argv[ARG_INFILE], &props, 0);
    if (ifd < 0)
    {
        printf("Error: unable to open inputfile %s\n", argv[ARG_INFILE]);
        return EXIT_FAILURE;
    }

    // set output format
    props.samptype = PSF_SAMP_IEEE_FLOAT;
    outformat = psf_getFormatExt(argv[ARG_OUTFILE]);
    if (outformat == PSF_FMT_UNKNOWN)
    {
        printf("Outfile name %s has unknown format\nUse any of .wav, .aiff",
               argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }
    props.format = outformat;

    // allocate space for frames
    frames = malloc(NFRAMES * props.chans * sizeof(float));
    if (frames == NULL)
    {
        printf("No memory\n");
        error++;
        goto cleanup;
    }

    // allocate space for PEAK info
    peaks = malloc(props.chans * sizeof(PSF_CHPEAK));
    if (peaks == NULL)
    {
        printf("No memory\n");
        error++;
        goto cleanup;
    }

    printf("Processing...\n");

    frames_read = psf_sndReadFloatFrames(ifd, frames, NFRAMES);
    total_read = 0;
    int update_interval = 0; // essentially a loop counter

    // Read peaks from file header or...
    if (psf_sndReadPeaks(ifd, peaks, NULL) > 0)
    {
        for (int i = 0; i < props.chans; i++)
            inpeak = MAX(inpeak, peaks[i].val);
    }
    else // scan file for peaks.
    {
        while (frames_read > 0)
        {
            double thispeak = sample_peak(frames, props.chans * NFRAMES);
            inpeak = MAX(inpeak, thispeak);
            frames_read = psf_sndReadFloatFrames(ifd, frames, NFRAMES);
        }
        // seek back to beginning of file
        if (psf_sndSeek(ifd, 0, PSF_SEEK_SET) < 0)
        {
            printf("Error: unable to rewind file\n");
            error++;
            goto cleanup;
        }
        // Read frames from beginning
        frames_read = psf_sndReadFloatFrames(ifd, frames, NFRAMES);
    }

    if (inpeak == 0.0)
    {
        printf("infile is silent. Outfile not created\n");
        goto cleanup;
    }
    printf("Peak of input file at %.2fdB\nNormalizing to %.2fdB\n", float_to_db(inpeak), dbval);

    // Scale factor used to normalize
    scalefac = (float)(ampfac / inpeak);

    // Create output file
    ofd = psf_sndCreate(argv[ARG_OUTFILE], &props, 0, 0, PSF_CREATE_RDWR);
    if (ofd < 0)
    {
        printf("Error: unable to create outfile %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    // Normalize
    while (frames_read > 0)
    {
        total_read += frames_read;
        for (int i = 0; i < NFRAMES * props.chans; i++)
            frames[i] *= scalefac;
        if (psf_sndWriteFloatFrames(ofd, frames, NFRAMES) != NFRAMES)
        {
            printf("Error writing to outfile\n");
            error++;
            goto cleanup;
        }

        frames_read = psf_sndReadFloatFrames(ifd, frames, NFRAMES);
        if (update_interval++ % 100 == 0)
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
    if (frames)
        free(frames);
    if (peaks)
        free(peaks);
    psf_finish();
    return error;
}
