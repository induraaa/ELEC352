// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "LedMatrixDemo.h"


bool LedMatrixDemo::BuildBox(uint16_t pattern[], size_t rowCount) const
{
    if (pattern == nullptr || rowCount != ROW_COUNT)
    {
        return false;
    }

    // Each 16-bit value represents one logical row. Bit 15 is the leftmost
    // column and bit 0 is the rightmost column.
    // 0xffff lights all 16 columns; 0x8001 lights only the two edge columns.
    const uint16_t boxPattern[ROW_COUNT] =
    {
        0xffff,
        0x8001,
        0x8001,
        0x8001,
        0x8001,
        0x8001,
        0x8001,
        0xffff
    };

    // Copy the image into the caller's plain data array. This function does
    // not display anything; main() passes the completed array to LedMatrix.
    for (uint8_t row = 0U; row < ROW_COUNT; row++)
    {
        pattern[row] = boxPattern[row];
    }
    return true;
}


bool LedMatrixDemo::BuildPixel(uint8_t row, uint8_t column,
                               uint16_t pattern[], size_t rowCount) const
{
    // Validate coordinates before shifting a bit or indexing the row array.
    if (pattern == nullptr || rowCount != ROW_COUNT ||
        row >= ROW_COUNT || column >= COLUMN_COUNT)
    {
        return false;
    }

    // Clear the previous image before selecting the next pixel.
    for (uint8_t clearRow = 0U; clearRow < ROW_COUNT; clearRow++)
    {
        pattern[clearRow] = 0x0000;
    }

    // In the logical image, bit 15 is the leftmost column and bit 0 is the
    // rightmost. Decreasing the bit position therefore moves left-to-right.
    pattern[row] =
        static_cast<uint16_t>(1U << (15U - column));
    return true;
}
