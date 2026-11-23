
#include "VectorDisplay.h"

bool VectorDisplay::Process(const float values[],
                            std::size_t valueCount,
                            uint16_t pattern[],
                            std::size_t rowCount) const
{
    if (values == nullptr || pattern == nullptr ||
        valueCount != COLUMN_COUNT || rowCount != ROW_COUNT)
    {
        return false;
    }

    for (unsigned row = 0; row < ROW_COUNT; ++row)
    {
        pattern[row] = 0U;
    }

    for (unsigned column = 0; column < COLUMN_COUNT; ++column)
    {
        float a = values[column];

        if (!(a >= 0.0f))
            a = 0.0f;

        if (a > 1.0f)
            a = 1.0f;

        unsigned height = 1U + static_cast<unsigned>(7.0f * a + 0.5f);

        uint16_t mask = static_cast<uint16_t>(1U << (15U - column));

        for (unsigned row = 0; row < height; ++row)
        {
            pattern[7U - row] |= mask;
        }
    }

    return true;
}
