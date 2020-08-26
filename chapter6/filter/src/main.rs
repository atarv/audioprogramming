//! Small application to test applying various types of filters to a saw wave

use clap::{App, Arg};
use hound;
use std::f32::consts::PI;

const SAMPLE_RATE: u32 = 44100; // TODO: Let user choose sample rate
const WAVETABLE_LENGTH: usize = 128; // TODO: Maybe let user choose wavetable size too

/// Convert 32bit floating point sample to 16bit integer sample
#[inline]
fn float_sample_to_i16(sample: f32) -> i16 {
    if sample > 0.0 {
        (sample * std::i16::MAX as f32) as i16
    } else {
        (sample * -(std::i16::MIN as f32)) as i16
    }
}

/// Fill wavetable with saw waveform
fn fill_saw_table(table: &mut [i16]) {
    for i in 0..table.len() {
        let s = 1. - (2. * i as f32 / (table.len() as f32));
        table[i] = float_sample_to_i16(s);
    }
}

/// Table lookup oscillator
struct Oscillator<'a> {
    /// Current phase of oscillator
    phase: f32,
    /// Current frequency (Hz)
    frequency: f32,
    /// Phase increment per tick
    phase_increment: f32,
    /// Size of wavetable over sample rate
    size_over_srate: f32,
    wavetable: &'a [i16],
}

impl<'a> Oscillator<'a> {
    fn create(frequency: f32, wavetable: &'a [i16]) -> Oscillator {
        let size_over_srate = wavetable.len() as f32 / SAMPLE_RATE as f32;
        Oscillator {
            phase: 0.,
            frequency,
            size_over_srate,
            phase_increment: size_over_srate * frequency,
            wavetable,
        }
    }

    /// Tick oscillator for one sample
    fn tick(&mut self, freq: f32) -> i16 {
        let index = self.phase as usize;
        let mut current_phase = self.phase;
        // Update oscillators frequency
        if self.frequency != freq {
            self.frequency = freq;
            self.phase_increment = self.size_over_srate * self.frequency;
        }
        // Advance oscillator phase
        current_phase += self.phase_increment;
        while current_phase >= self.wavetable.len() as f32 {
            current_phase -= self.wavetable.len() as f32;
        }
        while current_phase < 0. {
            current_phase += self.wavetable.len() as f32;
        }
        self.phase = current_phase;
        self.wavetable[index]
    }
}

/// Simple first order lowpass filter
fn lowpass<'a>(
    signal: &'a mut [i16],
    cutoff_freq: f32,
    delay: &mut i16,
    sample_rate: u32,
) -> &'a mut [i16] {
    let cos_theta = 2. - f32::cos(2. * PI * cutoff_freq / sample_rate as f32);
    let coefficient = (cos_theta.powf(2.) - 1.).sqrt() - cos_theta;
    for sample in signal.iter_mut() {
        *sample = (*sample as f32 * (1. + coefficient) - (*delay as f32) * coefficient) as i16;
        *delay = *sample;
    }
    signal
}

/// Simple first order highpass filter
fn highpass<'a>(
    signal: &'a mut [i16],
    cutoff_freq: f32,
    delay: &mut i16,
    sample_rate: u32,
) -> &'a mut [i16] {
    let cos_theta = 2. - f32::cos(2. * PI * cutoff_freq / sample_rate as f32);
    let coefficient = cos_theta - (cos_theta.powf(2.) - 1.).sqrt();
    for sample in signal.iter_mut() {
        *sample = (*sample as f32 * (1. - coefficient) - (*delay as f32) * coefficient) as i16;
        *delay = *sample;
    }
    signal
}

fn main() {
    // TODO: Get command line arguments
    let matches = App::new("Filter experiment")
        .author("Aleksi Tarvainen")
        .about("Small program to experiment with filters")
        .arg(
            Arg::with_name("OUTPUT")
                .takes_value(true)
                .required(true)
                .help("Path of output file"),
        )
        .arg(
            Arg::with_name("type")
                .short("f")
                .long("filter")
                .takes_value(true)
                .required(true)
                .possible_values(&["lpf", "hpf", "none"])
                .help("Set filter type"),
        )
        .arg(
            Arg::with_name("frequency")
                .takes_value(true)
                .required(true)
                .short("z")
                .long("frequency")
                .help("Oscillator frequency in Hz"),
        )
        .arg(
            Arg::with_name("cutoff")
                .takes_value(true)
                .required(true)
                .short("c")
                .long("cutoff")
                .help("Cutoff frequency of filter"),
        )
        .arg(
            Arg::with_name("duration")
                .takes_value(true)
                .required(true)
                .short("d")
                .long("duration")
                .help("Duration of tone in seconds"),
        )
        .get_matches();

    let mut wavetable = [0i16; WAVETABLE_LENGTH];
    fill_saw_table(&mut wavetable);

    let freq = matches
        .value_of("frequency")
        .unwrap()
        .parse::<f32>()
        .unwrap();
    let mut osc = Oscillator::create(freq, &wavetable);

    let mut writer = hound::WavWriter::create(
        matches.value_of("OUTPUT").unwrap(),
        hound::WavSpec {
            bits_per_sample: 16,
            sample_rate: SAMPLE_RATE,
            channels: 1,
            sample_format: hound::SampleFormat::Int,
        },
    )
    .unwrap();

    let duration = matches
        .value_of("duration")
        .unwrap()
        .parse::<f32>()
        .unwrap();
    let mut delay = 0;
    let cutoff_freq = matches.value_of("cutoff").unwrap().parse::<f32>().unwrap();
    for _ in 0..((duration * SAMPLE_RATE as f32) as usize) {
        let s = osc.tick(freq);
        let mut buf = [s];
        match matches.value_of("type").unwrap() {
            "lpf" => {
                lowpass(&mut buf, cutoff_freq, &mut delay, SAMPLE_RATE);
                ()
            }
            "hpf" => {
                highpass(&mut buf, cutoff_freq, &mut delay, SAMPLE_RATE);
                ()
            }
            "none" => (),
            _ => panic!("No filter type provided. This shouldn't happen."),
        }
        writer.write_sample(buf[0]).unwrap();
    }
    writer
        .finalize()
        .expect("Failed to finalize output file headers");
}
