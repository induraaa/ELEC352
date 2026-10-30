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

#include "stm32f4xx_hal.h" // STM32F4 Hardware Abstraction Layer
#include "board.h"         // Board and printf initialisation

#include <cstdint>
#include <cstdio>

struct ComplexSample
{
    float real;
    float imag;
};

ComplexSample Add(const ComplexSample &a, const ComplexSample &b)
{
    return ComplexSample{a.real + b.real, a.imag + b.imag};
}
float MagnitudeSquared(const ComplexSample &value)
{
    return value.real * value.real + value.imag * value.imag;
}

void PrintSample(const ComplexSample &value)
{
    printf("(%.1f, %.1f)\r\n", value.real, value.imag);
}

void ScaleCopy(ComplexSample value, float factor)
{
    value.real *= factor;
    value.imag *= factor;
}
void ScaleInPlace(ComplexSample &value, float factor)
{
    value.real *= factor;
    value.imag *= factor;
}

struct ButtonState
{
    bool wasPressed;
    uint32_t presses;
};
bool UpdateButton(ButtonState& state, bool pressed)
{
    bool event = pressed && !state.wasPressed;

    if (event)
    {
        state.presses++;
    }

    state.wasPressed = pressed;

    return event;
}

int main()
{
    int idx;

    // Initialise the Nucleo board. After this call, printf() is ready to use.
    Board_Init();

    ComplexSample first{3.0f, 4.0f};
    ComplexSample second = first;
    ComplexSample &alias = first;

    ComplexSample x{3.0f, 4.0f};
    ScaleCopy(x, .0f);

    x = {3.0f, 4.0f};
    ScaleInPlace(x, 2.0f);
    PrintSample(x);

    HAL_Delay(1000);
    printf("first=(%.1f, %.1f)\r\n", first.real, first.imag);
    printf("second=(%.1f, %.1f)\r\n", second.real, second.imag);
    printf("\r\n");

    second.real = -2.0f;
    printf("After changing second.real:\r\n");
    printf("first=(%.1f, %.1f)\r\n", first.real, first.imag);
    printf("second=(%.1f, %.1f)\r\n", second.real, second.imag);
    printf("\r\n");

    first.imag = 8.0f;
    printf("After changing first.imag:\r\n");
    printf("first=(%.1f, %.1f)\r\n", first.real, first.imag);
    printf("second=(%.1f, %.1f)\r\n", second.real, second.imag);
    printf("\r\n");

    alias.real = 7.0f;
    printf("After changing alias.real:\r\n");
    printf("first=(%.1f, %.1f)\r\n", first.real, first.imag);
    printf("second=(%.1f, %.1f)\r\n", second.real, second.imag);
    printf("\r\n");

    printf("first + second = ");
    PrintSample(Add(first, second));
    printf("MagnitudeSquared(first) = %.1f\r\n", MagnitudeSquared(first));
    printf("MagnitudeSquared(second) = %.1f\r\n", MagnitudeSquared(second));
    printf("\r\n");

    PrintSample(x);

    // Allow time for the PlatformIO serial monitor to connect before the first
    // message is transmitted. Without this delay, the first printf() may occur
    // before the serial monitor is ready.

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
