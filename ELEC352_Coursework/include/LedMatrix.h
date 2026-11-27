// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------
//
// LedMatrix.h
//
// Low-level driver for the 16-column x 8-row LED matrix fitted to the module
// support board.  The class provides:
//
//   1. GPIO configuration for the matrix control signals.
//   2. SPI3 configuration for transferring row and column data.
//   3. A persistent 8 x 16 logical image buffer.
//   4. Row multiplexing so the complete matrix appears continuously illuminated.
//
// The class deliberately separates the LOW-LEVEL hardware interface from any
// HIGHER-LEVEL interpretation of the displayed data.  For example, a
// VectorDisplay object can convert a vector of numerical values into an image,
// while LedMatrix is concerned only with displaying that image on the hardware.
// -----------------------------------------------------------------------------

#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "stm32f4xx_hal.h"

#include <cstdint>


// -----------------------------------------------------------------------------
// LedMatrix
// -----------------------------------------------------------------------------
// Represents the 16-column x 8-row LED matrix on the module support board.
//
// IMAGE REPRESENTATION
// --------------------
// The display image is stored as eight 16-bit words:
//
//     pattern_[0]  -> logical top row
//     pattern_[1]  -> next row
//         ...
//     pattern_[7]  -> logical bottom row
//
// Within each 16-bit row:
//
//     bit 15 -> logical leftmost column
//     bit  0 -> logical rightmost column
//
// A bit value of 1 requests that LED to be illuminated; a value of 0 requests
// that it be off.  The conversion between this convenient logical orientation
// and the physical wiring of the board is handled internally by the class.
//
// ROW MULTIPLEXING
// ----------------
// The hardware does not display all eight rows simultaneously.  Instead, one
// row is selected at a time.  Refresh() advances through the rows, displaying
// each for approximately 1 ms.  Cycling quickly through all eight rows creates
// the visual impression of a continuously illuminated 8 x 16 image.
//
// With ROW_REFRESH_MS = 1 ms, one complete eight-row scan takes approximately
// 8 ms, corresponding to a nominal full-frame refresh rate of about 125 Hz.
//
// NON-BLOCKING USE
// ----------------
// For applications that must perform other work while the matrix is active:
//
//     matrix.LoadPattern(image);
//
//     while (true)
//     {
//         matrix.Refresh();
//         // Other application processing can run here.
//     }
//
// Refresh() returns immediately if the next row is not yet due.  It is therefore
// suitable for cooperative main-loop programs such as the microphone spectrum
// analyser.
//
// BLOCKING USE
// ------------
// ShowPattern() is supplied as a simple convenience function for demonstrations.
// It repeatedly calls Refresh() for the requested duration and therefore blocks
// the caller until the display interval has elapsed.
// -----------------------------------------------------------------------------
class LedMatrix
{
public:
    // Number of physical/logical rows in the matrix.
    // The matrix has 16 columns, represented by the 16 bits of uint16_t.
    static constexpr uint8_t ROW_COUNT = 8;

    // Construct the object and establish the board-specific pin assignments.
    // No hardware peripheral is enabled by the constructor; call Initialise()
    // before attempting to display anything.
    LedMatrix();

    // Configure the GPIO pins and SPI3 peripheral used by the matrix, clear the
    // logical image buffer, and place the hardware in a known blank state.
    void Initialise();

    // Enable the matrix outputs.
    // The board's output-enable signal is active LOW, so this method drives the
    // output-enable pin low.
    void Enable();

    // Disable the matrix outputs.
    // This is used while changing row/column data so intermediate values are not
    // briefly visible on the display.
    void Disable();

    // Clear the complete logical image buffer and blank the hardware immediately.
    void Clear();

    // Immediately select one PHYSICAL matrix row and provide the eight column
    // bits for its left and right halves.
    //
    // This is a low-level operation.  Most application code should instead use
    // LoadPattern() plus Refresh(), which work with the complete logical image.
    //
    // Parameters:
    //   row          Physical row number.  Only the lower three bits are used,
    //                giving a hardware row range of 0 to 7.
    //   leftPattern  Eight column bits for one half of the row.
    //   rightPattern Eight column bits for the other half of the row.
    void SetRow(
        uint8_t row,
        uint8_t leftPattern,
        uint8_t rightPattern
    );

    // Copy a complete logical 8 x 16 image into the internal display buffer.
    //
    // pattern[0] is the logical top row and pattern[7] the logical bottom row.
    // Within each row, bit 15 is the logical leftmost column and bit 0 the
    // logical rightmost column.
    //
    // The image is only stored here; Refresh() performs the actual multiplexed
    // transfer to the matrix hardware.  Passing nullptr clears the display.
    void LoadPattern(const uint16_t pattern[ROW_COUNT]);

    // Perform one step of the non-blocking row-multiplexing process.
    //
    // Call Refresh() as frequently as possible from the main loop.  If at least
    // ROW_REFRESH_MS has elapsed since the previous row update, one logical row
    // is transferred to the hardware and the internal row index advances.
    // Otherwise the function simply returns.
    void Refresh();

    // Blocking convenience function for simple demonstrations.
    //
    // The supplied pattern is loaded, repeatedly refreshed for durationMs, and
    // then cleared.  Because this function waits for the complete duration, use
    // LoadPattern()/Refresh() instead when other real-time processing must run
    // concurrently with the display.
    void ShowPattern(
        const uint16_t pattern[ROW_COUNT],
        uint32_t durationMs
    );

private:
    // Number of bits processed by ReverseByte().
    static constexpr uint8_t BITS_PER_BYTE = 8;

    // Each SPI update consists of three bytes:
    //   byte 0 -> right-half column pattern
    //   byte 1 -> left-half column pattern
    //   byte 2 -> selected row number
    static constexpr uint8_t TRANSFER_SIZE = 3;

    // Time for which each multiplexed row is nominally retained before the next
    // row is selected.  HAL_GetTick() has millisecond resolution.
    static constexpr uint32_t ROW_REFRESH_MS = 1U;

    // Configure the GPIO pins used by SPI3 and the matrix control signals.
    void InitialiseGpio();

    // Configure SPI3 as the master interface used to send matrix data.
    void InitialiseSpi();

    // Reverse the order of the eight bits in a byte.  This compensates for the
    // physical column wiring being opposite to the logical image bit order.
    uint8_t ReverseByte(uint8_t value) const;

    // Send one complete three-byte row/column update to the matrix interface and
    // latch the transferred values into the hardware outputs.
    void SendData(
        uint8_t row,
        uint8_t leftPattern,
        uint8_t rightPattern
    );

    // Generate the rising/falling latch pulse that makes newly shifted data take
    // effect at the matrix interface.
    void PulseLatch();

    // Convert one row from the logical image representation to the physical
    // wiring representation and send it to the matrix.
    void DisplayLogicalRow(uint8_t logicalRow);

    // -------------------------------------------------------------------------
    // Board-specific SPI3 pin assignments.
    // -------------------------------------------------------------------------
    GPIO_TypeDef* spiPort_;
    uint16_t spiClockPin_;
    uint16_t spiMisoPin_;
    uint16_t spiMosiPin_;

    // -------------------------------------------------------------------------
    // Matrix control signals.
    // -------------------------------------------------------------------------
    GPIO_TypeDef* controlPort_;
    uint16_t latchPin_;
    uint16_t outputEnablePin_;

    // STM32 HAL handle containing the SPI3 peripheral configuration and state.
    SPI_HandleTypeDef spi_;

    // -------------------------------------------------------------------------
    // Persistent display state used by the non-blocking multiplexing process.
    // -------------------------------------------------------------------------

    // Complete logical 8 x 16 image currently requested by the application.
    uint16_t pattern_[ROW_COUNT];

    // Logical row that will be transferred on the next timed Refresh() update.
    uint8_t currentRow_;

    // HAL tick value recorded when the previous row was transferred.
    uint32_t lastRefreshTick_;
};


#endif  // LED_MATRIX_H
