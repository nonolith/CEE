#include "core/cpu/cpu.h"
#include "core/pmu/pmu.h"
#include "core/adc/adc.h"
#include "core/gpio/gpio.h"
#include "core/systick/systick.h"

#include "lufa_usb/USB.h"

#define LED_PORT 0
#define LED_PIN 7

#define DataEndpoint 1

int main(void){
	cpuInit();                                // Configure the CPU
  	gpioInit();                               // Enable GPIO
  	pmuInit();                                // Configure power management
  	adcInit(); 
	systickInit(1);						// Systick = 1ms
	USB_Init();
	
	gpioSetDir(LED_PORT, LED_PIN, gpioDirection_Output);
	
	while (1){
		USB_USBTask();
		
		Endpoint_SelectEndpoint(DataEndpoint);
		
		if (Endpoint_IsINReady()){
			Endpoint_SelectEndpoint(DataEndpoint);
			Endpoint_Write_32_LE(adcRead(0)|adcRead(1)<<16);
			Endpoint_ClearIN();
		}
		
		Endpoint_SelectEndpoint(2);
		if (Endpoint_IsOUTReceived() && Endpoint_IsINReady()){
			uint8_t i2c_out_buf[64];
			uint32_t i2c_out_buf_size;
			uint8_t i2c_in_buf[64];
			uint32_t i2c_in_buf_size=0;
			uint32_t i=0;
		
			i2c_out_buf_size = Endpoint_Read_buf(&i2c_out_buf, sizeof(i2c_out_buf));
			Endpoint_ClearOUT();
			
			for (i=0; i<i2c_out_buf_size; i++){
				i2c_in_buf[i2c_in_buf_size++] = i2c_out_buf[i]+1;
			}
			
			Endpoint_write_buf(i2c_in_buf, i2c_in_buf_size);
			Endpoint_ClearIN();
		}
	}

	return 0;
}


typedef struct
		{
			USB_Descriptor_Configuration_Header_t Config;
			USB_Descriptor_Interface_t            CEEInterface;
			USB_Descriptor_Endpoint_t             Data_endpoint;
			USB_Descriptor_Endpoint_t             Tst_endpoint;
			USB_Descriptor_Endpoint_t             Tst2_endpoint;
		} USB_Descriptor_Configuration_t;


/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
const USB_Descriptor_Device_t CEE_DeviceDescriptor =
{
	.Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

	.USBSpecification       = VERSION_BCD(01.10),
	.Class                  = USB_CSCP_VendorSpecificClass,
	.SubClass               = USB_CSCP_NoDeviceSubclass,
	.Protocol               = USB_CSCP_NoDeviceProtocol,

	.Endpoint0Size          = 64,

	.VendorID               = 0x9999,
	.ProductID              = 0xA023,
	.ReleaseNumber          = VERSION_BCD(02.00),

	.ManufacturerStrIndex   = 0x01,
	.ProductStrIndex        = 0x02,
	.SerialNumStrIndex      = 0x03,

	.NumberOfConfigurations = 1
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
const USB_Descriptor_Configuration_t CEE_ConfigurationDescriptor =
{
	.Config =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

			.TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
			.TotalInterfaces        = 1,

			.ConfigurationNumber    = 1,
			.ConfigurationStrIndex  = NO_DESCRIPTOR,

			.ConfigAttributes       = USB_CONFIG_ATTR_BUSPOWERED,

			.MaxPowerConsumption    = USB_CONFIG_POWER_MA(500)
		},

	.CEEInterface =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

			.InterfaceNumber        = 0,
			.AlternateSetting       = 0,

			.TotalEndpoints         = 3,

			.Class                  = USB_CSCP_VendorSpecificClass,
			.SubClass               = 0x00,
			.Protocol               = 0x00,

			.InterfaceStrIndex      = NO_DESCRIPTOR
		},
	.Data_endpoint = 
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

			.EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_IN | DataEndpoint),
			.Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
			.EndpointSize           = 64,
			.PollingIntervalMS      = 0
		},
	.Tst_endpoint = 
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

			.EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_IN | 2),
			.Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
			.EndpointSize           = 64,
			.PollingIntervalMS      = 0
		},
	.Tst2_endpoint = 
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

			.EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_OUT | 2),
			.Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
			.EndpointSize           = 64,
			.PollingIntervalMS      = 0
		}
};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
const USB_Descriptor_String_t CEE_LanguageString =
{
	.Header                 = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},

	.UnicodeString          = {LANGUAGE_ID_ENG}
};

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t CEE_ManufacturerString =
{
	.Header                 = {.Size = USB_STRING_LEN(13), .Type = DTYPE_String},

	.UnicodeString          = L"Nonolith Labs"
};

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t CEE_ProductString =
{
	.Header                 = {.Size = USB_STRING_LEN(3), .Type = DTYPE_String},

	.UnicodeString          = L"CEE"
};

/** Serial number string. This is a Unicode string containing the device's unique serial number, expressed as a
 *  series of uppercase hexadecimal digits.
 */
const USB_Descriptor_String_t CEE_SerialString =
{
	.Header                 = {.Size = USB_STRING_LEN(5), .Type = DTYPE_String},

	.UnicodeString          = L"00001"
};

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void** const DescriptorAddress)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);

	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
		case DTYPE_Device:
			Address = &CEE_DeviceDescriptor;
			Size    = sizeof(USB_Descriptor_Device_t);
			break;
		case DTYPE_Configuration:
			Address = &CEE_ConfigurationDescriptor;
			Size    = sizeof(USB_Descriptor_Configuration_t);
			break;
		case DTYPE_String:
			switch (DescriptorNumber)
			{
				case 0x00:
					Address = &CEE_LanguageString;
					Size    = pgm_read_byte(&CEE_LanguageString.Header.Size);
					break;
				case 0x01:
					Address = &CEE_ManufacturerString;
					Size    = pgm_read_byte(&CEE_ManufacturerString.Header.Size);
					break;
				case 0x02:
					Address = &CEE_ProductString;
					Size    = pgm_read_byte(&CEE_ProductString.Header.Size);
					break;
				case 0x03:
					Address = &CEE_SerialString;
					Size    = pgm_read_byte(&CEE_SerialString.Header.Size);
					break;
			}

			break;
	}

	*DescriptorAddress = Address;
	return Size;
}

void EVENT_USB_Device_ConfigurationChanged(){
	Endpoint_ConfigureEndpoint(DataEndpoint, 0, 0, 64,0);
	Endpoint_ConfigureEndpoint(2, 0, 0, 64,0);
}

void EVENT_USB_Device_ControlRequest(void){
	if (USB_ControlRequest.bmRequestType & (REQTYPE_CLASS | REQREC_INTERFACE)){
		switch (USB_ControlRequest.bRequest){
			case 0x01:
				Endpoint_ClearSETUP();
				Endpoint_prepare_write(0);
				Endpoint_complete_write();
				gpioSetValue (LED_PORT, LED_PIN, USB_ControlRequest.wValue);
				break;
		}
	}
}

