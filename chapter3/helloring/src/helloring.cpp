#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <portaudio.h>

constexpr auto NFRAMES = 256ul;
constexpr auto SAMPLE_RATE = 44100;
constexpr auto TWO_PI = 2 * M_PI;

struct CallbackData
{
    double samplingIncrement, phase;
};

int audioCallback(const void *input, void *output, unsigned long frameCount,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags, void *userData)
{
    (void)timeInfo, (void)statusFlags; // Unused parameters

    auto cbData = (CallbackData *)userData;
    float *in = (float *)input, *out = (float *)output;
    for (auto i = 0ul; i < frameCount; ++i)
    {
        float sine = sin(cbData->phase);
        *out++ = *in++ * sine; // Left channel
        *out++ = *in++ * sine; // Right channel
        cbData->phase += cbData->samplingIncrement;
    }
    return paContinue;
}

auto main() -> int
{
    CallbackData cbData;
    cbData.phase = 0.0;

    // Get modulation frequency

    double frequency = 0;
    std::cout << "Type the modulator frequency in Hz\n>";
    std::cin >> frequency;
    cbData.samplingIncrement = TWO_PI * frequency / SAMPLE_RATE;

    // Initialize portaudio

    if (Pa_Initialize() != paNoError)
    {
        std::cerr << "Error: failed to initialize portaudio.\n";
        exit(EXIT_FAILURE);
    }

    // Print output audio device info
    for (auto i = 0; i < Pa_GetDeviceCount(); ++i)
    {
        auto info = Pa_GetDeviceInfo(i);
        auto hostApi = Pa_GetHostApiInfo(info->hostApi);
        if (info->maxOutputChannels > 0)
            std::cout << i << ": [" << hostApi->name << "] " << info->name
                      << '\n';
    }

    PaStreamParameters inputParameters, outputParameters;

    PaDeviceIndex outputDeviceId = 0;
    std::cout << "Type output device index from above list.\n>";
    std::cin >> outputDeviceId;
    auto info = Pa_GetDeviceInfo(outputDeviceId);
    auto hostApi = Pa_GetHostApiInfo(info->hostApi);
    std::cout << "Opening output device"
              << ": [" << hostApi->name << "] " << info->name << '\n';

    outputParameters.device = outputDeviceId;
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = info->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;

    // Print input audio device info
    for (auto i = 0; i < Pa_GetDeviceCount(); ++i)
    {
        auto info = Pa_GetDeviceInfo(i);
        auto hostApi = Pa_GetHostApiInfo(info->hostApi);
        if (info->maxInputChannels > 0)
            std::cout << i << ": [" << hostApi->name << "] " << info->name
                      << '\n';
    }

    PaDeviceIndex inputDeviceId = 0;
    std::cout << "Type input device index from above list.\n>";
    std::cin >> inputDeviceId;
    info = Pa_GetDeviceInfo(inputDeviceId);
    hostApi = Pa_GetHostApiInfo(info->hostApi);
    std::cout << "Opening input device"
              << ": [" << hostApi->name << "] " << info->name << '\n';

    inputParameters.device = inputDeviceId;
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = info->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    // Start processing

    PaStream *audiostream;
    Pa_OpenStream(&audiostream, &inputParameters, &outputParameters,
                  SAMPLE_RATE, NFRAMES, paClipOff, audioCallback, &cbData);
    Pa_StartStream(audiostream);

    std::cout << "Running... press q and enter to quit\n";
    while (std::cin.get() != 'q')
        Pa_Sleep(100);

    // Cleanup

    if (Pa_StopStream(audiostream) == paNoError)
        Pa_CloseStream(audiostream);
    else
        std::cerr << "Error: failed to stop audio stream.\n";
    Pa_Terminate();
}