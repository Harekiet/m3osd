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

static uint8_t WorkBuffer[VIRTUAL_COM_PORT_DATA_SIZE];

void EP1_OUT_Callback(void)
{
    uint16_t USB_Rx_Cnt;

    /* Get the received data buffer and update the counter */
    USB_Rx_Cnt = USB_SIL_Read(EP1_OUT, WorkBuffer );

    /*
    * Byte 0: Line Status
    *
    * Offset	Description
    * B0	Reserved - must be 1
    * B1	Reserved - must be 0
    * B2..7	Length of message - (not including Byte 0)
    **/

    /* USB data will be immediately processed, this allow next USB traffic being
       NAKed till the end of the USART Xfer */

    if ( USB_Rx_Cnt >= 2 ) {
    	uint8_t length = WorkBuffer[0] >> 2;
    	if ( (length + 1 == USB_Rx_Cnt ) ) {
    		OnUsbDataRx(WorkBuffer + 1, length );

    		//Send the same thing to the output
    		UserToPMABufferCopy( WorkBuffer + 1, ENDP1_TXADDR, 1);
    	    SetEPTxCount(ENDP1, 1);
    	    SetEPTxValid(ENDP1);
    	}
    }

    /* Enable the receive of data on EP1 */
    SetEPRxValid(ENDP1);
}

void EP1_IN_Callback(void)
{

	WorkBuffer[0] = WorkBuffer[0];


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


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
