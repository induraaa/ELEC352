// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#ifndef BOARD_H
#define BOARD_H

// -----------------------------------------------------------------------------
// Board initialisation interface
// -----------------------------------------------------------------------------
//
// Board_Init() performs the basic hardware setup required before the main
// application starts. Keeping this setup in a separate board-support module
// prevents main.cpp from becoming cluttered with low-level configuration code.
// -----------------------------------------------------------------------------

// Initialise the HAL and serial output. After this call, printf() is available.
void Board_Init();

#endif  // BOARD_H
