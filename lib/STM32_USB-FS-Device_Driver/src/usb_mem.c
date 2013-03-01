/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_mem.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : Utility functions for memory transfers to/from PMA
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
#ifndef STM32F10X_CL

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : UserToPMABufferCopy
* Description    : Copy a buffer from user memory area to packet memory area (PMA)
* Input          : - pbUsrBuf: pointer to user memory area.
*                  - wPMABufAddr: address into PMA.
*                  - wNBytes: no. of bytes to be copied.
* Output         : None.
* Return         : None	.
*******************************************************************************/
void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wOffset, uint16_t wNBytes)
{
	//Skip 0 counts
	if ( !wNBytes )
		return;
	uint16_t *pdwVal = (uint16_t *)( (wPMABufAddr + (wOffset & ~0x1 ) ) * 2 + PMAAddr);
	//Handle the first unaligned one
	if ( wOffset & 1 ) {
		*pdwVal = ( *pdwVal & 0x00ff ) | ( *pbUsrBuf << 8 );
		pdwVal += 2;
		pbUsrBuf += 1;
		wNBytes--;
	}
	//Handle the remaining ones
	for ( ; wNBytes > 1; wNBytes -= 2 ) {
		*pdwVal =  *(uint16_t*) pbUsrBuf;
		pbUsrBuf += 2;
		pdwVal += 2;
	}
	//Handle any remaining byte
	if ( wNBytes ) {
		*pdwVal = ( *pdwVal & 0xff00 ) | ( *pbUsrBuf );
	}
}

/*******************************************************************************
* Function Name  : PMAToUserBufferCopy
* Description    : Copy a buffer from user memory area to packet memory area (PMA)
* Input          : - pbUsrBuf    = pointer to user memory area.
*                  - wPMABufAddr = address into PMA.
*                  - wNBytes     = no. of bytes to be copied.
* Output         : None.
* Return         : None.
*******************************************************************************/
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wOffset, uint16_t wNBytes)
{
	//Skip 0 counts
	if ( !wNBytes )
		return;
	uint32_t *pdwVal = (uint32_t *)( (wPMABufAddr + (wOffset & ~0x1 ) ) * 2 + PMAAddr);
	//First handle an unaligned read
	if ( wOffset & 1 ) {
		*pbUsrBuf++ = (*pdwVal++) >> 8;
		wNBytes--;
	}
	//Handle aligned any aligned bytes
	for ( ; wNBytes > 1; wNBytes -= 2 ) {
		*(uint16_t*)pbUsrBuf++ = *pdwVal++;
		pbUsrBuf++;
	}
	//Last byte, read 32 bits and write the low 8 bits as the last result
	if ( wNBytes ) {
		*pbUsrBuf = *pdwVal;
	}
}

#endif /* STM32F10X_CL */
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
