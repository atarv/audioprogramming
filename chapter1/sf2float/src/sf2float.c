// convert soundfile to floats
#include "sfprop.h"
#include <math.h>
#include <stdlib.h>

#define BLOCK_SIZE 4096

enum
{
    ARG_PORGNAME,
    ARG_INFILE,
    ARG_OUTFILE,
    ARG_LOOP_COUNT,
    ARG_NARGS
};

double float_to_db(float f) { return 20.0 * log10(f); }

int main(int argc, char const *argv[])
{
    PSF_PROPS props;
    long framesread, totalread;
    // Init all resources to default states
    int ifd = -1, ofd = -1;
    int error = 0; // psf error code
    psf_format outformat = PSF_FMT_UNKNOWN;
    PSF_CHPEAK *peaks = NULL;
    float *frame = NULL;

    printf("SF2FLOAT: convert soundfile to floats format\n");

    if (argc < ARG_NARGS - 1)
    {
        printf("Insufficient arguments\nUsage: sf2float infile outfile "
               "[loop_count]\n");
        return EXIT_FAILURE;
    }

    // If user did not provide loop count, loop only once
    const char *LOOP_COUNT_INPUT =
        (argc == ARG_NARGS) ? argv[ARG_LOOP_COUNT] : "1";
    const unsigned int LOOP_COUNT =
        (unsigned int)strtoul(LOOP_COUNT_INPUT, NULL, 10);
    if (!LOOP_COUNT)
    {
        printf("Loop count must be a positive non-zero integer\n");
        return EXIT_FAILURE;
    }

    // Be good, and startup portsf
    if (psf_init())
    {
        printf("Unable to start portsf\n");
        return EXIT_FAILURE;
    }

    ifd = psf_sndOpen(argv[ARG_INFILE], &props, 0);
    if (ifd < 0)
    {
        printf("Error: unable to open infile %s\n", argv[ARG_INFILE]);
        return EXIT_FAILURE;
    }

    printf("Properties of %s\n", argv[ARG_INFILE]);
    print_props(&props);

    // We now have a resource, so we use goto hereafter on hitting any error

    // Tell user if source file is already floats
    if (props.samptype == PSF_SAMP_IEEE_FLOAT)
    {
        printf("Info: infile is already in floats format\n");
        goto cleanup;
    }
    props.samptype = PSF_SAMP_IEEE_FLOAT;
    // check outfile extension is one we know about
    outformat = psf_getFormatExt(argv[ARG_OUTFILE]);
    if (outformat == PSF_FMT_UNKNOWN)
    {
        printf("Outfile name %s has unknown format\nUse any of .wav, .aiff",
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

    // allocate space for one sample frame
    frame = malloc(BLOCK_SIZE * props.chans * sizeof(float));
    if (frame == NULL)
    {
        printf("No memory!\n");
        error++;
        goto cleanup;
    }

    // and allocate space for PEAK info
    peaks = malloc(props.chans * sizeof(PSF_CHPEAK));
    if (peaks == NULL)
    {
        printf("No memory!\n");
        error++;
        goto cleanup;
    }
    printf("Copying...\n");

    // single block loop to do copy, report any errors
    framesread = psf_sndReadFloatFrames(ifd, frame, BLOCK_SIZE);
    totalread = 0;
    int update_interval = 0;
    for (unsigned int n = 0; n < LOOP_COUNT; n++)
    {
        while (framesread > 0)
        {
            totalread += framesread;
            if (psf_sndWriteFloatFrames(ofd, frame, BLOCK_SIZE) != BLOCK_SIZE)
            {
                printf("Error writing to outfile\n");
                error++;
                break;
            }
            // <--- do any processing here --->
            framesread = psf_sndReadFloatFrames(ifd, frame, BLOCK_SIZE);
            if (update_interval++ % 100 == 0)
                printf("%ld samples read\r", totalread);
        }
        // seek beginning of file for next iteration
        psf_sndSeek(ifd, 0, PSF_SEEK_SET);
        // read one frame so while loop above is entered
        framesread = psf_sndReadFloatFrames(ifd, frame, 1);
    }

    if (framesread < 0)
    {
        printf("Error reading infile. Outfile is incomplete\n");
        error++;
    }
    else
        printf("Done. %ld sample frames copied to %s\n", totalread,
               argv[ARG_OUTFILE]);

    // Report PEAK values to user
    if (psf_sndReadPeaks(ofd, peaks, NULL) > 0)
    {
        double peaktime = 0.0;
        printf("PEAK information:\n");
        for (long i = 0; i < props.chans; i++)
        {
            peaktime = (double)peaks[i].pos / props.srate;
            printf("CH %ld:\t%.1fdB (%.4f) at %.4f secs\n", i + 1,
                   float_to_db(peaks[i].val), peaks[i].val, peaktime);
        }
    }

// do all cleanup
cleanup:
    if (ifd >= 0)
        psf_sndClose(ifd);
    if (ofd >= 0)
        psf_sndClose(ofd);
    if (frame)
        free(frame);
    if (peaks)
        free(peaks);
    psf_finish();
    return error;
}
