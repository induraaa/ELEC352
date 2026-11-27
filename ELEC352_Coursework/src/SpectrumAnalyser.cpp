// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "SpectrumAnalyser.h"

#include <math.h>

// Signal-processing sequence:
//   caller's sample block -> remove mean -> Hann window -> FFT -> band peaks
//   -> temporal smoothing -> noise-floor removal -> reference scaling -> display.
// Gain selection changes only the amplitude reference; the optional dB mapping
// changes only the final display scale. No acquisition or display hardware is
// accessed here, so the same implementation can be tested with synthetic data.

// Pi is used when calculating the Hann window and FFT twiddle factors.
//
// Keep it local to this source file because application code does not need it.
static const float PI = 3.14159265358979323846f;


SpectrumAnalyser::SpectrumAnalyser()
{
    // All working storage is fixed size. The caller owns the input/output data.
    for (uint16_t n = 0U; n < SAMPLE_COUNT; n++)
    {
        real_[n] = 0.0f;
        imaginary_[n] = 0.0f;
        window_[n] = 0.0f;
    }
    for (uint16_t k = 0U; k < SAMPLE_COUNT / 2U; k++)
    {
        twiddleReal_[k] = 0.0f;
        twiddleImaginary_[k] = 0.0f;
    }
    for (uint8_t band = 0U; band < BAND_COUNT; band++)
    {
        smoothedMagnitude_[band] = 0.0f;
    }
    windowSum_ = 0.0f;
    referenceLevel_ = MINIMUM_REFERENCE;
    automaticGainEnabled_ = true;
    fixedReferenceCounts_ = 100.0f;
    sampleRateHz_ = 0.0f;
    initialised_ = false;
    decibelScaleEnabled_ = false;
    dynamicRangeDb_ = 40.0f;
}


bool SpectrumAnalyser::SetAutomaticGain(bool enabled, float fixedReferenceCounts)
{
    // Validate first so a rejected setting cannot partly change the mode.
    if (!isfinite(fixedReferenceCounts) || fixedReferenceCounts <= 0.0f)
    {
        return false;
    }

    // Do not reuse an old loud-signal reference when returning to AGC. The
    // next processed frame will establish a reference for the current signal.
    // Smoothing is independent and is deliberately retained across the switch.
    if (enabled != automaticGainEnabled_)
    {
        referenceLevel_ = MINIMUM_REFERENCE;
    }
    automaticGainEnabled_ = enabled;
    fixedReferenceCounts_ = fixedReferenceCounts;
    return true;
}


bool SpectrumAnalyser::SetDecibelScale(bool enabled, float dynamicRangeDb)
{
    // A strictly positive range is required by the final division. Configure
    // only the mapping here; switching it must not alter the gain or FFT history.
    if (!isfinite(dynamicRangeDb) || dynamicRangeDb <= 0.0f)
    {
        return false;
    }
    decibelScaleEnabled_ = enabled;
    dynamicRangeDb_ = dynamicRangeDb;
    return true;
}


bool SpectrumAnalyser::Initialise(float sampleRateHz)
{
    initialised_ = false;
    sampleRateHz_ = 0.0f;
    if (!isfinite(sampleRateHz) || sampleRateHz <= 0.0f)
    {
        return false;
    }

    // Reset processing history while retaining the chosen gain and dB settings.
    sampleRateHz_ = sampleRateHz;
    InitialiseWindow();
    InitialiseTwiddles();
    referenceLevel_ = MINIMUM_REFERENCE;
    for (uint8_t band = 0U; band < BAND_COUNT; band++)
    {
        smoothedMagnitude_[band] = 0.0f;
    }
    initialised_ = true;
    return true;
}


void SpectrumAnalyser::InitialiseWindow()
{
    // The Hann window reduces spectral leakage when the captured frame
    // does not contain an exact integer number of waveform cycles.
    //
    // The coefficient is:
    //
    //                  1         2*pi*n
    //     w[n] = 0.5 - - cos(-------------)
    //                  2         N - 1
    //
    // for n = 0 ... N-1.
    windowSum_ = 0.0f;

    for (uint16_t n = 0U; n < SAMPLE_COUNT; n++)
    {
        const float phase =
            (2.0f * PI * static_cast<float>(n)) /
            static_cast<float>(SAMPLE_COUNT - 1U);

        window_[n] =
            0.5f - (0.5f * cosf(phase));

        // The sum is used later to compensate approximately for the amplitude
        // reduction caused by the window.
        windowSum_ += window_[n];
    }
}


void SpectrumAnalyser::InitialiseTwiddles()
{
    // Precalculate the complex rotations used by the FFT.
    //
    // For a 128-point FFT, only 64 distinct twiddle factors are required.
    for (uint16_t k = 0U; k < (SAMPLE_COUNT / 2U); k++)
    {
        const float angle =
            (-2.0f * PI * static_cast<float>(k)) /
            static_cast<float>(SAMPLE_COUNT);

        twiddleReal_[k] = cosf(angle);
        twiddleImaginary_[k] = sinf(angle);
    }
}


bool SpectrumAnalyser::Process(
    const float samples[], size_t sampleCount,
    float spectrum[], size_t bandCount
)
{
    if (!initialised_ || samples == nullptr || spectrum == nullptr ||
        sampleCount != SAMPLE_COUNT || bandCount != BAND_COUNT)
    {
        return false;
    }

    // Validate the complete frame before changing any persistent DSP state.
    for (uint16_t n = 0U; n < SAMPLE_COUNT; n++)
    {
        if (!isfinite(samples[n]))
        {
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // Find the mean ADC level across the frame.
    // -------------------------------------------------------------------------
    //
    // The microphone signal contains a DC bias so that an AC audio waveform can
    // remain inside the ADC's 0 to 3.3 V input range. That DC component is not
    // useful in the audio spectrum and is removed before the FFT.
    float mean = 0.0f;

    for (uint16_t n = 0U; n < SAMPLE_COUNT; n++)
    {
        mean += samples[n];
    }

    mean /= static_cast<float>(SAMPLE_COUNT);

    // -------------------------------------------------------------------------
    // Remove the DC bias and apply the Hann window.
    // -------------------------------------------------------------------------
    //
    // The microphone input is real-valued, so every imaginary input is zero.
    for (uint16_t n = 0U; n < SAMPLE_COUNT; n++)
    {
        real_[n] =
            (samples[n] - mean) * window_[n];

        imaginary_[n] = 0.0f;
    }

    // Transform the time-domain samples into frequency-domain values.
    PerformFft();

    // Group FFT bins into 16 normalised display levels.
    FormDisplayBands(spectrum);
    return true;
}


void SpectrumAnalyser::BitReverse()
{
    // -------------------------------------------------------------------------
    // Reorder the input into bit-reversed index order.
    // -------------------------------------------------------------------------
    //
    // The iterative radix-2 Cooley-Tukey FFT below expects this permutation.
    //
    // For an N = 128 transform, sample indices contain seven binary bits.
    // Bit reversal swaps positions such as:
    //
    //     binary 0000011  -> 1100000
    //
    // The real and imaginary arrays must be rearranged identically.
    uint16_t j = 0U;

    for (uint16_t i = 1U; i < SAMPLE_COUNT; i++)
    {
        uint16_t bit =
            static_cast<uint16_t>(SAMPLE_COUNT >> 1U);

        while ((j & bit) != 0U)
        {
            j ^= bit;
            bit >>= 1U;
        }

        j ^= bit;

        if (i < j)
        {
            const float realTemp = real_[i];
            real_[i] = real_[j];
            real_[j] = realTemp;

            const float imaginaryTemp = imaginary_[i];
            imaginary_[i] = imaginary_[j];
            imaginary_[j] = imaginaryTemp;
        }
    }
}


void SpectrumAnalyser::PerformFft()
{
    // Put samples into the order required by the iterative FFT.
    BitReverse();

    // -------------------------------------------------------------------------
    // Iterative radix-2 Cooley-Tukey FFT.
    // -------------------------------------------------------------------------
    //
    // The transform is built from stages of length:
    //
    //     2, 4, 8, 16, 32, 64, 128
    //
    // Each butterfly combines an "even" and "odd" complex value using one
    // precalculated complex twiddle factor.
    //
    // Because all sine/cosine values were calculated in InitialiseTwiddles(),
    // there is no trigonometry inside this real-time FFT loop.
    for (uint16_t length = 2U;
         length <= SAMPLE_COUNT;
         length <<= 1U)
    {
        const uint16_t halfLength =
            static_cast<uint16_t>(length >> 1U);

        const uint16_t twiddleStep =
            static_cast<uint16_t>(SAMPLE_COUNT / length);

        for (uint16_t block = 0U;
             block < SAMPLE_COUNT;
             block =
                 static_cast<uint16_t>(block + length))
        {
            for (uint16_t j = 0U;
                 j < halfLength;
                 j++)
            {
                const uint16_t twiddleIndex =
                    static_cast<uint16_t>(j * twiddleStep);

                const float wr =
                    twiddleReal_[twiddleIndex];

                const float wi =
                    twiddleImaginary_[twiddleIndex];

                const uint16_t even =
                    static_cast<uint16_t>(block + j);

                const uint16_t odd =
                    static_cast<uint16_t>(
                        even + halfLength
                    );

                // Multiply the odd complex value by the twiddle factor:
                //
                //     (wr + j*wi) * (real + j*imaginary)
                const float tr =
                    (wr * real_[odd]) -
                    (wi * imaginary_[odd]);

                const float ti =
                    (wr * imaginary_[odd]) +
                    (wi * real_[odd]);

                // Save the even value before overwriting it.
                const float evenReal =
                    real_[even];

                const float evenImaginary =
                    imaginary_[even];

                // FFT butterfly:
                //
                //     even output = even input + rotated odd input
                //     odd output  = even input - rotated odd input
                real_[even] =
                    evenReal + tr;

                imaginary_[even] =
                    evenImaginary + ti;

                real_[odd] =
                    evenReal - tr;

                imaginary_[odd] =
                    evenImaginary - ti;
            }
        }
    }
}


void SpectrumAnalyser::FormDisplayBands(
    float spectrum[BAND_COUNT]
)
{
    // Temporary magnitude for each of the 16 display bands.
    float bandMagnitude[BAND_COUNT];

    // -------------------------------------------------------------------------
    // Approximate FFT amplitude correction for the Hann window.
    // -------------------------------------------------------------------------
    //
    // For a Hann-windowed sinusoid, multiplying the complex FFT magnitude by:
    //
    //     2 / sum(window)
    //
    // approximately recovers the original sinusoidal amplitude in ADC counts.
    //
    // Exact metrological amplitude is not required here; consistent relative
    // scaling between frequency bands is the important property.
    const float amplitudeScale =
        2.0f / windowSum_;

    float framePeak = 0.0f;

    // -------------------------------------------------------------------------
    // Convert FFT bins into 16 frequency bands.
    // -------------------------------------------------------------------------
    //
    // Bin 0 is the DC component and is deliberately skipped.
    //
    // The FFT resolution is:
    //
    //     sampleRateHz_ / 128 (100 Hz at the nominal 12.8 kHz rate)
    //
    // Four bins are combined per display band. At 12.8 kHz:
    //
    //     Band 0  -> bins  1 ...  4 -> approximately 100 ... 400 Hz
    //     Band 1  -> bins  5 ...  8 -> approximately 500 ... 800 Hz
    //     ...
    //     Band 15 -> bins 61 ... 64 -> approximately 6.1 ... 6.4 kHz
    //
    // For each band, the largest magnitude is used as the displayed band level.
    for (uint8_t band = 0U;
         band < BAND_COUNT;
         band++)
    {
        const uint16_t firstBin =
            static_cast<uint16_t>(
                1U +
                (static_cast<uint16_t>(band) *
                 BINS_PER_BAND)
            );

        const uint16_t lastBin =
            static_cast<uint16_t>(
                firstBin + BINS_PER_BAND - 1U
            );

        float maximumMagnitude = 0.0f;

        for (uint16_t bin = firstBin;
             bin <= lastBin;
             bin++)
        {
            const float re = real_[bin];
            const float im = imaginary_[bin];

            // Magnitude of the complex FFT value:
            //
            //     sqrt(real^2 + imaginary^2)
            const float magnitude =
                sqrtf(
                    (re * re) +
                    (im * im)
                ) * amplitudeScale;

            if (magnitude > maximumMagnitude)
            {
                maximumMagnitude = magnitude;
            }
        }

        // ---------------------------------------------------------------------
        // Light temporal smoothing.
        // ---------------------------------------------------------------------
        //
        // Keep 60% of the previous value and add 40% of the new value.
        // This reduces single-frame flicker while remaining responsive enough
        // for an audio spectrum display.
        smoothedMagnitude_[band] =
            ((1.0f - SMOOTHING_NEW_WEIGHT) *
             smoothedMagnitude_[band]) +
            (SMOOTHING_NEW_WEIGHT *
             maximumMagnitude);

        bandMagnitude[band] =
            smoothedMagnitude_[band];

        // Track the strongest band in this frame for automatic display scaling.
        if (bandMagnitude[band] > framePeak)
        {
            framePeak = bandMagnitude[band];
        }
    }

    // -------------------------------------------------------------------------
    // Automatic display gain: fast attack, slow release.
    // -------------------------------------------------------------------------
    //
    // A new stronger signal raises the reference immediately.
    //
    // When the signal becomes weaker, the reference decays slowly so the
    // display does not constantly rescale from one frame to the next.
    //
    // MINIMUM_REFERENCE prevents low-level ADC noise from being amplified until
    // it fills the LED matrix. None of this reference adaptation runs in fixed
    // gain mode; changes in signal amplitude then remain visible over time.
    if (automaticGainEnabled_)
    {
        if (framePeak > referenceLevel_)
        {
            referenceLevel_ = framePeak;
        }
        else
        {
            referenceLevel_ *= REFERENCE_DECAY;

            if (referenceLevel_ < MINIMUM_REFERENCE)
            {
                referenceLevel_ = MINIMUM_REFERENCE;
            }
        }
    }

    // The same selected reference is used for every frequency band in a frame,
    // with either display mapping. A fixed reference also stays constant between
    // frames. Its positive value has already been checked by SetAutomaticGain().
    const float displayReference = automaticGainEnabled_
        ? referenceLevel_ : fixedReferenceCounts_;

    // -------------------------------------------------------------------------
    // Convert each band into the final 0.0 ... 1.0 display value.
    // -------------------------------------------------------------------------
    for (uint8_t band = 0U;
         band < BAND_COUNT;
         band++)
    {
        // Remove a small fixed noise floor.
        float magnitude =
            bandMagnitude[band] -
            NOISE_FLOOR;

        if (magnitude < 0.0f)
        {
            magnitude = 0.0f;
        }

        // Form an amplitude ratio using the selected fixed or automatic gain.
        float level =
            magnitude / displayReference;

        // Keep the value within the range expected by VectorDisplay.
        if (level > 1.0f)
        {
            level = 1.0f;
        }

        if (decibelScaleEnabled_)
        {
            // Amplitude ratios use 20*log10(), not the power-ratio factor 10.
            // Do not take log10(0). Silence and values below the selected range
            // produce zero, which VectorDisplay draws as the baseline LED.
            float displayLevel = 0.0f;
            if (level > 0.0f)
            {
                const float decibels = 20.0f * log10f(level);
                if (decibels > -dynamicRangeDb_)
                {
                    displayLevel = 1.0f + decibels / dynamicRangeDb_;
                }
            }
            spectrum[band] = displayLevel;
        }
        else
        {
            // Keep the original display mapping when dB mode is disabled.
            // Square-root compression makes weaker components more visible.
            spectrum[band] = sqrtf(level);
        }
    }
}


float SpectrumAnalyser::GetFrequencyResolutionHz() const
{
    // FFT bin spacing is sample rate divided by transform length.
    return
        sampleRateHz_ /
        static_cast<float>(SAMPLE_COUNT);
}
