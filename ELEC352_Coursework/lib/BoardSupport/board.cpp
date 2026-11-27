// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "board.h"

#include "stm32f4xx_hal.h"
#include "serial.h"


void Board_Init()
{
    HAL_Init();
    Serial_Init();
}
