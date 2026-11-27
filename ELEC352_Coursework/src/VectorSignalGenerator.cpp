
#include "VectorSignalGenerator.h"
#include <cmath>

void VectorSignalGenerator::GenerateNext(float values[])
{
    const float pi = 3.14159265f;
    const float t = 0.04f * static_cast<float>(frameNumber_);

    for (unsigned column = 0; column < VECTOR_SIZE; column++)
    {
        const float x = static_cast<float>(column) / 15.0f;

        values[column] =
            0.5f * (std::sin(2.0f * pi * x - t) + 1.0f);
    }

    ++frameNumber_;
}
