# P6_Example_LEDMatrix

**Author:** Dr Ian S. Howard  
School of Engineering, Computing and Mathematics  
University of Plymouth

An object-oriented C++ example for controlling the 16-column by 8-row LED matrix on the module support board with the **STM32 NUCLEO-F439ZI**, **PlatformIO**, and **STM32Cube HAL**.

The `LedMatrix` class owns the complete GPIO mapping and SPI3 peripheral, while `LedMatrixDemo` generates example images in a caller-owned `uint16_t pattern[8]` array. `main.cpp` passes this array to `LedMatrix` and controls the animation sequence and timing. This separates the mechanism used to refresh the physical display from the patterns the application wants to show.

## Learning objectives

This example introduces:

- encapsulating GPIO and an SPI peripheral inside a C++ hardware class
- constructors with explicit member assignments
- a consistent `Initialise()` method
- transmitting data through cascaded shift registers
- refreshing a display by row multiplexing
- representing a 16-column image with 16-bit row values
- connecting an image generator and hardware driver through a plain bitmap array
- separating logical image coordinates from physical wiring

## Hardware mapping

| Matrix signal | STM32 connection | Configuration |
|---|---|---|
| SPI3 clock | `PC10` | Alternate function 6 |
| SPI3 MISO | `PC11` | Alternate function 6 |
| SPI3 MOSI | `PC12` | Alternate function 6 |
| Shift-register latch | `PB6` | Push-pull output |
| Output enable | `PB12` | Active-low push-pull output |
| Serial output | USART3 on `PD8`/`PD9`, 115200 baud | ST-LINK Virtual COM Port |

The matrix receives data through SPI3 using MOSI and the clock. PC11 is configured as the associated SPI3 MISO pin for completeness, although this transmit-only example does not read data back from the display.

All matrix-specific hardware information is contained in `LedMatrix`. The global SPI handle and matrix definitions previously located in `board.cpp` and `board.h` have been removed.

## Initialisation structure

Common project infrastructure remains deliberately small:

```cpp
void Board_Init()
{
    HAL_Init();
    Serial_Init();
}
```

The matrix then initialises itself:

```cpp
LedMatrix matrix;
matrix.Initialise();
```

`LedMatrix::Initialise()` performs the following operations:

1. Enables GPIO ports B and C.
2. Establishes safe latch and output-enable states.
3. Configures PC10–PC12 for SPI3 alternate function 6.
4. Configures PB6 and PB12 as control outputs.
5. Configures SPI3 as an 8-bit mode-0 master.
6. Clears and enables the matrix.

The `SPI_HandleTypeDef` is a private member of `LedMatrix` and is explicitly cleared in the constructor before use.

## Shift-register transfer

The matrix electronics contain three cascaded eight-bit shift registers. Each row update transmits three bytes in this order:

| Transfer order | Contents |
|---:|---|
| 1 | Right-hand eight-column pattern |
| 2 | Left-hand eight-column pattern |
| 3 | Three-bit physical row selection |

The latch remains low while the 24 bits are shifted into place. A short high pulse then transfers the complete result to the display outputs. `SetRow()` temporarily disables the outputs during this operation to reduce visible ghosting.

## Logical image representation

A complete image is supplied as eight 16-bit row values:

```cpp
const uint16_t boxPattern[8] =
{
    0b1111111111111111,
    0b1000000000000001,
    0b1000000000000001,
    0b1000000000000001,
    0b1000000000000001,
    0b1000000000000001,
    0b1000000000000001,
    0b1111111111111111
};
```

Logical row 0 is the top row. Within each row, bit 15 represents the leftmost column and bit 0 represents the rightmost column.

The PCB wiring reverses both the physical row numbering and the bit order within each eight-column group. `LedMatrix` performs those conversions internally, so application patterns can use the natural top-to-bottom, left-to-right representation.

## Row multiplexing

Only one physical row is selected at a time. The shared `LedMatrix` class stores a complete 8 x 16 logical image with `LoadPattern()` and advances the physical display by one row approximately every 1 ms when `Refresh()` is called. One complete eight-row scan therefore takes approximately 8 ms, corresponding to roughly 125 complete frames per second before allowing for small transfer overheads.

`Refresh()` is non-blocking and is intended for later real-time applications that must continue processing while the display is multiplexed. This introductory project uses the convenience method `ShowPattern()`, which loads an image and repeatedly calls `Refresh()` for the requested duration before clearing the display.

## Separation of responsibilities

`LedMatrix` is responsible for:

- GPIO and SPI3 initialisation
- hardware pin mapping
- SPI transfers and latch control
- physical row and column conversion
- display buffering and row multiplexing
- non-blocking `Refresh()` support

`LedMatrixDemo` is responsible for:

- defining the rectangular border image
- generating an image containing one pixel at a requested row and column
- validating output lengths and pixel coordinates

`main.cpp` is responsible for the moving-pixel sequence and how long each image
remains visible. The two objects are constructed independently:

```cpp
LedMatrix matrix;
matrix.Initialise();
LedMatrixDemo demo;
uint16_t pattern[LedMatrixDemo::ROW_COUNT] = {};

if (demo.BuildBox(pattern, LedMatrixDemo::ROW_COUNT))
{
    matrix.ShowPattern(pattern, 3000U);
}
```

For one pixel at the bottom-left corner:

```cpp
if (demo.BuildPixel(7U, 0U, pattern, LedMatrixDemo::ROW_COUNT))
{
    matrix.ShowPattern(pattern, 75U);
}
```

`BuildBox()` and `BuildPixel()` fill the caller's array synchronously without
accessing hardware or retaining its address. They require exactly eight output
rows. Null pointers, incorrect row counts, or pixel coordinates outside rows
0–7 and columns 0–15 return false and leave the output unchanged. The supplied
count must describe actual valid storage.

This matches the array interface used by `VectorDisplay` in the later projects.
A bitmap uses `uint16_t` because each bit represents an LED. No class instance
is passed between the image generator and hardware driver. The image generator
can be compiled and tested without STM32 headers.

`ShowPattern()` keeps the original blocking demonstration behaviour. For a later
application that needs to perform other work, use `matrix.LoadPattern(pattern)`
and call `matrix.Refresh()` frequently from the main loop.

## Program behaviour

The demonstration repeatedly:

1. Displays a rectangular border for 3 seconds.
2. Pauses for 500 ms.
3. Scans one pixel across every row, starting at the bottom.
4. Pauses for 500 ms.

Each stage is reported through `printf()`. Serial output is available immediately after `Board_Init()`, with floating-point formatting enabled for consistency with later engineering examples.

## Project structure

```text
P6_Example_LEDMatrix/
├── include/
│   ├── LedMatrix.h
│   └── LedMatrixDemo.h
├── lib/
│   └── BoardSupport/
│       ├── board.cpp
│       ├── board.h
│       ├── serial.cpp
│       └── serial.h
├── src/
│   ├── LedMatrix.cpp
│   ├── LedMatrixDemo.cpp
│   ├── interrupts.cpp
│   └── main.cpp
├── .gitignore
├── platformio.ini
└── README.md
```

## Building and running

1. Open the `P6_Example_LEDMatrix` folder in VS Code with the PlatformIO extension installed.
2. Connect the NUCLEO-F439ZI through its ST-LINK USB connector and attach the module support board.
3. Select **PlatformIO: Build**.
4. Select **PlatformIO: Upload**.
5. Open **PlatformIO: Serial Monitor** at **115200 baud**.

The matrix should alternate between the rectangular border and the complete moving-pixel scan while the serial monitor reports each stage.

## Shared-class relationship

`include/LedMatrix.h` and `src/LedMatrix.cpp` are intentionally identical to the `LedMatrix` class used in `L4eg2_MicrophoneSpectrumAnalyser`. `LedMatrixDemo` remains specific to this introductory matrix demonstration.

## Changes and validation

`LedMatrixDemo.h`, `LedMatrixDemo.cpp`, `main.cpp` and this README were updated.
The old `LedMatrixDemo(matrix)`, `ShowBox(duration)` and `ScanPixels(duration)`
interface is replaced by independent construction and `BuildBox()`/`BuildPixel()`.
Detailed comments and author headers are preserved or updated for the new roles.
The hardware driver, board support, interrupts and PlatformIO configuration are
unchanged. This revised array-based example is now incorporated into the
combined lab collection.

Host checks cover the border image, all 128 pixel positions, output-array guards
and invalid coordinates. The image generator compiles as standard C++11 without
STM32 headers; `main.cpp` was syntax-checked using a minimal HAL substitute.
These checks do not constitute a full STM32 firmware build or physical-board
test, which remain unverified because the required toolchain is unavailable.

## Place in the teaching sequence

The same `LedMatrix.h/.cpp` is reused unchanged in the vector-display lab and
coursework. `LedMatrixDemo` is a practice image source; it is not required in the
final spectrum analyser.

See `../README_CLASS_PROGRESSION.md` for the complete class map, preparation
exercises and coursework boundary.
