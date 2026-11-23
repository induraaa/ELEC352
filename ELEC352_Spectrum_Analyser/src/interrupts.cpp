// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "stm32f4xx_hal.h"


// -----------------------------------------------------------------------------
// SysTick interrupt handler
// -----------------------------------------------------------------------------
// Called automatically every 1 ms by the Cortex-M SysTick timer.
// HAL_IncTick() updates the counter used by HAL timing functions such as
// HAL_Delay().
// -----------------------------------------------------------------------------

extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
}
