#include "sfprop.h"

/* Print all properties of a sound file */
void print_props(const PSF_PROPS *props)
{
    printf("Sample type: %s\n"
           "File format: %s\n"
           "Sample rate: %d\n"
           "Channels: %d\n"
           ,
           sget_stype(props), sget_format(props), props->srate, props->chans);
}

/* get sample type as a string */
char *sget_stype(const PSF_PROPS *props)
{
    char *stype = NULL;
    switch (props->samptype)
    {
    case PSF_SAMP_16:
        stype = "16bit";
        break;
    case PSF_SAMP_24:
        stype = "24bit";
        break;
    case PSF_SAMP_32:
        stype = "32bit";
        break;
    case PSF_SAMP_IEEE_FLOAT:
        stype = "32bit (float)";
        break;
    default:
        stype = "Unknown sample type";
        break;
    }
    return stype;
}

/* Get format of a soundfile as a string */
char *sget_format(const PSF_PROPS *props)
{
    char *fmt = NULL;
    switch (props->format)
    {
    case PSF_AIFF:
        fmt = "AIFF";
        break;
    case PSF_AIFC:
        fmt = "AIFC";
        break;
    case PSF_WAVE_EX:
        fmt = "WAVE";
        break;
    case PSF_STDWAVE:
        fmt = "WAV";
        break;
    default:
        fmt = "Unknown format";
        break;
    }
    return fmt;
}
