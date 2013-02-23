#pragma once

#include "usb_lib.h"

void USB_TXWrite( uint8_t c );
uint8_t USB_RXAvailable();
uint8_t USB_RXRead();

void USB_Renumerate(void);
void USB_Interrupts_Config(void);
void Get_SerialNum(void);
