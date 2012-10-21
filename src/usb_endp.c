/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
 * File Name          : usb_endp.c
 * Author             : MCD Application Team
 * Version            : V3.3.0
 * Date               : 21-March-2011
 * Description        : Endpoint routines
 ********************************************************************************
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *******************************************************************************/

#include "board.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_mem.h"
#include "usb_istr.h"
#include "usb_pwr.h"
#include "usb.h"

uint8_t USB_Rx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];

uint8_t USB_Tx_State = 0;

extern uint8_t USART_Rx_Buffer[];
extern uint32_t USART_Rx_ptr_in;
extern uint32_t USART_Rx_ptr_out;
extern uint32_t USART_Rx_length;

void EP3_OUT_Callback(void)
{
    uint16_t USB_Rx_Cnt;

    /* Get the received data buffer and update the counter */
    USB_Rx_Cnt = USB_SIL_Read(EP3_OUT, USB_Rx_Buffer);

    /* USB data will be immediately processed, this allow next USB traffic being
       NAKed till the end of the USART Xfer */
    OnUsbDataRx(USB_Rx_Buffer, USB_Rx_Cnt);

    /* Enable the receive of data on EP3 */
    SetEPRxValid(ENDP3);
}

void EP1_IN_Callback(void)
{
#if 0
    uint16_t USB_Tx_ptr;
    uint16_t USB_Tx_length;

    if (USB_Tx_State == 1) {
        if (USART_Rx_length == 0) {
            USB_Tx_State = 0;
        } else {
            // assumption: always transmitting multiply of endpoint size
            if (USART_Rx_ptr_out == USART_RX_DATA_SIZE)
                USART_Rx_ptr_out = 0;
            USB_Tx_ptr = USART_Rx_ptr_out;
            USB_Tx_length = VIRTUAL_COM_PORT_DATA_SIZE;

            USART_Rx_ptr_out += VIRTUAL_COM_PORT_DATA_SIZE;
            USART_Rx_length -= VIRTUAL_COM_PORT_DATA_SIZE;

            UserToPMABufferCopy(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
            SetEPTxCount(ENDP1, USB_Tx_length);
            SetEPTxValid(ENDP1);
        }
    }
#endif
}

// Start Of Frame (SOF) callback
void SOF_Callback(void)
{
    static uint32_t FrameCount = 0;
    uint8_t len = 0;

    if (bDeviceState == CONFIGURED) {
        if (FrameCount++ == 5) {
            /* Reset the frame counter */
            FrameCount = 0;
            /* Check the data to be sent through IN pipe */
            OnUsbDataTx(USART_Rx_Buffer, &len);
        }
    }

    if (len) {
        UserToPMABufferCopy(USART_Rx_Buffer, ENDP1_TXADDR, len);
        SetEPTxCount(ENDP1, len);
        SetEPTxValid(ENDP1);
    }
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
