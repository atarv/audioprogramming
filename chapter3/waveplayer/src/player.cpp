#include "portsf.h"
#include <functional>
#include <iostream>
#include <memory>
#include <portaudio.h>

constexpr size_t NFRAMES = 2048;

auto psf_to_pa_sampleformat(psf_stype psf) -> PaSampleFormat
{
    switch (psf)
    {
    case PSF_SAMP_8:
        return paInt8;
        break;
    case PSF_SAMP_16:
        return paInt16;
        break;
    case PSF_SAMP_24:
        return paInt24;
        break;
    case PSF_SAMP_32:
        return paInt32;
    case PSF_SAMP_IEEE_FLOAT:
        return paFloat32;
        break;
    default:
        std::cerr << "Error: invalid sound file format\n";
        exit(1); // No chance for cleanup...
        break;
    }
}

auto main(int argc, char const *argv[]) -> int
{
    int exitCode = 0;
    int paErr = -1;
    float *sampleBuffer;

    if (argc < 2)
    {
        std::cerr
            << "Error: too few arguments. Usage: ./player /path/to/file.wav\n";
        exit(EXIT_FAILURE);
    }

    int psfFailure = psf_init();
    if (psfFailure)
    {
        std::cerr << "Error: failed to initialize libportsf\n";
        exit(EXIT_FAILURE);
    }

    PSF_PROPS props;
    int inputFileDesc = psf_sndOpen(argv[1], &props, 0);
    if (inputFileDesc < 0)
    {
        std::cerr << "Error: failed to open sound file\n";
        ++exitCode;
        goto cleanup;
    }

    // DEBUG:
    std::cout << "chans: " << props.chans << " chformat: " << props.chformat
              << " srate: " << props.srate << " format: " << props.format
              << '\n';

    if (props.format != PSF_STDWAVE)
    {
        std::cerr << "Error: only .wav-files are supported\n";
        ++exitCode;
        goto cleanup;
    }
    if (props.chformat > 2)
    {
        std::cerr << "Error: only stereo and mono files are allowed\n";
        ++exitCode;
        goto cleanup;
    }

    paErr = Pa_Initialize();
    if (paErr != paNoError)
    {
        ++exitCode;
        std::cerr << "Error initializing portaudio: " << Pa_GetErrorText(paErr)
                  << '\n';
        goto cleanup;
    }

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice)
    {
        ++exitCode;
        std::cerr << "Error: no default output device\n";
        goto cleanup;
    }
    outputParams.channelCount = props.chans;
    outputParams.sampleFormat = psf_to_pa_sampleformat(props.samptype);
    // outputParams.sampleFormat = paInt16;
    outputParams.suggestedLatency =
        Pa_GetDeviceInfo(outputParams.device)->defaultHighOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    {
        PaStream *stream;
        paErr =
            Pa_OpenStream(&stream, nullptr /* no input */, &outputParams,
                          props.srate, NFRAMES, paClipOff, nullptr, nullptr);
        if (paErr != paNoError)
        {
            // std::cerr << (void*)stream.get() << '\n';
            std::cerr << "Error: failed to open output stream.\n"
                      << Pa_GetErrorText(paErr) << '\n';
            ++exitCode;
            goto cleanup;
        }


        paErr = Pa_StartStream(stream);
        if (paErr != paNoError)
        {
            std::cerr << "Error: failed to start stream\n"
                      << Pa_GetErrorText(paErr) << '\n';
            ++exitCode;
            goto cleanup;
        }

        // TODO: play file
        unsigned totalFramesRead = 0;
        int framesRead = 0;
        const long samples = props.chans * NFRAMES;
        sampleBuffer = new float[samples];
        do
        {
            // Read frames from file
            framesRead = psf_sndReadFloatFrames(
                inputFileDesc, (float *)sampleBuffer, NFRAMES);
            // Write frames to output stream
            paErr = Pa_WriteStream(stream, (const void *)sampleBuffer,
                                   framesRead);
            if (paErr != paNoError)
            {
                std::cerr << "Error: failed to write to output stream\n";
                ++exitCode;
                goto cleanup;
            }
            totalFramesRead += framesRead;
        } while (framesRead > 0);
            std::cout << "Total frames read: " << totalFramesRead << '\n';

        paErr = Pa_StopStream(stream);
        if (paErr != paNoError)
        {
            std::cerr << "Error: failed to stop stream\n";
            ++exitCode;
            goto cleanup;
        }

        paErr = Pa_CloseStream(stream);
        if (paErr != paNoError)
        {
            std::cerr << "Error: failed to close stream\n";
            ++exitCode;
            goto cleanup;
        }
        delete sampleBuffer;
    }

cleanup:
    if (inputFileDesc >= 0)
        psf_sndClose(inputFileDesc);
    if (paErr == paNoError)
        if (Pa_Terminate() != paNoError)
            std::cerr << "Error: failed to terminate portaudio\n";
    if (psfFailure == 0)
        psf_finish();
    return exitCode;
}
