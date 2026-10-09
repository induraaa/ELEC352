// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#include "stm32f4xx_hal.h"   // STM32F4 Hardware Abstraction Layer


// -----------------------------------------------------------------------------
// SysTick interrupt handler
// -----------------------------------------------------------------------------
// Called automatically every 1 ms by the Cortex-M SysTick timer.
// HAL_IncTick() updates the HAL millisecond counter used by functions such as
// HAL_Delay().
// -----------------------------------------------------------------------------

extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
}
