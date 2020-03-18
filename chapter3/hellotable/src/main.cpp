#include <cmath>
#include <iostream>
#include <memory>
#include <portaudio.h>

constexpr int SAMPLE_RATE = 48000;
constexpr size_t TABLE_LENGTH = 512;
constexpr size_t FRAMES_PER_BUFFER = 1024;

enum Waveform
{
    Sine = 0,
    Square,
    Saw,
    Triangle
};

// Lookup table for waveform
float table[TABLE_LENGTH];

/* Fill lookup table with sine wave */
auto fillSine() -> void
{
    for (auto i = 0ul; i < TABLE_LENGTH; ++i)
        table[i] = (float)sin(2 * M_PI * i / TABLE_LENGTH);
}

auto fillSquare() -> void
{
    auto i = 0ul;
    for (; i < TABLE_LENGTH / 2; ++i)
        table[i] = 1;
    for (; i < TABLE_LENGTH; ++i)
        table[i] = -1;
}

auto fillSaw() -> void
{
    for (auto i = 0ul; i < TABLE_LENGTH; ++i)
        table[i] = 1.f - (2 * (float)i / TABLE_LENGTH);
}

auto fillTriangle() -> void
{
    auto i = 0ul;
    for (; i < TABLE_LENGTH / 2; ++i)
        table[i] = 2. * i / (TABLE_LENGTH / 2.) - 1.;
    for (; i < TABLE_LENGTH; ++i)
        table[i] =
            1. - (2. * (double)(i - TABLE_LENGTH / 2) / (TABLE_LENGTH / 2.));
}

auto main() -> int
{
    int exitCode = 0;
    int frequency = -1;
    double duration = -1;

    std::cout << "Enter the desired frequency (Hz): ";
    std::cin >> frequency;
    if (frequency < 1)
    {
        std::cerr << "Error: frequency must be positive, was " << frequency
                  << '\n';
        exit(EXIT_FAILURE);
    }

    std::cout << "Enter the desired duration (seconds): ";
    std::cin >> duration;
    if (duration <= 0)
    {
        std::cerr << "Error: duration must be greater than zero, was "
                  << duration << '\n';
        exit(EXIT_FAILURE);
    }

    std::cout
        << "Choose waveform (0 = sine, 1 = square, 2 = saw, 3 = triangle): ";
    int selectedWaveform = -1;
    std::cin >> selectedWaveform;
    if (selectedWaveform < Waveform::Sine ||
        selectedWaveform > Waveform::Triangle)
    {
        std::cerr << "Error: invalid waveform, was " << selectedWaveform
                  << '\n';
        exit(EXIT_FAILURE);
    }
    const Waveform wave = Waveform(selectedWaveform);

    switch (wave)
    {
    case Waveform::Sine:
        fillSine();
        break;

    case Waveform::Square:
        fillSquare();
        break;

    case Waveform::Saw:
        fillSaw();
        break;

    case Waveform::Triangle:
        fillTriangle();
        break;

    default:
        std::cerr << "Error: couldn't decide lookup table fill method\n";
        exit(EXIT_FAILURE);
        break;
    }

    int paErr = Pa_Initialize();
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
    outputParams.channelCount = 1;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency =
        Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    PaStream *stream;
    paErr =
        Pa_OpenStream(&stream, NULL /* no input */, &outputParams, SAMPLE_RATE,
                      FRAMES_PER_BUFFER, paClipOff, NULL, NULL);
    if (paErr != paNoError)
    {
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

    {
        const size_t bufferCount = (duration * SAMPLE_RATE) / FRAMES_PER_BUFFER;
        // std::vector<float> buffer(FRAMES_PER_BUFFER);
        std::unique_ptr<float[]> buffer(new float[FRAMES_PER_BUFFER]);
        const double sampleIncrement = frequency * TABLE_LENGTH / SAMPLE_RATE;
        // How many buffers have to be filled and output
        double phase = 0; // Phase of waveform
        for (auto i = 0lu; i < bufferCount; ++i)
        {
            // Fill buffer with wave
            for (auto j = 0lu; j < FRAMES_PER_BUFFER; ++j)
            {
                buffer[j] = table[(size_t)phase];
                phase += sampleIncrement;
                if (phase > TABLE_LENGTH) // return to beginning of table
                    phase -= TABLE_LENGTH;
            }
            // Output buffer to stream
            paErr = Pa_WriteStream(stream, buffer.get(), FRAMES_PER_BUFFER);
            if (paErr != paNoError)
            {
                std::cerr << "Error: writing to stream failed\n"
                          << Pa_GetErrorText(paErr) << '\n';
                ++exitCode;
                goto cleanup;
            }
        }
    }

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

cleanup:
    if (paErr == paNoError)
        if (Pa_Terminate() != paNoError)
            std::cerr << "Error: failed to terminate portaudio\n";
    return exitCode;
}
