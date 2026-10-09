// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// main.cpp
//
// A first STM32 C++ program for the NUCLEO-F439ZI.
//
// The program initialises the board, sends text to the serial monitor using
// printf(), and then repeatedly prints an integer counter and its floating-point
// equivalent once per second.
// -----------------------------------------------------------------------------

#include "stm32f4xx_hal.h"   // STM32F4 Hardware Abstraction Layer
#include "board.h"           // Board and printf initialisation

#include <cstdint>
#include <cstdio>


// Main program entry point
int main()
{
    int idx;

    // Initialise the Nucleo board. After this call, printf() is ready to use.
    Board_Init();

    // Allow time for the PlatformIO serial monitor to connect before the first
    // message is transmitted. Without this delay, the first printf() may occur
    // before the serial monitor is ready.
    HAL_Delay(1000);

    // Print an initial message to the serial monitor.
    printf("ELEC352::Hello from STM32!\r\n");

    // Initialise the loop counter.
    idx = 0;

    // Embedded programs normally run continuously in an infinite loop.
    while (1)
    {
        // Wait for 1000 ms (1 second).
        HAL_Delay(1000);

        // Print the current counter value as an integer.
        printf("Hello from STM32! idx=%d\r\n", idx);

        // Convert the counter to float and print it using floating-point format.
        printf("and as a float: idx=%f\r\n", (float)idx);

        // Print a blank line to make the serial output easier to read.
        printf("\r\n");

        // Increment the counter ready for the next loop iteration.
        idx++;
    }
}
