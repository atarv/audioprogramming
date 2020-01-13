/**
 * Modifies amplitude of a sound file. Based on sf2float.
 */
#include "portsf.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
    ARG_PROGNAME,
    ARG_INFILE,
    ARG_OUTFILE,
    ARG_GAIN_MOD,
    ARG_NARGS
};

int main(int argc, char *argv[])
{
    PSF_PROPS props;
    long frames_read, total_read;
    int ifd = -1, ofd = -1; // input & output file descriptors
    int error = 0;
    psf_format outformat = PSF_FMT_UNKNOWN;
    float *frame = NULL;

    printf("sfgain: modify amplitude of a soundfile\n");

    // Validate arguments
    if (argc < ARG_NARGS)
    {
        printf("Error: Insufficient arguments\nUsage: sfgain infile outfile "
               "modifier\n");
        return EXIT_FAILURE;
    }

    float gain_mod = strtof(argv[ARG_GAIN_MOD], NULL);
    if (gain_mod < 0.0 || isnan(gain_mod))
    {
        printf(
            "Error: Gain modifier must be a positive floating point number\n");
        return EXIT_FAILURE;
    }
    else if (gain_mod == 1.0)
    {
        printf("Error: Gain modifier has to differ from 1.0 to modify "
               "amplitude. Exiting...\n");
        return EXIT_FAILURE;
    }

    if (psf_init())
    {
        printf("Error: Unable to start portsf\n");
        return EXIT_FAILURE;
    }

    ifd = psf_sndOpen(argv[ARG_INFILE], &props, 0);
    if (ifd < 0)
    {
        printf("Error: unable to open inputfile %s\n", argv[ARG_INFILE]);
        return EXIT_FAILURE;
    }

    props.samptype = PSF_SAMP_IEEE_FLOAT;
    outformat = psf_getFormatExt(argv[ARG_OUTFILE]);
    if (outformat == PSF_FMT_UNKNOWN)
    {
        printf(
            "Error: Outfile name %s has unknown format\nUse any of .wav, .aiff",
            argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }
    props.format = outformat;

    ofd = psf_sndCreate(argv[ARG_OUTFILE], &props, 0, 0, PSF_CREATE_RDWR);
    if (ofd < 0)
    {
        printf("Error: unable to create outfile %s\n", argv[ARG_OUTFILE]);
        error++;
        goto cleanup;
    }

    // allocate space for one frame
    frame = malloc(props.chans * sizeof(float));
    if (frame == NULL)
    {
        printf("Error: No memory\n");
        error++;
        goto cleanup;
    }

    printf("Processing...\n");

    frames_read = psf_sndReadFloatFrames(ifd, frame, 1);
    total_read = 0;
    int update_interval = 0; // essentially a loop counter
    while (frames_read == 1)
    {
        total_read += frames_read;
        for (int i = 0; i < props.chans; i++)
            frame[i] *= gain_mod;
        if (psf_sndWriteFloatFrames(ofd, frame, 1) != 1)
        {
            printf("Error writing to outfile\n");
            error++;
            goto cleanup;
        }

        frames_read = psf_sndReadFloatFrames(ifd, frame, 1);
        if (update_interval++ % 2064 == 0)
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
    if (frame)
        free(frame);
    psf_finish();
    return error;
}
