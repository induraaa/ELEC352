// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "board.h"

#include "stm32f4xx_hal.h"   // STM32F4 Hardware Abstraction Layer
#include "serial.h"          // Serial port initialisation


// -----------------------------------------------------------------------------
// Initialise the development board
//
// HAL_Init()    - initialises the STM32 HAL library and system tick
// Serial_Init() - configures USART3 for serial communication
// -----------------------------------------------------------------------------

void Board_Init()
{
    HAL_Init();
    Serial_Init();
}
