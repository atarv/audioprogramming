#include "../libportsf/portsf.h"
#include <math.h>
#include <stdio.h>

enum
{
    ARG_PROGRAM,
    ARG_INPUT_FILE
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        perror("Too few arguments");
        return 1;
    }
    else if (argc > 2)
    {
        perror("Too many arguments");
        return 1;
    }

    if (psf_init())
    {
        perror("Failed to initialize portsf\n");
        return 1;
    }

    PSF_PROPS props;
    int sf = psf_sndOpen(argv[ARG_INPUT_FILE], &props, 0);

    if (sf < 0)
    {
        printf("Error %d: unable to open soundfile %s\n", sf,
               argv[ARG_INPUT_FILE]);
        return 1;
    }

    char *stype = NULL;
    stype = "asdf";
    switch (props.samptype)
    {
    case (PSF_SAMP_16):
        stype = "16 bit\n";
        break;
    case (PSF_SAMP_24):
        stype = "24 bit\n";
        break;
    case (PSF_SAMP_32):
        stype = "32 bit (integer)\n";
        break;
    case (PSF_SAMP_IEEE_FLOAT):
        stype = "32 bit (floating point)\n";
        break;
    default:
        stype = "Invalid";
        break;
    }
    printf("Sample type: %s", stype);
    printf("Sample rate: %d\nNumber of channels: %d\n", props.srate,
           props.chans);

    psf_finish();
    return 0;
}
