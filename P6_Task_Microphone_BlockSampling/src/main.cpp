
#include "stm32f4xx_hal.h"
#include "board.h"
#include "Microphone.h"
#include "BlockStatistics.h"

#include <cstdint>
#include <cstddef>
#include <cstdio>

int main()
{
    Board_Init();
    HAL_Delay(1000U);

    Microphone microphone;
    BlockStatistics statistics;

    constexpr std::size_t SAMPLE_COUNT = 128U;
    constexpr uint32_t REPORT_INTERVAL_MS = 250U;

    float samples[SAMPLE_COUNT] = {};

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

    uint32_t completedBlocks = 0U;
    uint32_t lastReport = HAL_GetTick();

    while (1)
    {
        if (microphone.Update(samples, SAMPLE_COUNT) &&
            statistics.Process(samples, SAMPLE_COUNT))
        {
            ++completedBlocks;

            const uint32_t now = HAL_GetTick();

            if (completedBlocks == 1U ||
                static_cast<uint32_t>(now - lastReport)
                    >= REPORT_INTERVAL_MS)
            {
                printf("Block %lu: min=%.2f max=%.2f mean=%.2f peak-to-peak=%.2f\r\n",
                       static_cast<unsigned long>(completedBlocks),
                       statistics.GetMinimum(),
                       statistics.GetMaximum(),
                       statistics.GetMean(),
                       statistics.GetPeakToPeak());

                lastReport = now;
            }
        }
    }
}
