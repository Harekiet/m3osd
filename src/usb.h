#pragma once

#include "usb_lib.h"

#define USART_RX_DATA_SIZE   (256)
void OnUsbDataRx(uint8_t * dataIn, uint8_t length);
void OnUsbDataTx(uint8_t * dataOut, uint8_t * length);
void USB_Renumerate(void);
void USB_Interrupts_Config(void);
void Get_SerialNum(void);
