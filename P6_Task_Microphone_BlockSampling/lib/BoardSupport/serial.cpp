// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "serial.h"
#include "stm32f4xx_hal.h"   // STM32F4 Hardware Abstraction Layer


// Everything in this unnamed namespace is private to the serial module.
namespace
{
    constexpr uint32_t SERIAL_BAUD_RATE = 115200;

    // USART3 configuration structure used by the HAL.
    UART_HandleTypeDef huart3;


// -----------------------------------------------------------------------------
// Configure the GPIO pins used by USART3
//
// PD8 = USART3 transmit (TX)
// PD9 = USART3 receive  (RX)
//
// USART3 is connected to the ST-LINK Virtual COM Port on the NUCLEO-F439ZI.
// -----------------------------------------------------------------------------

    void InitialiseSerialGpio()
    {
        // Enable the clock for GPIO port D.
        __HAL_RCC_GPIOD_CLK_ENABLE();

        // Create and initialise a GPIO configuration structure.
        GPIO_InitTypeDef GPIO_InitStruct = {};

        // Configure PD8 and PD9 for USART3 operation.
        GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;          // Alternate function, push-pull
        GPIO_InitStruct.Pull      = GPIO_NOPULL;              // No internal pull-up/down
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;     // High-speed GPIO
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;          // Select USART3 function

        // Apply the GPIO configuration.
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    }


// -----------------------------------------------------------------------------
// Configure USART3
//
// Baud rate:    115200 bits/s
// Data bits:    8
// Parity:       None
// Stop bits:    1
// Flow control: None
//
// Commonly written as: 115200, 8-N-1
// -----------------------------------------------------------------------------

    void InitialiseSerialUart()
    {
        // Enable the clock for the USART3 peripheral.
        __HAL_RCC_USART3_CLK_ENABLE();

        // Select USART3.
        huart3.Instance = USART3;

        // Configure the serial communication parameters.
        huart3.Init.BaudRate     = SERIAL_BAUD_RATE;
        huart3.Init.WordLength   = UART_WORDLENGTH_8B;
        huart3.Init.StopBits     = UART_STOPBITS_1;
        huart3.Init.Parity       = UART_PARITY_NONE;
        huart3.Init.Mode         = UART_MODE_TX_RX;
        huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
        huart3.Init.OverSampling = UART_OVERSAMPLING_16;

        // Initialise USART3 using the HAL.
        HAL_UART_Init(&huart3);
    }
}


// -----------------------------------------------------------------------------
// Initialise serial communications
// -----------------------------------------------------------------------------

void Serial_Init()
{
    // Configure the USART pins first.
    InitialiseSerialGpio();

    // Then configure the USART peripheral.
    InitialiseSerialUart();
}


// -----------------------------------------------------------------------------
// Redirect printf() output to USART3
//
// The standard printf() function ultimately calls _write(). By implementing
// _write() here, printf() output is sent through the STM32 UART.
// -----------------------------------------------------------------------------

extern "C" int _write(int file, char *ptr, int len)
{
    // The file parameter is not required for this application.
    (void)file;

    // Send the characters supplied by printf().
    HAL_UART_Transmit(
        &huart3,
        reinterpret_cast<uint8_t *>(ptr),
        len,
        HAL_MAX_DELAY
    );

    // Return the number of characters transmitted.
    return len;
}
