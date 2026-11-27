// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#ifndef SPECTRUM_ANALYSER_H
#define SPECTRUM_ANALYSER_H

#include <stddef.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
// SpectrumAnalyser
// -----------------------------------------------------------------------------
// Performs a 128-point FFT on a supplied array of time-domain samples and
// produces 16 normalised frequency-band levels for the LED matrix.
//
// The class deliberately uses ordinary C-style arrays only. It does not use
// std::array, STL containers, templates, or dynamic memory.
//
// Typical sampling arrangement, selected by the application:
//
//     Sample rate         = 12.8 kHz
//     Samples per frame   = 128
//     Nominal frame time  = 10 ms
//     FFT resolution      = 100 Hz
//     Display bands       = 16
//     FFT bins per band   = 4
//     Display range       = approximately 100 Hz to 6.4 kHz
//
// The application supplies two simple float arrays:
//
//     float samples[128];
//     float spectrum[16];
//
// After filling samples[] from a microphone, a file or a test-signal generator:
//
//     analyser.Process(samples, 128, spectrum, 16);
//
// Each output value is normalised to the range 0.0 to 1.0 so it can be passed
// directly to VectorDisplay. These are display levels, not calibrated power
// or energy measurements.
//
// This class contains no microphone object, ADC code, sample scheduling or
// STM32 headers. The sample rate describes the data supplied by the caller;
// it does not cause this object to take samples. Hardware timing belongs to
// Microphone, while main() connects the objects using ordinary data arrays.
//
// Input values use ADC-count units to preserve the existing noise floor/gain.
// The caller owns both buffers, and the analyser does not retain their addresses.
// -----------------------------------------------------------------------------

class SpectrumAnalyser
{
public:
    // Number of samples used by each FFT. 128 is a power of two, which is
    // required by this radix-2 FFT.
    static constexpr uint16_t SAMPLE_COUNT = 128U;

    // Number of output frequency bands.
    static constexpr uint8_t BAND_COUNT = 16U;

    // Prepare internal storage only. No hardware or other object is required.
    // The class defaults to the original AGC behaviour for existing callers;
    // this project's main() explicitly selects fixed gain instead.
    SpectrumAnalyser();

    // Prepare the Hann window, FFT factors and smoothing state. sampleRateHz
    // describes the supplied data; this class does not schedule acquisition.
    // A non-finite or non-positive rate returns false and disables processing.
    bool Initialise(float sampleRateHz);

    // Choose the amplitude reference independently of the display scale.
    // enabled=true: track strong bands with fast attack and slow release (AGC).
    // enabled=false: use fixedReferenceCounts for every frame. Increasing the
    // fixed reference reduces display sensitivity; reducing it raises bars.
    // The reference is an FFT amplitude in ADC-count units, not a raw ADC bias,
    // peak-to-peak value or calibrated sound-pressure level.
    // Both modes retain the same temporal smoothing and noise-floor removal.
    // The fixed value must be finite and positive, even when enabling AGC.
    // Invalid settings return false without changing the configuration.
    // Settings survive Initialise(). Switching gain modes keeps smoothing but
    // resets the AGC reference to its minimum, ready for the next frame.
    bool SetAutomaticGain(bool enabled, float fixedReferenceCounts = 100.0f);

    // Select the amplitude-to-display mapping. Default: disabled, preserving
    // the original square-root compression. Enabled: use 20*log10(A/reference)
    // and map -dynamicRangeDb ... 0 dB onto display levels 0 ... 1.
    // A is the smoothed band amplitude after the existing noise-floor removal;
    // reference is either fixed or automatic, selected by SetAutomaticGain().
    // Neither mode measures acoustic SPL or ADC full-scale dB. Zero/below-range
    // amplitudes give the one-LED baseline; above-reference values clip at 0 dB.
    // The range must be finite and positive, even when disabling the mode.
    // Invalid settings return false and leave the previous settings unchanged.
    // Settings survive Initialise(); changing mode does not reset smoothing.
    bool SetDecibelScale(bool enabled, float dynamicRangeDb = 40.0f);

    // Process one complete frame synchronously. samples[] is read-only.
    //
    // The processing stages remain the same as in the original analyser:
    //     1. remove the DC microphone bias by subtracting the block mean;
    //     2. apply a Hann window;
    //     3. calculate a 128-point FFT;
    //     4. group the positive-frequency FFT bins into 16 bands;
    //     5. smooth and apply the selected amplitude scale to give 0.0 ... 1.0;
    //     6. fill the supplied spectrum array and return true.
    //
    // Counts must be exactly SAMPLE_COUNT and BAND_COUNT. Supply valid,
    // non-overlapping arrays with those many elements. All samples must be
    // finite and on an ADC-count-like scale (normally 0 ... 4095).
    // Null pointers, wrong counts, non-finite input or use before Initialise
    // return false and leave spectrum[] and the smoothing state unchanged.
    // C++ pointers cannot reveal the real allocation size: counts must be honest.
    bool Process(const float samples[], size_t sampleCount,
                 float spectrum[], size_t bandCount);

    // Bin spacing = supplied sample rate / 128; zero before Initialise.
    float GetFrequencyResolutionHz() const;

private:
    static constexpr uint8_t BINS_PER_BAND = 4U;

    // Display-conditioning constants, retained from the original analyser.
    //
    // NOISE_FLOOR:
    //     Small magnitudes below this level are removed.
    //
    // MINIMUM_REFERENCE:
    //     Prevents quiet ADC noise from being automatically amplified to fill
    //     the display.
    //
    // REFERENCE_DECAY:
    //     Controls the slow release of the automatic display gain.
    //
    // SMOOTHING_NEW_WEIGHT:
    //     Controls how strongly each new frame changes the displayed magnitude.
    //
    // The first two thresholds are in ADC-count units.
    static constexpr float NOISE_FLOOR = 1.5f;
    static constexpr float MINIMUM_REFERENCE = 30.0f;
    static constexpr float REFERENCE_DECAY = 0.995f;
    static constexpr float SMOOTHING_NEW_WEIGHT = 0.40f;

    // Precalculate the 128 Hann-window coefficients.
    void InitialiseWindow();

    // Precalculate the 64 complex FFT twiddle factors.
    void InitialiseTwiddles();

    // Perform the in-place 128-point radix-2 FFT.
    void PerformFft();

    // Reorder the FFT input into bit-reversed index order.
    void BitReverse();

    // Convert the positive-frequency FFT result into the 16 displayed bands.
    void FormDisplayBands(float spectrum[BAND_COUNT]);

    // -------------------------------------------------------------------------
    // Fixed-size signal-processing arrays.
    // -------------------------------------------------------------------------
    // real_ and imaginary_:
    //     Working arrays used by the complex FFT.
    //
    // window_:
    //     Hann-window coefficient for each sample.
    //
    // The old private samples_ buffer is now the caller's samples[] array.
    // Only private working memory and smoothing state remain in this object.
    float real_[SAMPLE_COUNT];
    float imaginary_[SAMPLE_COUNT];
    float window_[SAMPLE_COUNT];
    // A radix-2 FFT of length N requires only N/2 distinct twiddle factors.
    float twiddleReal_[SAMPLE_COUNT / 2U];
    float twiddleImaginary_[SAMPLE_COUNT / 2U];
    // Previous smoothed magnitude for each of the 16 display bands.
    float smoothedMagnitude_[BAND_COUNT];
    // Sum of the Hann-window coefficients, used for FFT amplitude scaling.
    float windowSum_;

    // Slowly decaying automatic display-gain reference.
    float referenceLevel_;

    // Gain selection is independent of logarithmic/square-root display mapping.
    // The fixed reference never follows the incoming signal's amplitude.
    bool automaticGainEnabled_;
    float fixedReferenceCounts_;

    // Numeric rate of the supplied data; used to calculate bin frequencies.
    float sampleRateHz_;

    // True after Initialise() has prepared the analyser.
    bool initialised_;

    // Display mapping only; FFT, sampling and gain selection are independent.
    bool decibelScaleEnabled_;
    float dynamicRangeDb_;
};

#endif  // SPECTRUM_ANALYSER_H
