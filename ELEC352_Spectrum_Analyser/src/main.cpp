#include "stm32f4xx_hal.h"
#include "board.h"
#include "LedMatrix.h"
#include "VectorSignalGenerator.h"
#include "VectorDisplay.h"

int main()
{
    Board_Init();

    LedMatrix matrix;
    matrix.Initialise();

    VectorSignalGenerator generator;
    VectorDisplay display;

    float values[VectorDisplay::COLUMN_COUNT] = {};
    uint16_t pattern[VectorDisplay::ROW_COUNT] = {};

    generator.GenerateNext(values);

    if (display.Process(values, 16U, pattern, 8U))
    {
        matrix.LoadPattern(pattern);
    }

    const uint32_t DISPLAY_UPDATE_MS = 10U;
    uint32_t previousUpdate = HAL_GetTick();

    while (1)
    {
        // FUTURE: Service microphone acquisition here
        // on every loop pass.

        const uint32_t now = HAL_GetTick();

        if (static_cast<uint32_t>(now - previousUpdate) >= DISPLAY_UPDATE_MS)
        {
            // FUTURE: Use completed spectrum levels
            // instead of artificially generated values.
            generator.GenerateNext(values);

            if (display.Process(values, 16U, pattern, 8U))
            {
                matrix.LoadPattern(pattern);
            }

            previousUpdate = now;
        }

        matrix.Refresh();
    }
}
