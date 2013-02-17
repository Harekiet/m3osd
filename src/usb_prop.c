#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb.h"

uint8_t Request = 0;

typedef uint32_t UNLONG;
typedef uint16_t WORD;
typedef uint8_t UCHAR;

typedef struct {
	UCHAR deviceFlags0;		// FTxxxx device type to be programmed
	UCHAR deviceFlags1;
	// Device descriptor options
	UCHAR VendorIdLo;				// 0x0403
	UCHAR VendorIdHi;
	UCHAR ProductIdLo;				// 0x6001
	UCHAR ProductIdHi;				// 0x6001
	UCHAR ReleaseLo;				// 0x200
	UCHAR ReleaseHi;				// 0x200
	UCHAR ConfigDescriptor;			// 0x80
	UCHAR MaxPower;					// 0x10
	WORD Zero;
} EepRom_t;

static const EepRom_t eepRom = {
		.VendorIdHi = 0x04,
		.VendorIdLo = 0x03,
		.ProductIdHi = 0x60,
		.ProductIdLo = 0x01,
		.ReleaseHi = 0x2,
		.ConfigDescriptor = 0x80,
		.MaxPower = 0x10,
};

//Address of currently read eeprom
static uint8_t eepromAddr;

DEVICE Device_Table = {
    EP_NUM,
    1
};

DEVICE_PROP Device_Property = {
    Virtual_Com_Port_init,
    Virtual_Com_Port_Reset,
    Virtual_Com_Port_Status_In,
    Virtual_Com_Port_Status_Out,
    Virtual_Com_Port_Data_Setup,
    Virtual_Com_Port_NoData_Setup,
    Virtual_Com_Port_Get_Interface_Setting,
    Virtual_Com_Port_GetDeviceDescriptor,
    Virtual_Com_Port_GetConfigDescriptor,
    Virtual_Com_Port_GetStringDescriptor,
    0,
    0x40                        /*MAX PACKET SIZE */
};

USER_STANDARD_REQUESTS User_Standard_Requests = {
    Virtual_Com_Port_GetConfiguration,
    Virtual_Com_Port_SetConfiguration,
    Virtual_Com_Port_GetInterface,
    Virtual_Com_Port_SetInterface,
    Virtual_Com_Port_GetStatus,
    Virtual_Com_Port_ClearFeature,
    Virtual_Com_Port_SetEndPointFeature,
    Virtual_Com_Port_SetDeviceFeature,
    Virtual_Com_Port_SetDeviceAddress
};

ONE_DESCRIPTOR Device_Descriptor = {
    (uint8_t *) Virtual_Com_Port_DeviceDescriptor,
    VIRTUAL_COM_PORT_SIZ_DEVICE_DESC
};

ONE_DESCRIPTOR Config_Descriptor = {
    (uint8_t *) Virtual_Com_Port_ConfigDescriptor,
    VIRTUAL_COM_PORT_SIZ_CONFIG_DESC
};

ONE_DESCRIPTOR String_Descriptor[4] = {
    {(uint8_t *) Virtual_Com_Port_StringLangID, VIRTUAL_COM_PORT_SIZ_STRING_LANGID}
    ,
    {(uint8_t *) Virtual_Com_Port_StringVendor, VIRTUAL_COM_PORT_SIZ_STRING_VENDOR}
    ,
    {(uint8_t *) Virtual_Com_Port_StringProduct, VIRTUAL_COM_PORT_SIZ_STRING_PRODUCT}
    ,
    {(uint8_t *) Virtual_Com_Port_StringSerial, VIRTUAL_COM_PORT_SIZ_STRING_SERIAL}
};

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : Virtual_Com_Port_init.
* Description    : Virtual COM Port Mouse init routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Virtual_Com_Port_init(void)
{

    // Update the serial number string descriptor with the data from the unique ID
    Get_SerialNum();

    pInformation->Current_Configuration = 0;

    // Connect the device
    PowerOn();

    /* Perform basic device initialization operations */
    USB_SIL_Init();

    bDeviceState = UNCONNECTED;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_Reset
* Description    : Virtual_Com_Port Mouse reset routine
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Virtual_Com_Port_Reset(void)
{
    /* Set Virtual_Com_Port DEVICE as not configured */
    pInformation->Current_Configuration = 0;

    /* Current Feature initialization */
    pInformation->Current_Feature = Virtual_Com_Port_ConfigDescriptor[7];

    /* Set Virtual_Com_Port DEVICE with the default Interface */
    pInformation->Current_Interface = 0;

    SetBTABLE(BTABLE_ADDRESS);

    /* Initialize Endpoint 0 */
    SetEPType(ENDP0, EP_CONTROL);
    SetEPTxStatus(ENDP0, EP_TX_STALL);
    SetEPRxAddr(ENDP0, ENDP0_RXADDR);
    SetEPTxAddr(ENDP0, ENDP0_TXADDR);
    Clear_Status_Out(ENDP0);
    SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
    SetEPRxValid(ENDP0);

    /* Initialize Endpoint 1 TX/RX Enabled */
    SetEPType(ENDP1, EP_BULK);

    SetEPRxAddr(ENDP1, ENDP1_RXADDR);
    SetEPTxAddr(ENDP1, ENDP1_TXADDR);

    SetEPRxCount(ENDP1, VIRTUAL_COM_PORT_DATA_SIZE );

    Clear_Status_Out(ENDP1);
    SetEPRxValid(ENDP1);
    SetEPTxStatus(ENDP1, EP_TX_NAK );

    /* Set this device to response on default address */
    SetDeviceAddress(0);

    bDeviceState = ATTACHED;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_SetConfiguration.
* Description    : Update the device state to configured.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Virtual_Com_Port_SetConfiguration(void)
{
    DEVICE_INFO *pInfo = &Device_Info;

    if (pInfo->Current_Configuration != 0) {
        /* Device configured */
        bDeviceState = CONFIGURED;
    }
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_SetConfiguration.
* Description    : Update the device state to addressed.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Virtual_Com_Port_SetDeviceAddress(void)
{
    bDeviceState = ADDRESSED;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_Status_In.
* Description    : Virtual COM Port Status In Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Virtual_Com_Port_Status_In(void)
{
#if 0
	if (Request == SET_LINE_CODING) {
        Request = 0;
    }
#endif
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_Status_Out
* Description    : Virtual COM Port Status OUT Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Virtual_Com_Port_Status_Out(void)
{
}

static uint8_t *Virtual_Com_Port_ReadEeprom(uint16_t Length)
{
    if (Length == 0) {
        pInformation->Ctrl_Info.Usb_wLength = 2;
        return NULL;
    }
//    static const uint16_t fakeRom = ~0;
//    return (uint8_t *) &fakeRom;

    return( (uint8_t *) &eepRom ) + eepromAddr;
}


static uint8_t *Virtual_Com_Port_ReadModemStatus(uint16_t Length)
{
	if (Length == 0) {
        pInformation->Ctrl_Info.Usb_wLength = 2;
        return NULL;
    }
	static const uint8_t fakeStatus[2] = {0, 0 };
    return (uint8_t*)&fakeStatus;
}


/*******************************************************************************
* Function Name  : Virtual_Com_Port_Data_Setup
* Description    : handle the data class specific requests
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT Virtual_Com_Port_Data_Setup(uint8_t RequestNo)
{
	uint8_t *(*CopyRoutine) (uint16_t);
	uint8_t requestType = pInformation->USBbmRequestType;
    CopyRoutine = NULL;

    //Vendor request device to host
    if ( requestType == 0xc0 ) {
		if ( RequestNo == SIO_READ_EEPROM_REQUEST ) {
			eepromAddr = pInformation->USBwIndexs.bw.bb0 * 2;
			if ( eepromAddr >= (sizeof( eepRom ) - 2 ) ) {
				eepromAddr = sizeof( eepRom ) - 2;
			}
			CopyRoutine = Virtual_Com_Port_ReadEeprom;
		} else if ( RequestNo == SIO_POLL_MODEM_STATUS_REQUEST ) {
			CopyRoutine = Virtual_Com_Port_ReadModemStatus;
		}
    }

    if (CopyRoutine == NULL) {
        return USB_UNSUPPORT;
    }

    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    //Init to set the length
    (*CopyRoutine) (0);
    return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_NoData_Setup.
* Description    : handle the no data class specific requests.
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT Virtual_Com_Port_NoData_Setup(uint8_t RequestNo)
{
	uint8_t requestType = pInformation->USBbmRequestType;
	//Vendor request host to device
	if ( requestType == 0x40 ) {
		if ( RequestNo == SIO_RESET_REQUEST ) {
			return USB_SUCCESS;
		}
		if ( RequestNo == SIO_SET_MODEM_CTRL_REQUEST ) {
			return USB_SUCCESS;
		}
		if ( RequestNo == SIO_SET_BAUDRATE_REQUEST ) {
			return USB_SUCCESS;
		}
		if ( RequestNo == SIO_SET_FLOW_CTRL_REQUEST ) {
			return USB_SUCCESS;
		}
		if ( RequestNo == SIO_SET_DATA_REQUEST ) {
			return USB_SUCCESS;
		}
		if ( RequestNo == SIO_SET_LATENCY_TIMER_REQUEST ) {
			return USB_SUCCESS;
		}
	}

    return USB_UNSUPPORT;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_GetDeviceDescriptor.
* Description    : Gets the device descriptor.
* Input          : Length.
* Output         : None.
* Return         : The address of the device descriptor.
*******************************************************************************/
uint8_t *Virtual_Com_Port_GetDeviceDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_GetConfigDescriptor.
* Description    : get the configuration descriptor.
* Input          : Length.
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *Virtual_Com_Port_GetConfigDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_GetStringDescriptor
* Description    : Gets the string descriptors according to the needed index
* Input          : Length.
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
uint8_t *Virtual_Com_Port_GetStringDescriptor(uint16_t Length)
{
    uint8_t wValue0 = pInformation->USBwValue0;
    if (wValue0 > 4) {
        return NULL;
    } else {
        return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
    }
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_Get_Interface_Setting.
* Description    : test the interface and the alternate setting according to the
*                  supported one.
* Input1         : uint8_t: Interface : interface number.
* Input2         : uint8_t: AlternateSetting : Alternate Setting number.
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
RESULT Virtual_Com_Port_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
    if (AlternateSetting > 0) {
        return USB_UNSUPPORT;
    } else if (Interface > 1) {
        return USB_UNSUPPORT;
    }
    return USB_SUCCESS;
}

#if 0
/*******************************************************************************
* Function Name  : Virtual_Com_Port_GetLineCoding.
* Description    : send the linecoding structure to the PC host.
* Input          : Length.
* Output         : None.
* Return         : Linecoding structure base address.
*******************************************************************************/
uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length)
{
    if (Length == 0) {
        pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
        return NULL;
    }
    return (uint8_t *) & linecoding;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_SetLineCoding.
* Description    : Set the linecoding structure fields.
* Input          : Length.
* Output         : None.
* Return         : Linecoding structure base address.
*******************************************************************************/
uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length)
{
    if (Length == 0) {
        pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
        return NULL;
    }
    return (uint8_t *) & linecoding;
}
#endif

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
