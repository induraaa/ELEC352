
#ifndef BLOCK_STATISTICS_H
#define BLOCK_STATISTICS_H

#include <cstddef>

class BlockStatistics
{
public:
    BlockStatistics();

    bool Process(const float samples[], std::size_t sampleCount);

    float GetMinimum() const;
    float GetMaximum() const;
    float GetMean() const;
    float GetPeakToPeak() const;

private:
    float minimum_;
    float maximum_;
    float mean_;
    float peakToPeak_;
};

#endif
