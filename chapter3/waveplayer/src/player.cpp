/**
 * `player` plays a wav-file through your deafult audio device
 * Usage: ./player /path/to/file.wav
 */
#include "portsf.h"
#include <functional>
#include <iostream>
#include <memory>
#include <portaudio.h>
#include <unistd.h>

// Number of frames in sample buffer
constexpr long NFRAMES = 2048;

void showHelp()
{
    std::cout << "Usage: player [-tTIME] [-dDURATION] file.wav\n";
}

auto main(int argc, char *const *argv) -> int
{
    int exitCode = 0; // 0 for success, positive for error
    int paErr = -1;   // Negative number signifies erorr with Portaudio
    double duration = -1.0;
    double startTime = 0.0;
    opterr = 0; // Disable getopt's default logging

    // Parse and validate arguments

    int optionChar;
    while ((optionChar = getopt(argc, argv, "d:t:")) != -1)
        switch (optionChar)
        {
        case 'd': // Play duration
            if ((duration = std::strtod(optarg, nullptr)) <= 0.0)
            {
                std::cerr
                    << "Error: argument for -d (duration) must be positive.\n";
                exit(EXIT_FAILURE);
            }
            break;

        case 't': // Start time
            if ((startTime = std::strtod(optarg, nullptr)) < 0.0)
            {
                std::cerr << "Error: argument for -t (start time) must be "
                             "non-negative.\n";
                exit(EXIT_FAILURE);
            }
            break;

        case '?': // Invalid or missing argument
            if (optopt == 'c')
                std::cerr << "Option " << (char)optopt
                          << " requires an argument.\n";
            else if (std::isprint(optopt))
                std::cerr << "Unknown option " << (char)optopt << ".\n";
            else
                std::cerr << "Unknown option character " << std::hex << optopt
                          << ".\n";
            exit(EXIT_FAILURE);
            break;

        default:
            exit(EXIT_FAILURE);
            break;
        }

    if (optind == argc)
    {
        std::cerr << "Error: File not specified.\n";
        showHelp();
        exit(EXIT_FAILURE);
    }

    int psfFailure = psf_init();
    if (psfFailure)
    {
        std::cerr << "Error: failed to initialize libportsf.\n";
        exit(EXIT_FAILURE);
    }

    // Validate input file properties

    PSF_PROPS props;
    int inputFileDesc = psf_sndOpen(argv[optind], &props, 0);
    if (inputFileDesc < 0)
    {
        std::cerr << "Error: failed to open sound file " << argv[optind]
                  << '\n';
        ++exitCode;
        goto cleanup;
    }

    if (props.format != PSF_STDWAVE)
    {
        std::cerr << "Error: only wav-files are supported.\n";
        ++exitCode;
        goto cleanup;
    }

    if (props.chformat > 2)
    {
        std::cerr << "Error: only stereo and mono files are allowed.\n";
        ++exitCode;
        goto cleanup;
    }

    // Setup portaudio

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
        std::cerr << "Error: no default output device.\n";
        goto cleanup;
    }
    outputParams.channelCount = props.chans;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency =
        Pa_GetDeviceInfo(outputParams.device)->defaultHighOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    {
        // Portaudio owns this pointer. It must be freed with Pa_StopStream
        // and Pa_CloseStream functions.
        PaStream *stream_raw;
        paErr =
            Pa_OpenStream(&stream_raw, nullptr /* no input */, &outputParams,
                          props.srate, NFRAMES, paClipOff, nullptr, nullptr);
        if (paErr != paNoError)
        {
            std::cerr << "Error: failed to open output stream.\n"
                      << Pa_GetErrorText(paErr) << '\n';
            ++exitCode;
            goto cleanup;
        }
        // Construct a unique pointer for stream. This will automatically handle
        // proper closing of the stream.
        std::unique_ptr<PaStream *, std::function<void(PaStream **)>> stream(
            nullptr, [](PaStream **p) {
                if (p != nullptr)
                    if (Pa_StopStream(*p) == paNoError)
                        if (Pa_CloseStream(*p) == paNoError)
                            return;
                std::cerr << "Warning: failed to close stream.\n";
            });
        stream.reset(&stream_raw); // Claim ownership for raw pointer

        paErr = Pa_StartStream(*stream);
        if (paErr != paNoError)
        {
            std::cerr << "Error: failed to start stream.\n"
                      << Pa_GetErrorText(paErr) << '\n';
            ++exitCode;
            goto cleanup;
        }

        // Determine start point and duration in frames

        int startFrame = (startTime * props.srate);
        if (psf_sndSeek(inputFileDesc, startFrame, PSF_SEEK_SET) < 0)
        {
            std::cerr << "Error: failed to set start position.\n";
            ++exitCode;
            goto cleanup;
        }

        int remainingFrames = duration > 0.0 ? (int)(duration * props.srate)
                                             : psf_sndSize(inputFileDesc);
        if (remainingFrames < 0)
        {
            std::cerr << "Error: failed to determine play duration.\n";
            ++exitCode;
            goto cleanup;
        }

        // Play audio file through stream
        unsigned totalFramesRead = 0;
        int framesRead = 0;
        const long samples = props.chans * NFRAMES;
        std::unique_ptr<float[]> sampleBuffer(new float[samples]);
        do
        {
            int framesToRead =
                remainingFrames > NFRAMES ? NFRAMES : remainingFrames;
            // Read frames from file
            framesRead = psf_sndReadFloatFrames(
                inputFileDesc, sampleBuffer.get(), framesToRead);
            remainingFrames -= framesRead;
            if (framesRead < 0)
            {
                std::cerr << "Error: failed to read from input file. Start "
                             "point -t may be over file's length.\n";
                ++exitCode;
                goto cleanup;
            }
            // Write frames to output stream
            paErr = Pa_WriteStream(*stream, (const void *)sampleBuffer.get(),
                                   framesRead);
            if (paErr != paNoError)
            {
                std::cerr << "Error: failed to write to output stream.\n"
                          << Pa_GetErrorText(paErr) << '\n';
                ++exitCode;
                goto cleanup;
            }
            totalFramesRead += framesRead;
        } while (remainingFrames > 0 && framesRead > 0);

        std::cout << "Total frames read: " << totalFramesRead << '\n';
    }

cleanup:
    if (inputFileDesc >= 0)
        psf_sndClose(inputFileDesc);
    if (paErr == paNoError)
        if (Pa_Terminate() != paNoError)
        {
            std::cerr << "Error: failed to terminate portaudio.\n"
                      << Pa_GetErrorText(paErr) << '\n';
        }
    if (psfFailure == 0)
        psf_finish();
    return exitCode;
}
