#pragma once

#include "usb_lib.h"

//Add a byte to tx queue and send new data when needed
void USB_TXWrite( uint8_t c );
//Call the usb updater each loop with how much msec has passed
void USB_Update( uint8_t msec );

uint8_t USB_RXAvailable();
uint8_t USB_RXRead();

void USB_Reset();
void USB_Renumerate(void);
void USB_Interrupts_Config(void);
void Get_SerialNum(void);
