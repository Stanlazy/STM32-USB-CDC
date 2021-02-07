#ifndef _SWHAL_USB_PCD_H
#define _SWHAL_USB_PCD_H

#define EP_AMOUNT 8

#include "main.h"

#define  USB_DESC_TYPE_DEVICE                           0x01U
#define  USB_DESC_TYPE_CONFIGURATION                    0x02U
#define  USB_DESC_TYPE_STRING                           0x03U
#define  USB_DESC_TYPE_INTERFACE                        0x04U
#define  USB_DESC_TYPE_ENDPOINT                         0x05U
#define  USB_DESC_TYPE_DEVICE_QUALIFIER                 0x06U
#define  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION        0x07U
#define  USB_DESC_TYPE_INTERFACE_ASSOCIATION            0x0BU
#define  USB_DESC_TYPE_BOS                              0x0FU
#define  USB_DESC_TYPE_HID_DESC                         0x21U
#define  USB_DESC_TYPE_HID_REPORT_DESC                  0x22U
#define  USB_DESC_TYPE_CLASS_SPECIFIC                   0x24U

typedef struct __attribute__((packed)) {
	uint8_t  bLength;            //18
	uint8_t  bDescriptorType;    //USB_DESC_TYPE_DEVICE
	uint16_t bcdUSB;             //0x0200
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
} USB_Device_Descriptor_TypeDef;

typedef struct __attribute__((packed)) {
	uint8_t  bLength;            //9
	uint8_t  bDescriptorType;    //USB_DESC_TYPE_CONFIGURATION
	uint16_t wTotalLength;
	uint8_t  bNumInterfaces;
	uint8_t  bConfigurationValue;
	uint8_t  iConfiguration;
	struct __attribute__((packed)) {
		uint8_t reserved_as_0        :5;
		uint8_t remote_wakeup        :1;
		uint8_t self_powered         :1;
		uint8_t reserved_as_1        :1;
	}        bmAttributes;
	uint8_t  bMaxPower;          //per 2mA
} USB_Configuration_Descriptor_TypeDef;

typedef struct __attribute__((packed)) {
	uint8_t  bLength;            //8
	uint8_t  bDescriptorType;    //USB_DESC_TYPE_INTERFACE_ASSOCIATION
	uint8_t  bFirstInterface;
	uint8_t  bInterfaceCount;
	uint8_t  bFunctionClass;
	uint8_t  bFunctionSubClass;
	uint8_t  bFunctionProtocol;
	uint8_t  iFunction;
} USB_Interface_Association_Descriptor_TypeDef;

typedef struct __attribute__((packed)) {
	uint8_t  bLength;            //9
	uint8_t  bDescriptorType;    //USB_DESC_TYPE_INTERFACE
	uint8_t  bInterfaceNumber;
	uint8_t  bAlternateSetting;
	uint8_t  bNumEndpoints;
	uint8_t  bInterfaceClass;
	uint8_t  bInterfaceSubClass;
	uint8_t  bInterfaceProtocol;
	uint8_t  iInterface;
} USB_Interface_Descriptor_TypeDef;

#define  USB_ENDPOINT_TYPE_CONTROL                      0x00U
#define  USB_ENDPOINT_TYPE_ISOCHRONOUS                  0x01U
#define  USB_ENDPOINT_TYPE_BULK                         0x02U
#define  USB_ENDPOINT_TYPE_INTERRUPT                    0x03U
#define  USB_ENDPOINT_SYNC_NOSYNC                       0x00U
#define  USB_ENDPOINT_SYNC_ASYNC                        0x01U
#define  USB_ENDPOINT_SYNC_ADAPTIVE                     0x02U
#define  USB_ENDPOINT_SYNC_SYNC                         0x03U
#define  USB_ENDPOINT_USAGE_DATA                        0x00U
#define  USB_ENDPOINT_USAGE_FEEDBACK                    0x01U
#define  USB_ENDPOINT_USAGE_EFEEDBACK                   0x02U

typedef struct __attribute__((packed)) {
	uint8_t  bLength;            //7
	uint8_t  bDescriptorType;    //USB_DESC_TYPE_ENDPOINT
	uint8_t  bEndpointAddress;
	struct __attribute__((packed)) {
		uint8_t transfer_type        :2;
		uint8_t synchronisation_type :2;
		uint8_t usage_type           :2;
		uint8_t unused               :2;
	}        bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t  bInterval;
} USB_Endpoint_Descriptor_TypeDef;

#define  USB_REQUEST_RECIPIENT_DEVICE                   0x00U
#define  USB_REQUEST_RECIPIENT_INTERFACE                0x01U
#define  USB_REQUEST_RECIPIENT_ENDPOINT                 0x02U
#define  USB_REQUEST_TYPE_STANDARD                      0x00U
#define  USB_REQUEST_TYPE_CLASS                         0x01U
#define  USB_REQUEST_TYPE_VENDOR                        0x02U
#define  USB_REQUEST_DIR_OUT                            0x00U
#define  USB_REQUEST_DIR_IN                             0x01U
#define  USB_STD_DEV_REQ_GET_STATUS                     0x00U
#define  USB_STD_DEV_REQ_CLEAR_FEATURE                  0x01U
#define  USB_STD_DEV_REQ_SET_FEATURE                    0x03U
#define  USB_STD_DEV_REQ_SET_ADDRESS                    0x05U
#define  USB_STD_DEV_REQ_GET_DESCRIPTOR                 0x06U
#define  USB_STD_DEV_REQ_SET_DESCRIPTOR                 0x07U
#define  USB_STD_DEV_REQ_GET_CONFIGURATION              0x08U
#define  USB_STD_DEV_REQ_SET_CONFIGURATION              0x09U
#define  USB_STD_ITF_REQ_GET_STATUS                     0x00U
#define  USB_STD_ITF_REQ_CLEAR_FEATURE                  0x01U
#define  USB_STD_ITF_REQ_SET_FEATURE                    0x03U
#define  USB_STD_ITF_REQ_GET_INTERFACE                  0x0AU
#define  USB_STD_ITF_REQ_SET_INTERFACE                  0x11U
#define  USB_STD_EP_REQ_GET_STATUS                      0x00U
#define  USB_STD_EP_REQ_CLEAR_FEATURE                   0x01U
#define  USB_STD_EP_REQ_SET_FEATURE                     0x03U
#define  USB_STD_EP_REQ_SYNCH_FRAME                     0x12U

typedef struct __attribute__((packed)) {
	struct __attribute__((packed)) {
		uint8_t recipient            :5;
		uint8_t type                 :2;
		uint8_t direction            :1;
	}        bmRequestType;
	uint8_t  bRequest;
	union {
		struct {
			uint8_t iDescIdx;
			uint8_t bDescType;
		};
		uint16_t wValue;
	};
	uint16_t wIndex;
	uint16_t wLength;
} USB_Request_Packet_TypeDef;


typedef struct {
	uint8_t* desc;
	uint16_t length;
} SWHAL_USB_PCD_Desc_Typedef;

typedef struct {
	uint8_t* buffer;
	uint32_t length;
} SWHAL_USB_PCD_IN_EP_State_Typedef;

typedef struct {
	uint8_t* buffer;
	uint32_t remain_len;
	uint32_t xfered_len;
} SWHAL_USB_PCD_OUT_EP_State_Typedef;

typedef struct {
	uint16_t ep_mps;
	uint8_t  active;
	uint8_t  ep_type;
} SWHAL_USB_PCD_EP_Config_Typedef;

typedef struct {
	SWHAL_USB_PCD_EP_Config_Typedef In_EP_Config[EP_AMOUNT];
	SWHAL_USB_PCD_EP_Config_Typedef Out_EP_Config[EP_AMOUNT];
	
	SWHAL_USB_PCD_IN_EP_State_Typedef In_EP_State[EP_AMOUNT];
	SWHAL_USB_PCD_OUT_EP_State_Typedef Out_EP_State[EP_AMOUNT];
	
	uint32_t reset_count;
	
	void (*SOFCallback)       (PCD_HandleTypeDef* hpcd);
	void (*SetupStageCallback)(PCD_HandleTypeDef* hpcd);
	void (*ResetCallback)     (PCD_HandleTypeDef* hpcd);
	void (*SuspendCallback)   (PCD_HandleTypeDef* hpcd);
	void (*ResumeCallback)    (PCD_HandleTypeDef* hpcd);
	void (*ConnectCallback)   (PCD_HandleTypeDef* hpcd);
	void (*DisconnectCallback)(PCD_HandleTypeDef* hpcd);
	
	void (*DataOutStageCallback)    (PCD_HandleTypeDef* hpcd, uint8_t epnum);
	void (*DataInStageCallback)     (PCD_HandleTypeDef* hpcd, uint8_t epnum);
	void (*ISOOUTIncompleteCallback)(PCD_HandleTypeDef* hpcd, uint8_t epnum);
	void (*ISOINIncompleteCallback) (PCD_HandleTypeDef* hpcd, uint8_t epnum);
	void (*BCDCallback)             (PCD_HandleTypeDef* hpcd, PCD_BCD_MsgTypeDef msg);
	void (*LPMCallback)             (PCD_HandleTypeDef* hpcd, PCD_LPM_MsgTypeDef msg);
	
	void (*Transmit_Cplt_Callback) (PCD_HandleTypeDef* hpcd, uint8_t epnum);
	void (*Receive_Cplt_Callback)  (PCD_HandleTypeDef* hpcd, uint8_t epnum, uint16_t alen);
	void (*Real_Connected_Callback)(PCD_HandleTypeDef* hpcd);
	int8_t (*Nonstnadard_Setup)    (PCD_HandleTypeDef* hpcd, USB_Request_Packet_TypeDef* request);
	
	SWHAL_USB_PCD_Desc_Typedef* (*Get_Desc)(PCD_HandleTypeDef* hpcd, uint8_t bDescType, uint8_t iDescIdx);
	
	void* pData;
} SWHAL_USB_PCD_HandleTypeDef;

void SWHAL_USB_PCD_Init     (PCD_HandleTypeDef* hpcd, SWHAL_USB_PCD_HandleTypeDef* swpcd);
void SWHAL_USB_PCD_Reset    (PCD_HandleTypeDef* hpcd);
void SWHAL_USB_PCD_Transmit (PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t* buf, uint32_t len);
void SWHAL_USB_PCD_ReceiveA (PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t* buf);
void SWHAL_USB_PCD_ReceiveL (PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t* buf, uint32_t len);
void SWHAL_USB_PCD_Stall    (PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t is_stall);
void SWHAL_USB_PCD_Stall_EP0(PCD_HandleTypeDef* hpcd);

#endif /*_SWHAL_USB_PCD_H*/
