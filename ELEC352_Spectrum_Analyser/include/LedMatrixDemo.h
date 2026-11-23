// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#ifndef LED_MATRIX_DEMO_H
#define LED_MATRIX_DEMO_H

#include <stddef.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
// LedMatrixDemo
// -----------------------------------------------------------------------------
// Defines example images without knowing how the physical matrix is wired.
// LedMatrix remains responsible for the hardware. main() controls the animation
// sequence and timing, passing a plain eight-row bitmap between the objects.
//
// Each uint16_t stores one logical row: row 0 is the top, bit 15 is the leftmost
// column and bit 0 is the rightmost column. The caller owns the array.
// No LedMatrix object, STM32 header, timer or dynamic allocation is needed here.
// -----------------------------------------------------------------------------
class LedMatrixDemo
{
public:
    static constexpr uint8_t ROW_COUNT = 8U;
    static constexpr uint8_t COLUMN_COUNT = 16U;

    // No hardware or animation state is held by this image generator.
    LedMatrixDemo() = default;

    // Generate a rectangular border in the supplied eight-row array.
    // main() decides how long to display it using matrix.ShowPattern().
    bool BuildBox(uint16_t pattern[], size_t rowCount) const;

    // Generate one illuminated pixel at the supplied logical row and column.
    // main() can move it left-to-right, starting on the bottom row and proceeding
    // upwards, by repeatedly generating and displaying one image at a time.
    bool BuildPixel(uint8_t row, uint8_t column,
                    uint16_t pattern[], size_t rowCount) const;

    // Both methods require a valid array of eight uint16_t elements and
    // rowCount == ROW_COUNT. Null pointers, wrong lengths or out-of-range pixel
    // coordinates return false without modifying the output array. Counts must
    // accurately describe allocated storage; a pointer carries no size itself.
};

#endif  // LED_MATRIX_DEMO_H
