/// Small application to test applying various types of filters to a saw wave
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
    let mut wavetable = [0i16; WAVETABLE_LENGTH];
    fill_saw_table(&mut wavetable);

    let mut osc = Oscillator::create(200.0, &wavetable);

    let mut writer = hound::WavWriter::create(
        "out.wav",
        hound::WavSpec {
            bits_per_sample: 16,
            sample_rate: SAMPLE_RATE,
            channels: 1,
            sample_format: hound::SampleFormat::Int,
        },
    )
    .unwrap();

    let time = 5.0f32;
    let mut delay = 0;
    for _ in 0..((time * SAMPLE_RATE as f32) as usize) {
        let s = osc.tick(200.0);
        let mut buf = [s];
        // lowpass(&mut buf, 1000., &mut delay, SAMPLE_RATE);
        highpass(&mut buf, 1000., &mut delay, SAMPLE_RATE);
        writer.write_sample(buf[0]).unwrap();
    }
    writer
        .finalize()
        .expect("Failed to finalize output file headers");
}
