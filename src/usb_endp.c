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

//Don't really need that large buffers, usb will just stall if buffers are full
#define RX_SIZE 64
#define RX_MASK (RX_SIZE - 1 )
#define TX_SIZE 64
#define TX_MASK (TX_SIZE - 1 )

static uint8_t rxBuffer[ RX_SIZE ];
static uint8_t txBuffer[ TX_SIZE ];
static volatile uint8_t rxWrite, rxRead;
static volatile uint8_t txWrite, txRead;
//Is there an active transfer
static volatile uint8_t txActive;
//How long has the transmitter been inactive
static uint8_t txInactiveTime;
//How much data is there waiting to be put into the buffer from dma
static uint8_t rxWaiting;

void USB_Reset() {
	txActive = 0;
	rxWaiting = 0;
	txInactiveTime = 0;
}

int USB_TXStartTransfer() {
	uint8_t length = (txWrite - txRead) & TX_MASK;
	if ( !length ) {
		return 0;
	}
	//Signal an active transfer
	txActive = true;

	//Prepare the data header with modem status and line status bytes
	uint8_t header[2];
	header[ 0 ] = 1;	//B0 always 1
	header[ 1 ] = 0;

	//Only so much data available in the usb buffer
	const uint8_t maxLength = VIRTUAL_COM_PORT_DATA_SIZE - 2;
	if ( length > maxLength )
		length = maxLength;

	//Set the header in the buffer
	UserToPMABufferCopy( header, ENDP1_TXADDR, 0, 2 );
	//How much still remains beyond the tail
	uint8_t left = TX_SIZE - txRead;
	if ( length <= left ) {
		 UserToPMABufferCopy( txBuffer + txRead, ENDP1_TXADDR, 2, length );
	} else {
		 UserToPMABufferCopy( txBuffer + txRead, ENDP1_TXADDR, 2, left ) ;
		 UserToPMABufferCopy( txBuffer, ENDP1_TXADDR, 2 + left, length - left );
	}
	txRead = ( txRead + length ) & TX_MASK;

	//length + header ready to be sent
    SetEPTxCount(ENDP1, length + 2 );
    SetEPTxValid(ENDP1);
    return 1;
}

void USB_TXWrite( uint8_t c ) {
	uint8_t write = txWrite;
	uint8_t next = ( write + 1 ) & TX_MASK;
	//Delay the thread if we're overflowing the buffer
	while ( 1 ) {
		if ( next != txRead ) {
			break;
		}
		CoTickDelay( 1 );
	}
	txBuffer[ write ] = c;
	txWrite = next;
	//Check if a new transfer needs to be started
	if ( !txActive ) {
		USB_TXStartTransfer();
	}
}

void USB_TXUpdate( uint8_t msec ) {
	if ( txActive )
		return;
	txInactiveTime += msec;
	//30 milliseconds of inactivity, send a header packet?
	if ( bDeviceState == CONFIGURED && txInactiveTime > 30 ) {
		//Send an empty header
		uint8_t header[2];
		//Signal an active transfer
		txActive = true;
		//Prepare the data header with modem status and line status bytes
		header[ 0 ] = 1;	//B0 always 1
		header[ 1 ] = 0;
		//Set the header in the buffer
		UserToPMABufferCopy( header, ENDP1_TXADDR, 0, 2 );
		//length + header ready to be sent
	    SetEPTxCount(ENDP1, 2 );
	    SetEPTxValid(ENDP1);
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

void USB_RXUpdate() {
	uint8_t length = rxWaiting;
	if ( length ) {
		uint8_t remain = RX_SIZE - 1 - ((rxWrite - rxRead) & RX_MASK);
		//Is there enough room to receive this
		if ( length < remain ) {
			//How much still fits beyond the head
			uint8_t fit = RX_SIZE - rxWrite;
			if ( length <= fit ) {
				PMAToUserBufferCopy( rxBuffer + rxWrite, ENDP2_RXADDR, 1, length);
			} else {
				PMAToUserBufferCopy( rxBuffer + rxWrite, ENDP2_RXADDR, 1, fit ) ;
				PMAToUserBufferCopy( rxBuffer, ENDP2_RXADDR,  1 + fit, length - fit );
			}
			rxWrite = ( rxWrite + length ) & RX_MASK;
			rxWaiting = 0;
			/* Enable the receive of new data on EP2 */
			SetEPRxValid(ENDP2);
		}
	}
}


void USB_Update( uint8_t msec ) {
	//Update the read buffer, signal that we are ready to receive new data
	USB_RXUpdate();
	//Update the transmitter
	USB_TXUpdate( msec );
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
    	PMAToUserBufferCopy( &rxWaiting, ENDP2_RXADDR, 0, 1 );
    	rxWaiting >>= 2;
    	if ( rxWaiting ) {
    		USB_RXUpdate();
    		return;
    	}
    }
    //Invalid dataset, just setup to receive new data
    SetEPRxValid(ENDP2);
}

void EP1_IN_Callback(void) {
	if ( !USB_TXStartTransfer() ) {
		txActive = false;
		txInactiveTime = 0;
	}
}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
