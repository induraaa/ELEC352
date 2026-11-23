
#ifndef VECTOR_DISPLAY_H
#define VECTOR_DISPLAY_H

#include <cstddef>
#include <cstdint>

class VectorDisplay
{
public:
    static constexpr unsigned COLUMN_COUNT = 16U;
    static constexpr unsigned ROW_COUNT = 8U;

    bool Process(const float values[],
                 std::size_t valueCount,
                 uint16_t pattern[],
                 std::size_t rowCount) const;
};

#endif
