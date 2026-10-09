# P1_Example_HelloWorld

**Author:** Dr Ian S. Howard  
School of Engineering, Computing and Mathematics  
University of Plymouth

A first C++ program for the **STM32 NUCLEO-F439ZI** using **PlatformIO** and the **STM32Cube HAL** framework.

This project is deliberately **portable between Windows, macOS and Linux**. It does not contain generated PlatformIO build files or machine-specific absolute paths.

## Learning objectives

This example introduces:

- the basic structure of a PlatformIO STM32 project
- the `main()` entry point
- board and peripheral initialisation
- basic encapsulation of hardware-specific code
- serial communication through the ST-LINK Virtual COM Port
- using `printf()` on an embedded target
- integer and floating-point variables
- an embedded `while (1)` loop
- delays using the STM32 HAL
- the role of the SysTick timer in HAL timing

## Hardware

Target board: **STM32 NUCLEO-F439ZI**

The project configures **USART3** for serial communication using:

- `PD8` - USART3 transmit (TX)
- `PD9` - USART3 receive (RX)
- baud rate: **115200**

USART3 is connected to the board's ST-LINK Virtual COM Port, so serial output can be viewed directly in the PlatformIO serial monitor.

## Project structure

```text
P1_Example_HelloWorld/
├── lib/
│   └── BoardSupport/
│       ├── board.cpp
│       ├── board.h
│       ├── serial.cpp
│       └── serial.h
├── src/
│   ├── interrupts.cpp
│   └── main.cpp
├── platformio.ini
└── README.md
```

The `.pio` build directory and generated VS Code files are intentionally not included. PlatformIO creates them locally on each computer using that computer's own paths.

## Building and running

1. Extract the project to a normal local folder on the PC.
2. In VS Code, choose **File -> Open Folder...** and open the folder containing `platformio.ini`.
3. Make sure the **PlatformIO IDE** extension is installed.
4. Connect the NUCLEO-F439ZI using the ST-LINK USB connector.
5. Build using **PlatformIO: Build**.
6. Upload using **PlatformIO: Upload**.
7. Open **PlatformIO: Serial Monitor**. The baud rate is set to **115200** in `platformio.ini`.

The first build may take a little longer because PlatformIO creates its local `.pio` directory and, if necessary, installs the STM32 platform/toolchain in the current user's PlatformIO directory.

### Serial port on Windows

PlatformIO normally detects the ST-LINK Virtual COM Port automatically. If it does not, identify the COM port using:

```text
pio device list
```

The port will normally appear as something such as `COM3`, `COM4`, etc. A fixed `monitor_port` should normally **not** be put in `platformio.ini`, because the COM number can differ between PCs.

### Serial port on macOS

PlatformIO also normally detects the port automatically. If required, it can be inspected with:

```text
pio device list
```

or:

```text
ls /dev/cu.usbmodem*
```

## Program behaviour

After the board is initialised, the program waits briefly so that the serial monitor has time to connect and then prints:

```text
Hello World!
```

It then enters an infinite loop. Once per second it prints an integer counter and the same value converted to floating point, for example:

```text
Hello from STM32! idx=0
and as a float: idx=0.000000

Hello from STM32! idx=1
and as a float: idx=1.000000
```

## Board support

`board.cpp` and `board.h` provide `Board_Init()`, which initialises the STM32 HAL and serial communications.

`serial.cpp` and `serial.h` configure USART3 and redirect `printf()` output to the serial port. The hardware-specific details remain encapsulated inside the serial module.

## SysTick interrupt

`interrupts.cpp` defines the SysTick interrupt handler. SysTick provides the millisecond time base used by HAL timing functions such as:

```cpp
HAL_Delay(1000);
```

## Floating-point `printf()`

The example prints a floating-point value using `%f`. The following linker option in `platformio.ini` enables floating-point formatting:

```ini
build_flags =
    -Wl,-u_printf_float
```

## Portability note

Do **not** copy a `.pio` directory or generated `.vscode/c_cpp_properties.json` / `.vscode/launch.json` files from one computer to another. They contain paths specific to the computer on which PlatformIO generated them.

The portable project files are the source files, libraries and `platformio.ini`. PlatformIO regenerates the machine-specific files automatically.
