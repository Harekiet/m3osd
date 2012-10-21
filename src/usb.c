#include "board.h"
#include "uart.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb.h"
#include "gps.h"

uint8_t USART_Rx_Buffer[USART_RX_DATA_SIZE];
uint32_t USART_Rx_ptr_in;
uint32_t USART_Rx_ptr_out;
uint32_t USART_Rx_length = 0;

void OnUsbDataRx(uint8_t * dataIn, uint8_t length)
{
    int i;

    if (gpsData.mode == MODE_PASSTHROUGH) {
        LED1_ON;
        for (i = 0; i < length; i++)
            uartWrite(gpsData.serial, dataIn[i]);
        LED1_OFF;
    }
}

volatile int usb_len = 0;

void OnUsbDataTx(uint8_t * dataOut, uint8_t * length)
{
    int i = 0;

    if (gpsData.mode == MODE_PASSTHROUGH) {
        while (uartAvailable(gpsData.serial)) {
            LED1_ON;
            dataOut[i++] = uartRead(gpsData.serial);
        }
        *length = i;
        usb_len = i;
        LED1_OFF;
    }
}

void USB_Renumerate(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Force USB reset and power-down (this will also release the USB pins for direct GPIO control) */
    _SetCNTR(CNTR_FRES | CNTR_PDWN);

    /* Using a "dirty" method to force a re-enumeration: */
    /* Force DPM (Pin PA12) low for ca. 10 mS before USB Tranceiver will be enabled */
    /* This overrules the external Pull-Up at PA12, and at least Windows & MacOS will enumerate again */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    timerDelay(50);

    /* Release power-down, still hold reset */
    _SetCNTR(CNTR_PDWN);
    timerDelay(5);

    /* CNTR_FRES = 0 */
    _SetCNTR(0);
    /* Clear pending interrupts */
    _SetISTR(0);

    /* Configure USB clock */
    /* USBCLK = PLLCLK / 1.5 */
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
    /* Enable USB clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}



void USB_Interrupts_Config(void)
{
    NVIC_InitTypeDef nvic;

    nvic.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 3;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

/*******************************************************************************
 * Description    : Convert Hex 32Bits value into char.
 *******************************************************************************/
static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len)
{
    uint8_t idx = 0;

    for (idx = 0; idx < len; idx++) {
        if (((value >> 28)) < 0xA) {
            pbuf[2 * idx] = (value >> 28) + '0';
        } else {
            pbuf[2 * idx] = (value >> 28) + 'A' - 10;
        }

        value = value << 4;

        pbuf[2 * idx + 1] = 0;
    }
}

/*******************************************************************************
 * Description    : Create the serial number string descriptor.
 *******************************************************************************/
void Get_SerialNum(void)
{
    uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

    Device_Serial0 = *(volatile uint32_t *) (0x1FFFF7E8);
    Device_Serial1 = *(volatile uint32_t *) (0x1FFFF7EC);
    Device_Serial2 = *(volatile uint32_t *) (0x1FFFF7F0);

    Device_Serial0 += Device_Serial2;

    if (Device_Serial0 != 0) {
        IntToUnicode(Device_Serial0, &Virtual_Com_Port_StringSerial[2], 8);
        IntToUnicode(Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
    }
}
