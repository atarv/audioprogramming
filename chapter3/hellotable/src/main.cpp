#include "tinyAudioLib.h"
#include <cmath>
#include <iostream>

constexpr int SAMPLE_RATE = 44100;
constexpr size_t TABLE_LENGTH = 512;

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

auto main() -> int
{
    int frequency = 0;
    int duration = 0;

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

    default:
        std::cerr << "Error: couldn't decide lookup table fill method\n";
        exit(EXIT_FAILURE);
        break;
    }

    tinyInit();

    // DEBUG:
    std::cout << '\n' << wave << " " << frequency << " " << duration << '\n';

    // Generate sound
    const double sampleIncrement = frequency * TABLE_LENGTH / SAMPLE_RATE;
    double phase = 0;
    for (int i = 0; i < duration * SAMPLE_RATE; ++i)
    {
        auto sample = table[(size_t)phase];
        outSampleMono(sample);
        phase += sampleIncrement;
        if (phase > TABLE_LENGTH)
            phase -= TABLE_LENGTH;
        std::cout << "out " << i << '\n';
    }

    // Cleanup
    tinyExit();
    return 0;
}
