// -----------------------------------------------------------------------------
// Author: Dr Ian S. Howard
// School of Engineering, Computing and Mathematics
// University of Plymouth
// -----------------------------------------------------------------------------

#ifndef SERIAL_H
#define SERIAL_H


// Configure USART3 and redirect printf() output to the ST-LINK Virtual COM
// Port. All hardware details remain private to serial.cpp.
void Serial_Init();


#endif  // SERIAL_H
