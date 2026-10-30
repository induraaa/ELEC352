// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#ifndef SERIAL_H
#define SERIAL_H

// -----------------------------------------------------------------------------
// Serial communication interface
// -----------------------------------------------------------------------------
//
// Serial_Init() configures USART3 and the associated GPIO pins. All hardware
// details remain private to serial.cpp. printf() output is redirected to this
// serial connection in serial.cpp.
// -----------------------------------------------------------------------------

// Initialise the serial communication hardware.
void Serial_Init();

#endif  // SERIAL_H
