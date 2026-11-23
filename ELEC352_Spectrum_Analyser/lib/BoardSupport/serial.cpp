// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "serial.h"
#include "stm32f4xx_hal.h"


// Everything in this unnamed namespace is private to the serial module.
namespace
{
    constexpr uint32_t SERIAL_BAUD_RATE = 115200;

    UART_HandleTypeDef huart3;


    // Configure PD8 and PD9 for their USART3 alternate functions.
    // USART3 is connected to the Nucleo board's ST-LINK Virtual COM Port.
    void InitialiseSerialGpio()
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();

        GPIO_InitTypeDef gpio = {};
        gpio.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF7_USART3;

        HAL_GPIO_Init(GPIOD, &gpio);
    }


    // Configure USART3 for 115200 baud, 8 data bits, no parity, and one stop
    // bit: conventionally written as 115200, 8-N-1.
    void InitialiseSerialUart()
    {
        __HAL_RCC_USART3_CLK_ENABLE();

        huart3.Instance          = USART3;
        huart3.Init.BaudRate     = SERIAL_BAUD_RATE;
        huart3.Init.WordLength   = UART_WORDLENGTH_8B;
        huart3.Init.StopBits     = UART_STOPBITS_1;
        huart3.Init.Parity       = UART_PARITY_NONE;
        huart3.Init.Mode         = UART_MODE_TX_RX;
        huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
        huart3.Init.OverSampling = UART_OVERSAMPLING_16;

        HAL_UART_Init(&huart3);
    }
}


void Serial_Init()
{
    InitialiseSerialGpio();
    InitialiseSerialUart();
}


// printf() ultimately calls _write(). Providing this implementation redirects
// the generated characters to USART3.
extern "C" int _write(int file, char* data, int length)
{
    (void)file;

    HAL_UART_Transmit(
        &huart3,
        reinterpret_cast<uint8_t*>(data),
        static_cast<uint16_t>(length),
        HAL_MAX_DELAY
    );

    return length;
}
