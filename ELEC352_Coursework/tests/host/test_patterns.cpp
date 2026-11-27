// Host-only checks: compile with LedMatrixDemo.cpp, without STM32 headers.
#include "LedMatrixDemo.h"
#include <cassert>
#include <cstdio>

int main()
{
    LedMatrixDemo demo;
    // Sentinel words detect writes beyond the caller's eight-row array.
    uint16_t storage[10] = {};
    storage[0] = 0x1234U;
    storage[9] = 0xabcdU;
    uint16_t* pattern = storage + 1;
    assert(demo.BuildBox(pattern, 8));
    for (unsigned row = 0; row < 8; ++row)
        assert(pattern[row] == ((row == 0 || row == 7) ? 0xffffU : 0x8001U));

    // The original scan starts at bottom-left, ends at top-right, and lights
    // exactly one pixel on each of its 128 frames.
    unsigned frames = 0;
    for (int row = 7; row >= 0; --row)
        for (uint8_t column = 0; column < 16; ++column)
        {
            assert(demo.BuildPixel(static_cast<uint8_t>(row), column, pattern, 8));
            for (int r = 0; r < 8; ++r)
                assert(pattern[r] == (r == row ? (1U << (15 - column)) : 0U));
            assert(storage[0] == 0x1234U && storage[9] == 0xabcdU);
            ++frames;
        }
    assert(frames == 128);
    for (unsigned row = 0; row < 8; ++row) pattern[row] = 0x5555U;
    assert(!demo.BuildBox(nullptr, 8));
    assert(!demo.BuildBox(pattern, 7));
    assert(!demo.BuildBox(pattern, 9));
    assert(!demo.BuildPixel(8, 0, pattern, 8));
    assert(!demo.BuildPixel(0, 16, pattern, 8));
    assert(!demo.BuildPixel(255, 255, pattern, 8));
    assert(!demo.BuildPixel(0, 0, nullptr, 8));
    assert(!demo.BuildPixel(0, 0, pattern, 7));
    for (unsigned row = 0; row < 8; ++row) assert(pattern[row] == 0x5555U);
    assert(storage[0] == 0x1234U && storage[9] == 0xabcdU);
    std::puts("PASS: box, 128 scan positions, bounds and invalid-input handling.");
}
