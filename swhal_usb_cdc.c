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
					if(swcdc->UART_Config_Callback) retval = (*swcdc->UART_Config_Callback)(hpcd, request->wIndex, &swcdc->uart_cfg);
					if(retval >= 0) SWHAL_USB_PCD_Transmit(hpcd, 0x00, NULL, 0);
					else            SWHAL_USB_PCD_Stall(hpcd, 0x80, 1);
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
					uint8_t idx = 0;
					switch (request->wIndex){
						case 0: idx = 1; break;
						case 2: idx = 2; break;
						case 4: idx = 3; break;
						default:         break;
					}
					if(swcdc->RTS_State_Callback && idx) retval = (*swcdc->RTS_State_Callback)(hpcd, idx, &swcdc->ctrl_cfg);
					if(retval >= 0) SWHAL_USB_PCD_Transmit(hpcd, 0x00, NULL, 0);
					else            SWHAL_USB_PCD_Stall(hpcd, 0x80, 1);
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
	static const __ALIGNED(4) struct __attribute__((packed)){
		USB_Configuration_Descriptor_TypeDef         cfg_desc;
		
		USB_Interface_Association_Descriptor_TypeDef s1_iad_desc;
		USB_Interface_Descriptor_TypeDef             s1_itf0_desc;
		USB_CDC_Header_Descriptor_TypeDef            s1_header_desc;
		USB_CDC_ACM_Descriptor_TypeDef               s1_acm_desc;
		USB_CDC_Union_Descriptor_TypeDef             s1_union_desc;
		USB_Interface_Descriptor_TypeDef             s1_itf1_desc;
		USB_Endpoint_Descriptor_TypeDef              s1_epout_desc;
		USB_Endpoint_Descriptor_TypeDef              s1_epin_desc;
		
		USB_Interface_Association_Descriptor_TypeDef s2_iad_desc;
		USB_Interface_Descriptor_TypeDef             s2_itf0_desc;
		USB_CDC_Header_Descriptor_TypeDef            s2_header_desc;
		USB_CDC_ACM_Descriptor_TypeDef               s2_acm_desc;
		USB_CDC_Union_Descriptor_TypeDef             s2_union_desc;
		USB_Interface_Descriptor_TypeDef             s2_itf1_desc;
		USB_Endpoint_Descriptor_TypeDef              s2_epout_desc;
		USB_Endpoint_Descriptor_TypeDef              s2_epin_desc;
		
		USB_Interface_Association_Descriptor_TypeDef s3_iad_desc;
		USB_Interface_Descriptor_TypeDef             s3_itf0_desc;
		USB_CDC_Header_Descriptor_TypeDef            s3_header_desc;
		USB_CDC_ACM_Descriptor_TypeDef               s3_acm_desc;
		USB_CDC_Union_Descriptor_TypeDef             s3_union_desc;
		USB_Interface_Descriptor_TypeDef             s3_itf1_desc;
		USB_Endpoint_Descriptor_TypeDef              s3_epout_desc;
		USB_Endpoint_Descriptor_TypeDef              s3_epin_desc;
	} cfg_desc_c = {
		.cfg_desc = {
			.bLength                  = sizeof(USB_Configuration_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CONFIGURATION,
			.wTotalLength             = sizeof(cfg_desc_c),
			.bNumInterfaces           = 6,
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
		
		.s1_iad_desc = {
			.bLength                  = sizeof(USB_Interface_Association_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE_ASSOCIATION,
			.bFirstInterface          = 0,
			.bInterfaceCount          = 2,
			.bFunctionClass           = 0x02, //CLASS CDC
			.bFunctionSubClass        = 0x02, //Abstract Control Model
			.bFunctionProtocol        = 0x00,
			.iFunction                = 0
		},
		.s1_itf0_desc = {
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,
			.bInterfaceNumber         = 0,
			.bAlternateSetting        = 0,
			.bNumEndpoints            = 0,
			.bInterfaceClass          = 0x02, //CLASS CDC
			.bInterfaceSubClass       = 0x02, //Abstract Control Model
			.bInterfaceProtocol       = 0x01, //V.250
			.iInterface               = 0
		},
		.s1_header_desc = {
			.bFunctionLength          = sizeof(USB_CDC_Header_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_HEADER,
			.bcdCDC                   = 0x0110
		},
		.s1_acm_desc = {
			.bFunctionLength          = sizeof(USB_CDC_ACM_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_ACM,
			.bmCapabilities = {
				.Comm_Feature         = 0,
				.Line_Coding          = 1,
				.Send_Break           = 0,
				.Network_Connection   = 0,
				.reserved_as_0        = 0
			}
		},
		.s1_union_desc = {
			.bFunctionLength          = sizeof(USB_CDC_Union_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_UNION,
			.bControlInterface        = 0,
			.bSubordinateInterface0   = 1
		},
		.s1_itf1_desc = {
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,
			.bInterfaceNumber         = 1,
			.bAlternateSetting        = 0,
			.bNumEndpoints            = 2,
			.bInterfaceClass          = 0x0A, //CLASS CDC DATA
			.bInterfaceSubClass       = 0,
			.bInterfaceProtocol       = 0,
			.iInterface               = 0
		},
		.s1_epout_desc = {
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,
			.bEndpointAddress         = 0x01,
			.bmAttributes = {
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,
				.synchronisation_type = 0,
				.usage_type           = 0,
				.unused               = 0
			},
			.wMaxPacketSize           = 64,
			.bInterval                = 0
		},
		.s1_epin_desc = {
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,
			.bEndpointAddress         = 0x81,
			.bmAttributes = {
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,
				.synchronisation_type = 0,
				.usage_type           = 0,
				.unused               = 0
			},
			.wMaxPacketSize           = 64,
			.bInterval                = 0
		},
		
		.s2_iad_desc = {
			.bLength                  = sizeof(USB_Interface_Association_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE_ASSOCIATION,
			.bFirstInterface          = 2,
			.bInterfaceCount          = 2,
			.bFunctionClass           = 0x02, //CLASS CDC
			.bFunctionSubClass        = 0x02, //Abstract Control Model
			.bFunctionProtocol        = 0x00,
			.iFunction                = 0
		},
		.s2_itf0_desc = {
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,
			.bInterfaceNumber         = 2,
			.bAlternateSetting        = 0,
			.bNumEndpoints            = 0,
			.bInterfaceClass          = 0x02, //CLASS CDC
			.bInterfaceSubClass       = 0x02, //Abstract Control Model
			.bInterfaceProtocol       = 0x01, //V.250
			.iInterface               = 0
		},
		.s2_header_desc = {
			.bFunctionLength          = sizeof(USB_CDC_Header_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_HEADER,
			.bcdCDC                   = 0x0110
		},
		.s2_acm_desc = {
			.bFunctionLength          = sizeof(USB_CDC_ACM_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_ACM,
			.bmCapabilities = {
				.Comm_Feature         = 0,
				.Line_Coding          = 1,
				.Send_Break           = 0,
				.Network_Connection   = 0,
				.reserved_as_0        = 0
			}
		},
		.s2_union_desc = {
			.bFunctionLength          = sizeof(USB_CDC_Union_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_UNION,
			.bControlInterface        = 0,
			.bSubordinateInterface0   = 3
		},
		.s2_itf1_desc = {
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,
			.bInterfaceNumber         = 3,
			.bAlternateSetting        = 0,
			.bNumEndpoints            = 2,
			.bInterfaceClass          = 0x0A, //CLASS CDC DATA
			.bInterfaceSubClass       = 0,
			.bInterfaceProtocol       = 0,
			.iInterface               = 0
		},
		.s2_epout_desc = {
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,
			.bEndpointAddress         = 0x02,
			.bmAttributes = {
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,
				.synchronisation_type = 0,
				.usage_type           = 0,
				.unused               = 0
			},
			.wMaxPacketSize           = 64,
			.bInterval                = 0
		},
		.s2_epin_desc = {
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,
			.bEndpointAddress         = 0x82,
			.bmAttributes = {
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,
				.synchronisation_type = 0,
				.usage_type           = 0,
				.unused               = 0
			},
			.wMaxPacketSize           = 64,
			.bInterval                = 0
		},
		
		.s3_iad_desc = {
			.bLength                  = sizeof(USB_Interface_Association_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE_ASSOCIATION,
			.bFirstInterface          = 4,
			.bInterfaceCount          = 2,
			.bFunctionClass           = 0x02, //CLASS CDC
			.bFunctionSubClass        = 0x02, //Abstract Control Model
			.bFunctionProtocol        = 0x00,
			.iFunction                = 0
		},
		.s3_itf0_desc = {
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,
			.bInterfaceNumber         = 4,
			.bAlternateSetting        = 0,
			.bNumEndpoints            = 0,
			.bInterfaceClass          = 0x02, //CLASS CDC
			.bInterfaceSubClass       = 0x02, //Abstract Control Model
			.bInterfaceProtocol       = 0x01, //V.250
			.iInterface               = 0
		},
		.s3_header_desc = {
			.bFunctionLength          = sizeof(USB_CDC_Header_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_HEADER,
			.bcdCDC                   = 0x0110
		},
		.s3_acm_desc = {
			.bFunctionLength          = sizeof(USB_CDC_ACM_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_ACM,
			.bmCapabilities = {
				.Comm_Feature         = 0,
				.Line_Coding          = 1,
				.Send_Break           = 0,
				.Network_Connection   = 0,
				.reserved_as_0        = 0
			}
		},
		.s3_union_desc = {
			.bFunctionLength          = sizeof(USB_CDC_Union_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_CLASS_SPECIFIC,
			.bDescriptorSubtype       = USB_DESC_SUBTYPE_CDC_UNION,
			.bControlInterface        = 0,
			.bSubordinateInterface0   = 5
		},
		.s3_itf1_desc = {
			.bLength                  = sizeof(USB_Interface_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_INTERFACE,
			.bInterfaceNumber         = 5,
			.bAlternateSetting        = 0,
			.bNumEndpoints            = 2,
			.bInterfaceClass          = 0x0A, //CLASS CDC DATA
			.bInterfaceSubClass       = 0,
			.bInterfaceProtocol       = 0,
			.iInterface               = 0
		},
		.s3_epout_desc = {
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,
			.bEndpointAddress         = 0x03,
			.bmAttributes = {
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,
				.synchronisation_type = 0,
				.usage_type           = 0,
				.unused               = 0
			},
			.wMaxPacketSize           = 64,
			.bInterval                = 0
		},
		.s3_epin_desc = {
			.bLength                  = sizeof(USB_Endpoint_Descriptor_TypeDef),
			.bDescriptorType          = USB_DESC_TYPE_ENDPOINT,
			.bEndpointAddress         = 0x83,
			.bmAttributes = {
				.transfer_type        = USB_ENDPOINT_TYPE_BULK,
				.synchronisation_type = 0,
				.usage_type           = 0,
				.unused               = 0
			},
			.wMaxPacketSize           = 64,
			.bInterval                = 0
		}
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
	for(uint8_t i = 1; i <= 3; i++){
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
