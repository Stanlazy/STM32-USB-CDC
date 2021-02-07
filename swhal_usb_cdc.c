#include "swhal_usb_cdc.h"

#define  USB_DESC_SUBTYPE_CDC_HEADER                    0x00U
#define  USB_DESC_SUBTYPE_CDC_CM                        0x01U
#define  USB_DESC_SUBTYPE_CDC_ACM                       0x02U
#define  USB_DESC_SUBTYPE_CDC_UNION                     0x06U

typedef struct __attribute__((packed)) {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubtype;
	uint16_t bcdCDC;
} USB_CDC_Header_Descriptor_TypeDef;

typedef struct __attribute__((packed)) {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubtype;
	struct __attribute__((packed)) {
		uint8_t call_self_mng        :1;
		uint8_t call_mng_itf         :1;
		uint8_t reserved_as_0        :6;
	} bmCapabilities;
	uint8_t  bDataInterface;
} USB_CDC_CM_Descriptor_TypeDef;

typedef struct __attribute__((packed)) {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubtype;
	struct __attribute__((packed)) {
		uint8_t Comm_Feature         :1;
		uint8_t Line_Coding          :1;
		uint8_t Send_Break           :1;
		uint8_t Network_Connection   :1;
		uint8_t reserved_as_0        :4;
	} bmCapabilities;
} USB_CDC_ACM_Descriptor_TypeDef;

typedef struct __attribute__((packed)) {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubtype;
	uint8_t  bControlInterface;
	uint8_t  bSubordinateInterface0;
} USB_CDC_Union_Descriptor_TypeDef;

static void SWHAL_USB_CDC_Transmit_Cplt_Callback(PCD_HandleTypeDef* hpcd, uint8_t epnum){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	SWHAL_USB_CDC_HandleTypeDef* swcdc = swpcd->pData;
	if(epnum){
		if(swcdc->TX_Cplt_Callback)(*swcdc->TX_Cplt_Callback)(hpcd, epnum);
	}
}

static void SWHAL_USB_CDC_Receive_Cplt_Callback(PCD_HandleTypeDef* hpcd, uint8_t epnum, uint16_t alen){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	SWHAL_USB_CDC_HandleTypeDef* swcdc = swpcd->pData;
	if(epnum){
		if(swcdc->RX_Cplt_Callback)(*swcdc->RX_Cplt_Callback)(hpcd, epnum, alen);
	} else {
		USB_Request_Packet_TypeDef* request = hpcd->Setup;
		if(request->bmRequestType.recipient == USB_REQUEST_RECIPIENT_INTERFACE){
			if(request->bmRequestType.type == USB_REQUEST_TYPE_CLASS){
				if(request->bRequest == USB_CDC_SET_LINE_ENCODING){
					uint8_t retval = 0;
					if(swcdc->UART_Config_Callback) retval = (*swcdc->UART_Config_Callback)(hpcd, (request->wIndex/2)+1, &swcdc->uart_cfg);
					if(retval >= 0) SWHAL_USB_PCD_Transmit(hpcd, 0x00, NULL, 0);
					else            SWHAL_USB_PCD_Stall_EP0(hpcd);
				}
			}
		}
	}
}

static int8_t SWHAL_USB_CDC_NStd_Setup(PCD_HandleTypeDef *hpcd, USB_Request_Packet_TypeDef* request){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	SWHAL_USB_CDC_HandleTypeDef* swcdc = swpcd->pData;
	int8_t retval = -1;
	if(request->bmRequestType.recipient == USB_REQUEST_RECIPIENT_INTERFACE){
		if(request->bmRequestType.type == USB_REQUEST_TYPE_CLASS){
			retval = 0;
			switch (request->bRequest){
				case USB_CDC_SET_LINE_ENCODING:{
					SWHAL_USB_PCD_ReceiveL(hpcd, 0x00, &(swcdc->uart_cfg), sizeof(USB_CDC_Line_Encoding_Typedef));
				}break;
				case USB_CDC_GET_LINE_ENCODING:{
					SWHAL_USB_PCD_Transmit(hpcd, 0x80, &(swcdc->uart_cfg), sizeof(USB_CDC_Line_Encoding_Typedef));
				}break;
				case USB_CDC_SET_CTRL_LINE:{
					swcdc->ctrl_cfg.u16 = request->wValue;
					uint8_t retval = 0;
					if(swcdc->RTS_State_Callback) retval = (*swcdc->RTS_State_Callback)(hpcd, (request->wIndex/2)+1, &swcdc->ctrl_cfg);
					if(retval >= 0) SWHAL_USB_PCD_Transmit(hpcd, 0x00, NULL, 0);
					else            SWHAL_USB_PCD_Stall_EP0(hpcd);
				}break;
				default:{
					retval = -1;
				}break;
			}
		}
	}
	return retval;
}

static SWHAL_USB_PCD_Desc_Typedef* SWHAL_USB_CDC_Get_Desc(PCD_HandleTypeDef *hpcd, uint8_t bDescType, uint8_t iDescIdx){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	SWHAL_USB_CDC_HandleTypeDef* swcdc = swpcd->pData;
	(void)swcdc;
	static const __ALIGNED(4) USB_Device_Descriptor_TypeDef dev_desc_c = {
		.bLength            = sizeof(USB_Device_Descriptor_TypeDef),
		.bDescriptorType    = USB_DESC_TYPE_DEVICE,
		.bcdUSB             = 0x0200,
		.bDeviceClass       = 0xEF, //CLASS IAD
		.bDeviceSubClass    = 0x02, //CLASS IAD
		.bDeviceProtocol    = 0x01, //CLASS IAD
		.bMaxPacketSize     = 64,
		.idVendor           = 0x2333,
		.idProduct          = 0x2333,
		.bcdDevice          = 0x0200,
		.iManufacturer      = 0,
		.iProduct           = 0,
		.iSerialNumber      = 0,
		.bNumConfigurations = 1
	};
	static const SWHAL_USB_PCD_Desc_Typedef dev_desc = {
		.desc   = &dev_desc_c,
		.length = sizeof(dev_desc_c)
	};
	
	#define CDC_FUNTION_DESC_STRUCT(i)                                           \
		USB_Interface_Descriptor_TypeDef             s##i##_itf_ctrl_desc; __NL__\
		USB_CDC_Header_Descriptor_TypeDef            s##i##_header_desc;   __NL__\
		USB_CDC_CM_Descriptor_TypeDef                s##i##_cm_desc;       __NL__\
		USB_CDC_ACM_Descriptor_TypeDef               s##i##_acm_desc;      __NL__\
		USB_CDC_Union_Descriptor_TypeDef             s##i##_union_desc;    __NL__\
		USB_Endpoint_Descriptor_TypeDef              s##i##_ep_ctrl;       __NL__\
		USB_Interface_Descriptor_TypeDef             s##i##_itf_data_desc; __NL__\
		USB_Endpoint_Descriptor_TypeDef              s##i##_epout_desc;    __NL__\
		USB_Endpoint_Descriptor_TypeDef              s##i##_epin_desc      __NL__
	
	#define CDC_FUNCTION_DESC_STRUCT_WITH_IAD(i)                           __NL__\
		USB_Interface_Association_Descriptor_TypeDef s##i##_iad_desc;      __NL__\
		CDC_FUNTION_DESC_STRUCT(i)                                         __NL__
	
	#define CDC_FUNTION_CONTENT(i)                                                       \
		.s##i##_itf_ctrl_desc = {                                                  __NL__\
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),  __NL__\
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,                   __NL__\
			.bInterfaceNumber         = (i-1)*2,                                   __NL__\
			.bAlternateSetting        = 0,                                         __NL__\
			.bNumEndpoints            = 1,                                         __NL__\
			.bInterfaceClass          = 0x02, /*CLASS CDC*/                        __NL__\
			.bInterfaceSubClass       = 0x02, /*Abstract Control Model*/           __NL__\
			.bInterfaceProtocol       = 0x01, /*V.250*/                            __NL__\
			.iInterface               = 0                                          __NL__\
		},                                                                         __NL__\
		.s##i##_header_desc = {                                                    __NL__\
			.bFunctionLength          = sizeof(USB_CDC_Header_Descriptor_TypeDef), __NL__\
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,              __NL__\
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_HEADER,               __NL__\
			.bcdCDC                   = 0x0110                                     __NL__\
		},                                                                         __NL__\
		.s##i##_cm_desc = {                                                        __NL__\
			.bFunctionLength          = sizeof(USB_CDC_CM_Descriptor_TypeDef),     __NL__\
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,              __NL__\
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_CM,                   __NL__\
			.bmCapabilities = {                                                    __NL__\
				.call_self_mng        = 0,                                         __NL__\
				.call_mng_itf         = 0,                                         __NL__\
				.reserved_as_0        = 0                                          __NL__\
			},                                                                     __NL__\
			.bDataInterface           = ((i-1)*2)+1                                __NL__\
		},                                                                         __NL__\
		.s##i##_acm_desc = {                                                       __NL__\
			.bFunctionLength          = sizeof(USB_CDC_ACM_Descriptor_TypeDef),    __NL__\
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,              __NL__\
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_ACM,                  __NL__\
			.bmCapabilities = {                                                    __NL__\
				.Comm_Feature         = 0,                                         __NL__\
				.Line_Coding          = 1,                                         __NL__\
				.Send_Break           = 0,                                         __NL__\
				.Network_Connection   = 0,                                         __NL__\
				.reserved_as_0        = 0                                          __NL__\
			}                                                                      __NL__\
		},                                                                         __NL__\
		.s##i##_union_desc = {                                                     __NL__\
			.bFunctionLength          = sizeof(USB_CDC_Union_Descriptor_TypeDef),  __NL__\
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,              __NL__\
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_UNION,                __NL__\
			.bControlInterface        = (i-1)*2,                                   __NL__\
			.bSubordinateInterface0   = ((i-1)*2)+1                                __NL__\
		},                                                                         __NL__\
		.s##i##_ep_ctrl = {                                                        __NL__\
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),   __NL__\
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,                    __NL__\
			.bEndpointAddress         = 0x80+SERIAL_PORT_COUNT+i,                  __NL__\
			.bmAttributes = {                                                      __NL__\
				.transfer_type        = USB_ENDPOINT_TYPE_INTERRUPT,               __NL__\
				.synchronisation_type = 0,                                         __NL__\
				.usage_type           = 0,                                         __NL__\
				.unused               = 0                                          __NL__\
			},                                                                     __NL__\
			.wMaxPacketSize           = 8,                                         __NL__\
			.bInterval                = 255                                        __NL__\
		},                                                                         __NL__\
		.s##i##_itf_data_desc = {                                                  __NL__\
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),  __NL__\
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,                   __NL__\
			.bInterfaceNumber         = ((i-1)*2)+1,                               __NL__\
			.bAlternateSetting        = 0,                                         __NL__\
			.bNumEndpoints            = 2,                                         __NL__\
			.bInterfaceClass          = 0x0A, /*CLASS CDC DATA*/                   __NL__\
			.bInterfaceSubClass       = 0,                                         __NL__\
			.bInterfaceProtocol       = 0,                                         __NL__\
			.iInterface               = 0                                          __NL__\
		},                                                                         __NL__\
		.s##i##_epout_desc = {                                                     __NL__\
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),   __NL__\
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,                    __NL__\
			.bEndpointAddress         = 0x00+i,                                    __NL__\
			.bmAttributes = {                                                      __NL__\
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,                    __NL__\
				.synchronisation_type = 0,                                         __NL__\
				.usage_type           = 0,                                         __NL__\
				.unused               = 0                                          __NL__\
			},                                                                     __NL__\
			.wMaxPacketSize           = 64,                                        __NL__\
			.bInterval                = 0                                          __NL__\
		},                                                                         __NL__\
		.s##i##_epin_desc = {                                                      __NL__\
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),   __NL__\
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,                    __NL__\
			.bEndpointAddress         = 0x80+i,                                    __NL__\
			.bmAttributes = {                                                      __NL__\
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,                    __NL__\
				.synchronisation_type = 0,                                         __NL__\
				.usage_type           = 0,                                         __NL__\
				.unused               = 0                                          __NL__\
			},                                                                     __NL__\
			.wMaxPacketSize           = 64,                                        __NL__\
			.bInterval                = 0                                          __NL__\
		}                                                                          __NL__
	
	#define CDC_FUNTION_CONTENT_WITH_IAD(i)                                                         \
		.s##i##_iad_desc = {                                                                  __NL__\
			.bLength                  = sizeof(USB_Interface_Association_Descriptor_TypeDef), __NL__\
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE_ASSOCIATION,                  __NL__\
			.bFirstInterface          = (i-1)*2,                                              __NL__\
			.bInterfaceCount          = 2,                                                    __NL__\
			.bFunctionClass           = 0x02, /*CLASS CDC*/                                   __NL__\
			.bFunctionSubClass        = 0x02, /*Abstract Control Model*/                      __NL__\
			.bFunctionProtocol        = 0x01, /*V.250*/                                       __NL__\
			.iFunction                = 0                                                     __NL__\
		},                                                                                    __NL__\
		CDC_FUNTION_CONTENT(i)                                                                __NL__
	
	static const __ALIGNED(4) struct __attribute__((packed)){
		USB_Configuration_Descriptor_TypeDef         cfg_desc;
		CDC_FUNCTION_DESC_STRUCT_WITH_IAD(1);
		CDC_FUNCTION_DESC_STRUCT_WITH_IAD(2);
		CDC_FUNCTION_DESC_STRUCT_WITH_IAD(3);
	} cfg_desc_c = {
		.cfg_desc = {
			.bLength                  = sizeof(USB_Configuration_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CONFIGURATION,
			.wTotalLength             = sizeof(cfg_desc_c),
			.bNumInterfaces           = SERIAL_PORT_COUNT*2,
			.bConfigurationValue      = 1,
			.iConfiguration           = 0,
			.bmAttributes = {
				.reserved_as_0        = 0,
				.remote_wakeup        = 0,
				.self_powered         = 0,
				.reserved_as_1        = 1
			},
			.bMaxPower                = 100
		},
		CDC_FUNTION_CONTENT_WITH_IAD(1),
		CDC_FUNTION_CONTENT_WITH_IAD(2),
		CDC_FUNTION_CONTENT_WITH_IAD(3)
	};
	static const SWHAL_USB_PCD_Desc_Typedef cfg_desc = {
		.desc   = &cfg_desc_c,
		.length = sizeof(cfg_desc_c)
	};
	/*
	static const __ALIGNED(4) struct __attribute__((packed)){
		uint8_t  bLength;
		uint8_t  bDescriptorType;
		uint16_t wLANGID0;
	} str0_desc_c = {
		.bLength         = sizeof(str0_desc_c),
		.bDescriptorType = USB_DESC_TYPE_STRING,
		.wLANGID0        = 0x0409
	};
	static const SWHAL_USB_PCD_Desc_Typedef str0_desc = {
		.desc   = &str0_desc_c,
		.length = sizeof(str0_desc_c)
	};*/
	(void)iDescIdx;
	switch (bDescType){
		case USB_DESC_TYPE_DEVICE:        return &dev_desc; break;
		case USB_DESC_TYPE_CONFIGURATION: return &cfg_desc; break;
		//case USB_DESC_TYPE_STRING:
		default:                          return NULL; break;
	};
}

void SWHAL_USB_CDC_Init(PCD_HandleTypeDef* hpcd, SWHAL_USB_PCD_HandleTypeDef* swpcd, SWHAL_USB_CDC_HandleTypeDef* swcdc){
	for(uint8_t i = 1; i <= SERIAL_PORT_COUNT; i++){
		swpcd->In_EP_Config[i].active = 1;
		swpcd->In_EP_Config[i].ep_mps = 64;
		swpcd->In_EP_Config[i].ep_type = EP_TYPE_BULK;
		
		swpcd->Out_EP_Config[i].active = 1;
		swpcd->Out_EP_Config[i].ep_mps = 64;
		swpcd->Out_EP_Config[i].ep_type = EP_TYPE_BULK;
	}

	swpcd->Transmit_Cplt_Callback = SWHAL_USB_CDC_Transmit_Cplt_Callback;
	swpcd->Receive_Cplt_Callback = SWHAL_USB_CDC_Receive_Cplt_Callback;
	swpcd->Nonstnadard_Setup = SWHAL_USB_CDC_NStd_Setup;
	swpcd->Get_Desc = SWHAL_USB_CDC_Get_Desc;
	
	swpcd->pData = swcdc;
	
	SWHAL_USB_PCD_Init(hpcd, swpcd);
}

void SWHAL_USB_CDC_Transmit(PCD_HandleTypeDef* hpcd, uint8_t idx, uint8_t* buf, uint32_t size){
	SWHAL_USB_PCD_Transmit(hpcd, idx, buf, size);
}

void SWHAL_USB_CDC_Receive(PCD_HandleTypeDef* hpcd, uint8_t idx, uint8_t* buf){
	SWHAL_USB_PCD_ReceiveA(hpcd, idx, buf);
}
