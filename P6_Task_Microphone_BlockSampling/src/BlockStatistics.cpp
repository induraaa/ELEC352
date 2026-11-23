
#include "BlockStatistics.h"
#include <cmath>

BlockStatistics::BlockStatistics()
    : minimum_(0.0f),
      maximum_(0.0f),
      mean_(0.0f),
      peakToPeak_(0.0f)
{
}

bool BlockStatistics::Process(const float samples[],
                              std::size_t sampleCount)
{
    if (samples == nullptr || sampleCount == 0U)
        return false;

    if (!std::isfinite(samples[0]))
        return false;

    float minimum = samples[0];
    float maximum = samples[0];
    float sum = 0.0f;

    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        const float value = samples[i];

        if (!std::isfinite(value))
            return false;

        if (value < minimum)
            minimum = value;

        if (value > maximum)
            maximum = value;

        sum += value;
    }

    const float mean = sum / static_cast<float>(sampleCount);
    const float peakToPeak = maximum - minimum;

    if (!std::isfinite(sum) || !std::isfinite(mean) ||
        !std::isfinite(peakToPeak))
        return false;

    minimum_ = minimum;
    maximum_ = maximum;
    mean_ = mean;
    peakToPeak_ = peakToPeak;

    return true;
}

float BlockStatistics::GetMinimum() const
{
    return minimum_;
}

float BlockStatistics::GetMaximum() const
{
    return maximum_;
}

float BlockStatistics::GetMean() const
{
    return mean_;
}

float BlockStatistics::GetPeakToPeak() const
{
    return peakToPeak_;
}
