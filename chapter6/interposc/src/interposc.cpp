/*
 *  Interpolating or truncating table lookup oscillator
 */
#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <sndfile.h>
#include <vector>

constexpr int SAMPLE_RATE = 44100;
constexpr int WAVETABLE_SIZE = 1024;
constexpr int BUF_SIZE = 512;
constexpr auto PI = M_PI;

// TODO: Add usage info
// TODO: split classes to multiple files

enum class Waveform
{
    Sine,
    Saw,
    Square,
    Triangle
};

template <typename Iter> void normalize(Iter begin, Iter end)
{
    auto peak = *std::max_element(begin, end);
    auto scale = [peak](auto sample) { return sample * (1.0 / peak); };
    std::transform(begin, end, begin, scale);
}

auto fourierTable(const std::vector<float> &harmonicAmplitudes,
                  float phaseOffset)
{
    std::array<float, WAVETABLE_SIZE + 2> table;
    table.fill(0.f);
    phaseOffset *= (float)PI * 2;

    for (auto i = 0ul; i < harmonicAmplitudes.size(); ++i)
        for (auto n = 0ul; n < WAVETABLE_SIZE + 2; ++n)
        {
            auto a = harmonicAmplitudes.at(i);
            auto w = (i + 1) * (n * 2 * PI / WAVETABLE_SIZE);
            table[n] += (float)(a * cos(w + phaseOffset));
        }
    normalize(table.begin(), table.end());
    return table;
}

class Wavetable
{
    std::array<float, WAVETABLE_SIZE + 2> table;

  public:
    Wavetable(Waveform waveform, int harmonics)
    {
        switch (waveform)
        {
        case Waveform::Sine:
        {
            float phase = 0;
            const auto incr = (float)2 * PI / WAVETABLE_SIZE;
            for (int i = 0; i < WAVETABLE_SIZE + 2; ++i)
            {
                table[i] = sin(phase);
                phase += incr;
            }
            break;
        }
        case Waveform::Saw:
        {
            std::vector<float> harmAmps(harmonics, 0.f);
            for (int i = 0; i < harmonics; ++i)
                harmAmps[i] = 1.f / (i + 1);
            table = fourierTable(harmAmps, -0.25);
            break;
        }
        case Waveform::Square:
        {
            std::vector<float> harmAmps(harmonics, 0.f);
            for (int i = 0; i < harmonics; i += 2)
                harmAmps[i] = 1.f / (i + 1);
            table = fourierTable(harmAmps, -0.25);
            break;
        }
        case Waveform::Triangle:
        {
            std::vector<float> harmAmps(harmonics, 0.f);
            for (int i = 0; i < harmonics; i += 2)
                harmAmps[i] = 1.f / ((i + 1) * (i + 1));
            table = fourierTable(harmAmps, 0.0);
        }
        }
    }
    float operator[](const std::size_t idx) const { return table[idx]; }
};

class Oscillator
{
    const Wavetable &wave; // Wavetable to sample from
    float phase{0.0};      // Current phase of oscillator
    const int sampleRate;

  public:
    float amplitude{1.0}, freq;
    Oscillator(float amp, const Wavetable &w, float freq,
               int srate = SAMPLE_RATE)
        : wave(w), sampleRate(srate), amplitude(amp), freq(freq)
    {
    }

    /**
     * Fill given range with truncated lookup from oscillators wavetable
     */
    template <typename Iter> void fillTruncated(Iter begin, Iter end)
    {
        const auto increment = freq * WAVETABLE_SIZE / SAMPLE_RATE;
        for (auto i = begin; i != end; ++i)
        {
            *i = amplitude * wave[(int)phase];
            phase += increment;
            // Modulus for float value
            while (phase >= WAVETABLE_SIZE)
                phase -= WAVETABLE_SIZE;
            while (phase < 0)
                phase += WAVETABLE_SIZE;
        }
    }

    /**
     * Fill given range with linear interpolation from oscillators wavetable
     */
    template <typename Iter> void fillLinearInterpolation(Iter begin, Iter end)
    {
        const auto increment = freq * WAVETABLE_SIZE / SAMPLE_RATE;
        for (auto i = begin; i != end; ++i)
        {
            *i = amplitude *
                 (wave[(int)phase] + (wave[(int)phase + 1] - wave[(int)phase]) *
                                         (phase - (int)phase));
            phase += increment;
            // Modulus for float value
            while (phase >= WAVETABLE_SIZE)
                phase -= WAVETABLE_SIZE;
            while (phase < 0)
                phase += WAVETABLE_SIZE;
        }
    }
    // TODO: Cubic interpolation
};

auto main(int argc, char const *argv[]) -> int
{
    std::vector<float> outBuf(BUF_SIZE, 0.f);
    auto outFileName = "./out.wav";
    bool truncating = false;
    auto waveform = Waveform::Sine;
    float amplitude = 1.f;

    // Parse arguments

    int c;
    while ((c = getopt(argc, (char *const *)argv, "a:tw:")) != -1)
        switch (c)
        {
        case 'a':
        {

            float a = strtod(optarg, nullptr);
            if (isnanf(a) || a > 1 || a < 0)
            {
                std::cerr << "Error: amplitude must be a decimal number "
                             "between 0 and 1 (inclusive)\n";
                return EXIT_FAILURE;
            }
            amplitude = a;
            break;
        }
        case 'w':
        {
            int w = strtol(optarg, nullptr, 10);
            switch (w)
            {
            case 0:
                waveform = Waveform::Sine;
                break;
            case 1:
                waveform = Waveform::Saw;
                break;
            case 2:
                waveform = Waveform::Square;
                break;
            case 3:
                waveform = Waveform::Triangle;
                break;

            default:
                std::cerr << "Error: invalid waveform " << w << '\n';
                return EXIT_FAILURE;
                break;
            }
            break;
        }
        case 't':
        {
            truncating = true;
            break;
        }
        case '?':
            if (optopt == 'c')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            return 1;
        default:
            abort();
        }

    for (auto i = optind; i < argc; ++i) // DEBUG:
        std::cout << argv[i] << "\n";

    if (argc - optind != 3) // Check argument count
    {
        std::cerr << "Error: invalid number of arguments\n";
        return EXIT_FAILURE;
    }

    // Required (non-option) arguments

    outFileName = argv[optind++];

    float frequency = strtod(argv[optind++], nullptr);
    if (isnanf(frequency) || frequency < 0)
    {
        std::cerr << "Error: frequency must be positive\n";
        return EXIT_FAILURE;
    }

    int harmonics = strtol(argv[optind++], nullptr, 10);
    if (errno == ERANGE)
    {
        std::cerr << "Error: harmonics must be an integer\n";
        return EXIT_FAILURE;
    }

    // Initialize libsndfile and open file for output

    SF_INFO info = {};
    info.samplerate = SAMPLE_RATE;
    info.channels = 1;
    info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    auto outFile = sf_open(outFileName, SFM_WRITE, &info);
    if (outFile == nullptr)
    {
        std::cerr << "Error: failed to open output file."
                  << sf_strerror(outFile) << "\n";
        return EXIT_FAILURE;
    }

    // Initialize oscillator

    auto wavtab = Wavetable{waveform, harmonics};
    auto osc = Oscillator{amplitude, wavtab, frequency};

    // Process

    auto framesWritten = 0;
    for (int i = 0; i < 1000; ++i)
    {
        if (truncating)
            osc.fillTruncated(outBuf.begin(), outBuf.end());
        else
            osc.fillLinearInterpolation(outBuf.begin(), outBuf.end());
        framesWritten += sf_write_float(outFile, outBuf.data(), outBuf.size());
    }
    std::cout << framesWritten << "frames written to " << outFileName << "\n";

    // Cleanup

    sf_close(outFile);
    return EXIT_SUCCESS;
}
