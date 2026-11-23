// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------
//
// LedMatrix.cpp
//
// Implementation of the low-level 16 x 8 LED-matrix driver.
//
// The matrix is ROW MULTIPLEXED: only one row is actively selected at a time.
// Refresh() moves through the eight rows quickly enough that persistence of
// vision makes the complete image appear continuously illuminated.
//
// Higher-level classes should create a logical 8 x 16 image and pass it to
// LoadPattern().  This class then deals with the physical SPI transfer, bit
// ordering, row ordering, output enable, latching, and refresh timing.
// -----------------------------------------------------------------------------

#include "LedMatrix.h"


// -----------------------------------------------------------------------------
// LedMatrix constructor
// -----------------------------------------------------------------------------
// Store the board-specific pin assignments and establish a known SOFTWARE state.
//
// Hardware clocks and peripherals are deliberately not enabled here.  Keeping
// hardware initialisation in Initialise() makes construction predictable and
// allows application code to control when the hardware becomes active.
// -----------------------------------------------------------------------------
LedMatrix::LedMatrix()
{
    // -------------------------------------------------------------------------
    // SPI3 connections on the module support board.
    //
    // SPI clock (SCK)  -> PC10
    // SPI MISO         -> PC11
    // SPI MOSI         -> PC12
    //
    // This class only transmits matrix data, but the SPI peripheral is configured
    // in two-line mode and the complete SPI3 pin group is therefore initialised.
    // -------------------------------------------------------------------------
    spiPort_ = GPIOC;
    spiClockPin_ = GPIO_PIN_10;
    spiMisoPin_ = GPIO_PIN_11;
    spiMosiPin_ = GPIO_PIN_12;

    // -------------------------------------------------------------------------
    // Additional matrix-control connections on the module support board.
    //
    // latchPin_        -> transfers newly shifted serial data to the outputs.
    // outputEnablePin_ -> enables/disables the visible matrix outputs.
    // -------------------------------------------------------------------------
    controlPort_ = GPIOB;
    latchPin_ = GPIO_PIN_6;
    outputEnablePin_ = GPIO_PIN_12;

    // Value-initialise the STM32 HAL SPI handle so all fields begin in a known
    // state, then associate the handle with the SPI3 hardware peripheral.
    spi_ = {};
    spi_.Instance = SPI3;

    // Start with an entirely blank LOGICAL image.  One uint16_t stores all
    // 16 columns belonging to a single row.
    for (uint8_t row = 0; row < ROW_COUNT; row++)
    {
        pattern_[row] = 0U;
    }

    // The first timed refresh will begin with logical row 0 (the top row).
    currentRow_ = 0U;

    // A real HAL tick value is installed by Initialise().  Zero is simply a safe
    // constructor-time value before the STM32 hardware has been configured.
    lastRefreshTick_ = 0U;
}


// -----------------------------------------------------------------------------
// Initialise
// -----------------------------------------------------------------------------
// Configure all hardware needed by the matrix and leave both the logical buffer
// and physical display blank.
// -----------------------------------------------------------------------------
void LedMatrix::Initialise()
{
    // GPIO must be configured before it can be used by the SPI peripheral or as
    // the matrix's latch/output-enable control signals.
    InitialiseGpio();

    // Configure and enable SPI3 after its GPIO pins are ready.
    InitialiseSpi();

    // Clear both the internal image and the visible hardware state.
    Clear();

    // Record the current millisecond tick.  Refresh() uses elapsed time relative
    // to this value to decide when the first row update is due.
    lastRefreshTick_ = HAL_GetTick();
}


// -----------------------------------------------------------------------------
// InitialiseGpio
// -----------------------------------------------------------------------------
// Configure:
//   * GPIOC pins PC10/PC11/PC12 for the SPI3 alternate function.
//   * GPIOB pins PB6/PB12 as ordinary digital outputs for matrix control.
// -----------------------------------------------------------------------------
void LedMatrix::InitialiseGpio()
{
    // A GPIO peripheral cannot be configured until its RCC clock is enabled.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Establish safe output data-register values BEFORE changing the pins to
    // output mode.  This reduces the chance of a visible transient during setup.
    //
    // Latch is held low while no new data are being committed.
    HAL_GPIO_WritePin(controlPort_, latchPin_, GPIO_PIN_RESET);

    // Output enable is active LOW, so driving it HIGH keeps the matrix disabled
    // while its interface is being configured.
    HAL_GPIO_WritePin(controlPort_, outputEnablePin_, GPIO_PIN_SET);

    // -------------------------------------------------------------------------
    // Configure the three SPI3 pins.
    // -------------------------------------------------------------------------
    GPIO_InitTypeDef spiGpio = {};

    // All three signals are on GPIOC and can therefore be configured together.
    spiGpio.Pin = spiClockPin_ | spiMisoPin_ | spiMosiPin_;

    // Alternate-function push-pull allows the SPI peripheral, rather than normal
    // GPIO software writes, to drive the pins.
    spiGpio.Mode = GPIO_MODE_AF_PP;

    // No internal pull-up or pull-down is requested by this driver.
    spiGpio.Pull = GPIO_NOPULL;

    // SPI is a comparatively fast digital interface, so use the high GPIO output
    // speed setting for the peripheral pins.
    spiGpio.Speed = GPIO_SPEED_FREQ_HIGH;

    // On this STM32 device, alternate function AF6 connects these pins to SPI3.
    spiGpio.Alternate = GPIO_AF6_SPI3;

    // Apply the completed configuration to GPIOC.
    HAL_GPIO_Init(spiPort_, &spiGpio);

    // -------------------------------------------------------------------------
    // Configure latch and output-enable as conventional digital outputs.
    // -------------------------------------------------------------------------
    GPIO_InitTypeDef controlGpio = {};

    controlGpio.Pin = latchPin_ | outputEnablePin_;
    controlGpio.Mode = GPIO_MODE_OUTPUT_PP;
    controlGpio.Pull = GPIO_NOPULL;

    // These control signals change only around a row update, so the low GPIO
    // speed setting is sufficient.
    controlGpio.Speed = GPIO_SPEED_FREQ_LOW;

    // Apply the completed configuration to GPIOB.
    HAL_GPIO_Init(controlPort_, &controlGpio);
}


// -----------------------------------------------------------------------------
// InitialiseSpi
// -----------------------------------------------------------------------------
// Configure SPI3 for the serial transfer format used by the matrix interface.
// -----------------------------------------------------------------------------
void LedMatrix::InitialiseSpi()
{
    // The SPI3 peripheral registers cannot be used until the RCC clock is on.
    __HAL_RCC_SPI3_CLK_ENABLE();

    // This STM32 generates the SPI clock, so it acts as the bus master.
    spi_.Init.Mode = SPI_MODE_MASTER;

    // Use the normal two-line SPI arrangement.  Although this driver only sends
    // data, the peripheral is configured in its standard full-duplex form.
    spi_.Init.Direction = SPI_DIRECTION_2LINES;

    // Each SPI transfer unit is one byte.
    spi_.Init.DataSize = SPI_DATASIZE_8BIT;

    // Clock idle LOW and sample on the first clock edge: conventional SPI mode 0.
    spi_.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi_.Init.CLKPhase = SPI_PHASE_1EDGE;

    // No hardware NSS/chip-select signal is used by this driver.  Latching is
    // controlled explicitly by latchPin_, so NSS is managed in software.
    spi_.Init.NSS = SPI_NSS_SOFT;

    // Divide the SPI peripheral clock by 16.  The exact resulting bit rate depends
    // on the clock tree configured elsewhere in the project.
    spi_.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;

    // Send the most-significant bit of each byte first on the SPI bus.
    spi_.Init.FirstBit = SPI_FIRSTBIT_MSB;

    // Disable TI-specific framing; use ordinary Motorola-style SPI signalling.
    spi_.Init.TIMode = SPI_TIMODE_DISABLE;

    // Hardware CRC is not needed for this short local display connection.
    spi_.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    // HAL still requires a polynomial field in the handle even though CRC is
    // disabled, so retain the conventional value used by the original driver.
    spi_.Init.CRCPolynomial = 7;

    // Ask the HAL to configure and enable the SPI3 peripheral from the settings
    // stored above.
    HAL_SPI_Init(&spi_);
}


// -----------------------------------------------------------------------------
// ReverseByte
// -----------------------------------------------------------------------------
// Reverse the bit order in one 8-bit value.
//
// Example:
//
//     input  = 0b10010000
//     output = 0b00001001
//
// The logical image uses a natural left-to-right bit convention, while each
// physical eight-column half of the matrix is wired in the opposite bit order.
// Reversing each byte here keeps that hardware detail out of higher-level code.
// -----------------------------------------------------------------------------
uint8_t LedMatrix::ReverseByte(uint8_t value) const
{
    uint8_t reversed = 0U;

    // Process all eight input bits.  On each iteration:
    //   1. Make room for the next bit in 'reversed'.
    //   2. Copy the current least-significant input bit into that space.
    //   3. Shift the input so the following bit becomes available.
    for (uint8_t bit = 0; bit < BITS_PER_BYTE; bit++)
    {
        reversed = static_cast<uint8_t>(reversed << 1);
        reversed |= value & 0x01U;
        value >>= 1;
    }

    return reversed;
}


// -----------------------------------------------------------------------------
// SendData
// -----------------------------------------------------------------------------
// Send the three bytes required to update one matrix row.
//
// The byte order below is the order required by the module-support-board matrix
// interface:
//
//     data[0] : eight columns in the right half of the display
//     data[1] : eight columns in the left half of the display
//     data[2] : three-bit physical row selection (0..7)
//
// The serial data are shifted first and then committed to the visible outputs by
// PulseLatch().
// -----------------------------------------------------------------------------
void LedMatrix::SendData(
    uint8_t row,
    uint8_t leftPattern,
    uint8_t rightPattern
)
{
    // Local array containing one complete matrix update frame.
    uint8_t data[TRANSFER_SIZE];

    // Assemble the transfer in the board's required serial order.
    data[0] = rightPattern;
    data[1] = leftPattern;

    // Only three bits are needed to select one of eight rows.  Masking prevents
    // any upper bits in 'row' from being sent as part of the row address.
    data[2] = row & 0x07U;

    // Keep the latch low while new serial data are being shifted into the board.
    HAL_GPIO_WritePin(controlPort_, latchPin_, GPIO_PIN_RESET);

    // Transmit all three bytes as one blocking SPI transaction.  HAL_MAX_DELAY
    // means the call waits until the transfer completes rather than timing out.
    // The transaction itself is very short compared with the 1 ms row period.
    HAL_SPI_Transmit(
        &spi_,
        data,
        TRANSFER_SIZE,
        HAL_MAX_DELAY
    );

    // The shifted bits do not become the new matrix outputs until they are
    // latched.  Generate that latch pulse only after the complete frame arrives.
    PulseLatch();
}


// -----------------------------------------------------------------------------
// PulseLatch
// -----------------------------------------------------------------------------
// Commit the most recently shifted SPI data to the matrix interface outputs.
// -----------------------------------------------------------------------------
void LedMatrix::PulseLatch()
{
    // Generate a short LOW -> HIGH -> LOW pulse.  The transition causes the
    // interface hardware to copy the shifted serial values to its outputs.
    HAL_GPIO_WritePin(controlPort_, latchPin_, GPIO_PIN_SET);
    HAL_GPIO_WritePin(controlPort_, latchPin_, GPIO_PIN_RESET);
}


// -----------------------------------------------------------------------------
// Enable
// -----------------------------------------------------------------------------
// Make the currently selected row/column data visible.
// -----------------------------------------------------------------------------
void LedMatrix::Enable()
{
    // Output enable is ACTIVE LOW on the module support board.
    HAL_GPIO_WritePin(controlPort_, outputEnablePin_, GPIO_PIN_RESET);
}


// -----------------------------------------------------------------------------
// Disable
// -----------------------------------------------------------------------------
// Temporarily blank the matrix outputs.
// -----------------------------------------------------------------------------
void LedMatrix::Disable()
{
    // Because output enable is active low, a HIGH level disables the outputs.
    HAL_GPIO_WritePin(controlPort_, outputEnablePin_, GPIO_PIN_SET);
}


// -----------------------------------------------------------------------------
// SetRow
// -----------------------------------------------------------------------------
// Immediately update one PHYSICAL row of the matrix.
//
// The outputs are disabled while the serial data are changed.  This prevents a
// partially updated row/column combination from appearing briefly as visible
// ghosting during the transfer.
// -----------------------------------------------------------------------------
void LedMatrix::SetRow(
    uint8_t row,
    uint8_t leftPattern,
    uint8_t rightPattern
)
{
    // Three bits can represent exactly eight physical rows.  Masking also makes
    // this low-level method robust if a caller supplies a value greater than 7.
    row &= 0x07U;

    // Blank the LEDs while row and column information is changing.
    Disable();

    // Shift and latch the new row and column data.
    SendData(row, leftPattern, rightPattern);

    // Make the newly selected row visible.
    Enable();
}


// -----------------------------------------------------------------------------
// LoadPattern
// -----------------------------------------------------------------------------
// Copy an entire LOGICAL 8 x 16 image into the persistent internal buffer.
//
// This function does not itself scan the rows.  Once the image has been loaded,
// repeated calls to Refresh() transfer the rows to the physical display.
// -----------------------------------------------------------------------------
void LedMatrix::LoadPattern(const uint16_t pattern[ROW_COUNT])
{
    // Treat a null image pointer as a request to blank the display rather than
    // attempting to dereference an invalid pointer.
    if (pattern == nullptr)
    {
        Clear();
        return;
    }

    // Make a private copy of all eight rows.  The caller can therefore reuse or
    // modify its own array after LoadPattern() returns without affecting the
    // image currently being refreshed by this object.
    for (uint8_t row = 0; row < ROW_COUNT; row++)
    {
        pattern_[row] = pattern[row];
    }
}


// -----------------------------------------------------------------------------
// DisplayLogicalRow
// -----------------------------------------------------------------------------
// Convert one row from the program's convenient LOGICAL coordinate system into
// the PHYSICAL orientation required by the matrix hardware, then display it.
//
// This conversion is deliberately centralised here so higher-level display code
// can think simply in terms of "top", "bottom", "left" and "right".
// -----------------------------------------------------------------------------
void LedMatrix::DisplayLogicalRow(uint8_t logicalRow)
{
    // A 16-bit logical row contains both eight-column halves of the matrix.
    // Extract bits 15..8 as one byte and bits 7..0 as the other byte.
    uint8_t leftPattern =
        static_cast<uint8_t>(pattern_[logicalRow] >> 8);

    uint8_t rightPattern =
        static_cast<uint8_t>(pattern_[logicalRow] & 0x00FFU);

    // The physical columns are wired in the opposite bit order from the logical
    // image representation.  Reverse both bytes so the image appears with the
    // intended left-to-right orientation.
    leftPattern = ReverseByte(leftPattern);
    rightPattern = ReverseByte(rightPattern);

    // The logical convention numbers row 0 as the TOP of the image, while the
    // hardware numbering is reversed: hardware row 7 corresponds to the top.
    // Convert between those conventions here.
    const uint8_t hardwareRow =
        static_cast<uint8_t>((ROW_COUNT - 1U) - logicalRow);

    // SetRow() performs the actual blank-transfer-latch-enable sequence.
    SetRow(hardwareRow, leftPattern, rightPattern);
}


// -----------------------------------------------------------------------------
// Refresh
// -----------------------------------------------------------------------------
// Advance the row-multiplexing process without blocking the application.
//
// Refresh() should be called repeatedly from the main loop.  Calls made before
// the next 1 ms row interval has elapsed return immediately.  When a row is due,
// exactly one row is updated and the function returns again.
//
// This makes the display cooperative: matrix scanning and other application work
// can share the processor without a dedicated delay inside the driver.
// -----------------------------------------------------------------------------
void LedMatrix::Refresh()
{
    // HAL_GetTick() returns the number of milliseconds elapsed since the HAL tick
    // timer started.
    const uint32_t now = HAL_GetTick();

    // HAL_GetTick() has 1 ms resolution.  Returning immediately when the next
    // row is not due keeps this method non-blocking.
    //
    // Unsigned subtraction is intentionally used here.  It continues to give the
    // correct elapsed interval when the 32-bit tick counter eventually wraps.
    if ((now - lastRefreshTick_) < ROW_REFRESH_MS)
    {
        return;
    }

    // Remember when this row update was started so the following row will not be
    // selected until another ROW_REFRESH_MS interval has elapsed.
    lastRefreshTick_ = now;

    // Convert and display the row currently selected by the scan state.
    DisplayLogicalRow(currentRow_);

    // Advance to the next logical row ready for the following refresh interval.
    currentRow_++;

    // After row 7, wrap back to row 0 to begin the next complete matrix scan.
    if (currentRow_ >= ROW_COUNT)
    {
        currentRow_ = 0U;
    }
}


// -----------------------------------------------------------------------------
// Clear
// -----------------------------------------------------------------------------
// Blank both the logical image buffer and the physical display.
// -----------------------------------------------------------------------------
void LedMatrix::Clear()
{
    // Remove every illuminated pixel from the persistent logical image.
    for (uint8_t row = 0; row < ROW_COUNT; row++)
    {
        pattern_[row] = 0U;
    }

    // Restart the multiplexing sequence from the logical top row.  This gives a
    // deterministic scan order after a clear operation.
    currentRow_ = 0U;

    // Blank the hardware immediately rather than waiting for the next timed call
    // to Refresh().  With both column bytes zero, no LED in the selected row is
    // requested to illuminate.
    SetRow(0U, 0x00U, 0x00U);
}


// -----------------------------------------------------------------------------
// ShowPattern
// -----------------------------------------------------------------------------
// Blocking helper for simple demonstrations.
//
// It uses the same persistent image and Refresh() mechanism as normal operation,
// but keeps control inside this function until durationMs has elapsed.  This is
// convenient for simple animation examples but unsuitable when the application
// must simultaneously perform time-critical acquisition or control processing.
// -----------------------------------------------------------------------------
void LedMatrix::ShowPattern(
    const uint16_t pattern[ROW_COUNT],
    uint32_t durationMs
)
{
    // A null pointer cannot describe an image, so blank the display safely.
    if (pattern == nullptr)
    {
        Clear();
        return;
    }

    // Copy the requested image into the persistent buffer used by Refresh().
    LoadPattern(pattern);

    // Record the beginning of the requested display interval.
    const uint32_t startTime = HAL_GetTick();

    // Remain here until durationMs has elapsed.  Refresh() itself is non-blocking,
    // so the loop may execute many times between actual 1 ms row changes.
    while ((HAL_GetTick() - startTime) < durationMs)
    {
        Refresh();
    }

    // Leave the matrix blank when the requested display period has finished.
    Clear();
}
