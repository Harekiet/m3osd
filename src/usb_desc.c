#include "usb_lib.h"
#include "usb_desc.h"

/* USB Standard Device Descriptor
 * Copied from simple uart ttl convertor
*/
const uint8_t Virtual_Com_Port_DeviceDescriptor[] = {
    0x12,                       /* bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /* bDescriptorType */
    0x10,
    0x01,                       /* bcdUSB = 1.10 */
    0x00,                       /* bDeviceClass: 0 */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    0x40,                       /* bMaxPacketSize0 */
    0x03,
    0x04,                       /* idVendor = 0x0403 */
    0x01,
    0x60,                       /* idProduct = 0x6001 */
    0x00,
    0x01,                       /* bcdDevice = 1.00 */
    1,                          /* Index of string descriptor describing manufacturer */
    2,                          /* Index of string descriptor describing product */
    3,                          /* Index of string descriptor describing the device's serial number */
    0x01                        /* bNumConfigurations */
};

const uint8_t Virtual_Com_Port_ConfigDescriptor[ VIRTUAL_COM_PORT_SIZ_CONFIG_DESC ] = {
    /*Configuration Descriptor */
    0x09,                       /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE,  /* bDescriptorType: Configuration */
    VIRTUAL_COM_PORT_SIZ_CONFIG_DESC,   /* wTotalLength:no of returned bytes */
    0x00,
    0x01,                       /* bNumInterfaces: 1 interface */
    0x01,                       /* bConfigurationValue: Configuration value */
    0x00,                       /* iConfiguration: Index of string descriptor describing the configuration */
    0xC0,                       /* bmAttributes: self powered */
    0x32,                       /* MaxPower 0 mA */
    /*Data class interface descriptor */
    0x09,                       /* bLength: Endpoint Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType: */
    0x00,                       /* bInterfaceNumber: Number of Interface */
    0x00,                       /* bAlternateSetting: Alternate setting */
    0x02,                       /* bNumEndpoints: Two endpoints used */
    0xff,                       /* bInterfaceClass: 0xff */
    0x00,                       /* bInterfaceSubClass: */
    0x00,                       /* bInterfaceProtocol: */
    0x00,                       /* iInterface: */
    /* Endpoint 0x81 Descriptor */
    /* UART->HOST */
    0x07,                       /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType: Endpoint */
    0x81,                       /* bEndpointAddress: (IN1) */
    0x02,                       /* bmAttributes: Bulk */
    VIRTUAL_COM_PORT_DATA_SIZE, /* wMaxPacketSize: */
    0x00,
    0x00,                        /* bInterval */
    /* Endpoint 0x1 Descriptor */
    /* HOST->UART */
    0x07,                       /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType: Endpoint */
    0x01,                       /* bEndpointAddress: (OUT1) */
    0x02,                       /* bmAttributes: Bulk */
    VIRTUAL_COM_PORT_DATA_SIZE, /* wMaxPacketSize: */
    0x00,
    0x00,                       /* bInterval: ignore for Bulk transfer */
};

/* USB String Descriptors */
const uint8_t Virtual_Com_Port_StringLangID[VIRTUAL_COM_PORT_SIZ_STRING_LANGID] = {
    VIRTUAL_COM_PORT_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04                        /* LangID = 0x0409: U.S. English */
};

const uint8_t Virtual_Com_Port_StringVendor[VIRTUAL_COM_PORT_SIZ_STRING_VENDOR] = {
    VIRTUAL_COM_PORT_SIZ_STRING_VENDOR, /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE, /* bDescriptorType */
    /* Manufacturer: "tomeko.net" */
    'H', 0, 'I', 0, 'K', 0, 'A', 0, 'R', 0, 'I', 0, 'S', 0, 'O', 0,
    'F', 0, 'T', 0, ' ', 0, 'L', 0, 'L', 0, 'P', 0
};

const uint8_t Virtual_Com_Port_StringProduct[VIRTUAL_COM_PORT_SIZ_STRING_PRODUCT] = {
    VIRTUAL_COM_PORT_SIZ_STRING_PRODUCT,        /* bLength */
    USB_STRING_DESCRIPTOR_TYPE, /* bDescriptorType */
    /* Product name: "miniscope v2c" */
    'A', 0, 'f', 0, 'r', 0, 'o', 0, 'O', 0, 'S', 0, 'D', 0, '3', 0,
    '2', 0
};

uint8_t Virtual_Com_Port_StringSerial[VIRTUAL_COM_PORT_SIZ_STRING_SERIAL] = {
    VIRTUAL_COM_PORT_SIZ_STRING_SERIAL, /* bLength */
    USB_STRING_DESCRIPTOR_TYPE, /* bDescriptorType */
    'U', 0, 'S', 0, 'B', 0, 'D', 0, 'M', 0, 'X', 0, '_', 0
};

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
