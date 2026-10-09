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

void InitialiseTrafficLeds()
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    const uint16_t pins = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6;
    HAL_GPIO_WritePin(GPIOC, pins, GPIO_PIN_RESET);
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = pins;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
}

void SetRed(bool on)
{
    if (on)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2,GPIO_PIN_RESET);
    }
}
void SetYellow(bool on)
{
    if (on)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3,GPIO_PIN_RESET);
    }
}
void SetGreen(bool on)
{
    if (on)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,GPIO_PIN_RESET);
    }
}
void InitialiseButtonsAB()
{
    __HAL_RCC_GPIOG_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOG, &gpio);
}

// Main program entry point
int main()
{
    // Initialise the Nucleo board. After this call, printf() is ready to use.
    Board_Init();

    InitialiseButtonsAB();
    InitialiseTrafficLeds() ;
    // Once, before the loop:
    uint32_t lastToggle = HAL_GetTick();    
    uint32_t lastReport = HAL_GetTick();
    while (true)
    {
        const bool a = HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_0) == GPIO_PIN_SET;
        const bool b = HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_1) == GPIO_PIN_SET;
        SetRed(a);
        SetGreen(b);
        const uint32_t now = HAL_GetTick();
        if (static_cast<uint32_t>(now - lastToggle) >= 500U)
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_3);
            lastToggle = now;
        }
        if ((now - lastReport) >= 100U) {
            printf("A=%d B=%d\r\n",
                   static_cast<int>(a), static_cast<int>(b));
            lastReport = now;
        }
        
    }
}