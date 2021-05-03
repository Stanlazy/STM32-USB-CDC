#ifndef _SWHAL_USB_PCD_H
#define _SWHAL_USB_PCD_H

#include "main.h"
#include "swhal_usb_def.h"

#define EP_AMOUNT 8
//#define REAL_CONN_RESET
#define REAL_CONN_SETADDR
#define STM32UID_ASCII
//#define ENABLE_STR_DESC_CVT
//#define MAX_STR_DESC_LEN 32

typedef struct {
	const void* desc;
	uint16_t length;
} SWHAL_USB_PCD_Desc_Typedef;

typedef struct {
	const void* buffer;
	uint32_t length;
} SWHAL_USB_PCD_IN_EP_State_Typedef;

typedef struct {
	void* buffer;
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
	
	#ifdef REAL_CONN_RESET
	uint32_t reset_count;
	#endif
	
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
	
	SWHAL_USB_PCD_Desc_Typedef (*Get_Desc)(PCD_HandleTypeDef* hpcd, uint8_t bDescType, uint8_t iDescIdx);
	
	void* pData;
} SWHAL_USB_PCD_HandleTypeDef;

void SWHAL_USB_PCD_Init     (PCD_HandleTypeDef* hpcd, SWHAL_USB_PCD_HandleTypeDef* swpcd);
void SWHAL_USB_PCD_DeInit   (PCD_HandleTypeDef* hpcd);
void SWHAL_USB_PCD_Transmit (PCD_HandleTypeDef* hpcd, uint8_t epnum, const void* buf, uint32_t len);
void SWHAL_USB_PCD_ReceiveA (PCD_HandleTypeDef* hpcd, uint8_t epnum, void* buf);
void SWHAL_USB_PCD_ReceiveL (PCD_HandleTypeDef* hpcd, uint8_t epnum, void* buf, uint32_t len);
void SWHAL_USB_PCD_Stall    (PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t is_stall);
void SWHAL_USB_PCD_Stall_EP0(PCD_HandleTypeDef* hpcd);
SWHAL_USB_PCD_Desc_Typedef SWHAL_USB_PCD_Serial_Str_Desc(void);
SWHAL_USB_PCD_Desc_Typedef SWHAL_USB_PCD_Lang_Desc(void);
#ifdef ENABLE_STR_DESC_CVT
SWHAL_USB_PCD_Desc_Typedef SWHAL_USB_PCD_String_Desc_Cvt(char* s);
#endif

#endif /*_SWHAL_USB_PCD_H*/
