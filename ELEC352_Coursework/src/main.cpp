
#include "stm32f4xx_hal.h"
#include "board.h"
#include "LedMatrix.h"
#include "VectorDisplay.h"
#include "Microphone.h"
#include "SpectrumAnalyser.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

int main()
{
    Board_Init();

    LedMatrix matrix;
    matrix.Initialise();

    Microphone microphone;
    microphone.Initialise();

    if (!microphone.IsReady())
    {
        printf("Microphone setup failed\r\n");

        while (1)
        {
        }
    }

    if (!microphone.StartSampling(12800U))
    {
        printf("Sampling setup failed\r\n");

        while (1)
        {
        }
    }

    SpectrumAnalyser analyser;

    if (!analyser.Initialise(microphone.GetSampleRateHz()) ||
        !analyser.SetAutomaticGain(false, 100.0f) ||
        !analyser.SetDecibelScale(true, 30.0f))
    {
        printf("Spectrum analyser setup failed\r\n");

        while (1)
        {
        }
    }

    VectorDisplay display;

    constexpr std::size_t SAMPLE_COUNT =
        SpectrumAnalyser::SAMPLE_COUNT;

    constexpr std::size_t BAND_COUNT =
        SpectrumAnalyser::BAND_COUNT;

    float samples[SAMPLE_COUNT] = {};
    float levels[BAND_COUNT] = {};
    uint16_t pattern[VectorDisplay::ROW_COUNT] = {};

    while (1)
    {
        matrix.Refresh();

        if (microphone.Update(samples, SAMPLE_COUNT))
        {
            if (analyser.Process(samples, SAMPLE_COUNT,
                                 levels, BAND_COUNT))
            {
                if (display.Process(levels, BAND_COUNT,
                                    pattern, VectorDisplay::ROW_COUNT))
                {
                    matrix.LoadPattern(pattern);
                }
            }
        }
    }
}
