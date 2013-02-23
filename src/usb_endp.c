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

#define RX_SIZE 128
#define RX_MASK (RX_SIZE - 1 )
#define TX_SIZE 64
#define TX_MASK (TX_SIZE - 1 )

static uint8_t rxBuffer[ RX_SIZE ];
static uint8_t txBuffer[ TX_SIZE ];
static volatile uint8_t rxWrite, rxRead;
static volatile uint8_t txWrite, txRead;
//Is there an active transfer
static volatile uint8_t txActive;
//How much data is there waiting to be put into the buffer
static uint8_t rxWaiting;


void USB_TXStartTransfer() {
	uint8_t length = (txWrite - txRead) & TX_MASK;
	if ( !length ) {
		txActive = false;
		return;
	}
	uint8_t header[2];
	//Signal an active transfer
	txActive = true;
	//Only so much data available in the usb buffer
	const uint8_t maxLength = VIRTUAL_COM_PORT_DATA_SIZE - sizeof( header );
	if ( length > maxLength )
		length = maxLength;
	//Prepare the data header with modem status and line status bytes
	header[ 0 ] = 1;	//B0 always 1
	header[ 1 ] = 0;
	//Set the header in the buffer
	UserToPMABufferCopy( header, ENDP1_TXADDR, 2 );
	//How much still remains beyond the tail
	uint8_t left = TX_SIZE - txRead;
	if ( length <= left ) {
		 UserToPMABufferCopy( txBuffer + txRead, ENDP1_TXADDR + 2, length );
	} else {
		 UserToPMABufferCopy( txBuffer + txRead, ENDP1_TXADDR + 2, left ) ;
		 UserToPMABufferCopy( txBuffer, ENDP1_TXADDR + 2 + left, length - left );
	}
	txRead = ( txRead + length ) & TX_MASK;
	//length + header ready to be sent
    SetEPTxCount(ENDP1, length + 2 );
    SetEPTxValid(ENDP1);
}

void USB_TXWrite( uint8_t c ) {
	uint8_t next;
	//Delay the thread if we're overflowing the buffer
	while ( 1 ) {
		next = ( txWrite + 1 ) & TX_MASK;
		if ( next != txRead ) {
			break;
		}
		return;
		CoTickDelay( 1 );
	}
	txBuffer[ next ] = c;
	txWrite = next;
	//Check if a new transfer needs to be started
	if ( !txActive ) {
		USB_TXStartTransfer();
	}
}

uint8_t USB_RXAvailable() {
	return (rxWrite - rxRead) & RX_MASK;
}

uint8_t USB_RXRead() {
	if ( rxRead != rxWrite ) {
		uint8_t c = rxBuffer[ rxRead ];
		rxRead = ( rxRead + 1 ) & RX_MASK;
		return c;
	}
	return 0;
}

void USB_RXUpdateWaiting() {
	uint8_t length = rxWaiting;
	if ( length ) {
		uint8_t remain = RX_SIZE - 1 - ((rxWrite - rxRead) & RX_MASK);
		//Is there enough room to receive this
		if ( length < remain ) {
			//How much still fits beyond the head
			uint8_t fit = RX_SIZE - rxWrite;
			if ( length <= fit ) {
				PMAToUserBufferCopy( rxBuffer + rxWrite, ENDP2_RXADDR + 1, length);
			} else {
				PMAToUserBufferCopy( rxBuffer + rxWrite, ENDP2_RXADDR + 1, fit ) ;
				PMAToUserBufferCopy( rxBuffer, ENDP2_RXADDR + 1 + fit, length - fit );
			}
			rxWrite = ( rxWrite + length ) & RX_MASK;
			rxWaiting = 0;
			/* Enable the receive of new data on EP2 */
			SetEPRxValid(ENDP2);
		}
	}
}


void EP2_OUT_Callback(void) {
	//How much data is waiting
    uint16_t count = GetEPRxCount( ENDP2);
    /*
    * Byte 0: Line Status
    *
    * Offset	Description
    * B0	Reserved - must be 1
    * B1	Reserved - must be 0
    * B2..7	Length of message - (not including Byte 0)
    **/
    if ( count >= 2 ) {
    	uint8_t test[2];
    	PMAToUserBufferCopy( test, ENDP2_RXADDR, 2 );
    	PMAToUserBufferCopy( &rxWaiting, ENDP2_RXADDR, 1 );
    	rxWaiting >>= 2;
    	if ( rxWaiting ) {
    		USB_RXUpdateWaiting();
    		return;
    	}
    }
    //Invalid dataset, just setup to receive new data
    SetEPRxValid(ENDP2);
}

void EP1_IN_Callback(void) {
	USB_TXStartTransfer();
}

void SOF_Callback(void)
{
  static uint32_t FrameCount = 0;

  if(bDeviceState == CONFIGURED)
  {
    if ((FrameCount++ & 15) == 0 )
    {
//    	USB_TXWrite( 'c' );
    }
  }
}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
