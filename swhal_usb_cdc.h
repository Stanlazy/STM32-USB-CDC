#ifndef _SWHAL_USB_CDC_H
#define _SWHAL_USB_CDC_H

#define SERIAL_PORT_COUNT 3
#define __NL__

#include "main.h"
#include "swhal_usb_pcd.h"

#define  USB_CDC_SET_LINE_ENCODING                      0x20U
#define  USB_CDC_GET_LINE_ENCODING                      0x21U
typedef struct __attribute__((packed)) {
	uint32_t rate;
	uint8_t  stopbit;
	#define  LINE_ENCODING_STOPBIT_1   0
	#define  LINE_ENCODING_STOPBIT_1_5 1
	#define  LINE_ENCODING_STOPBIT_2   2
	uint8_t  parity;
	#define  LINE_ENCODING_PARITY_NONE  0
	#define  LINE_ENCODING_PARITY_ODD   1
	#define  LINE_ENCODING_PARITY_EVEN  2
	#define  LINE_ENCODING_PARITY_MARK  3
	#define  LINE_ENCODING_PARITY_SPACE 4
	uint8_t  databits;
} USB_CDC_Line_Encoding_Typedef;

#define  USB_CDC_SET_CTRL_LINE                          0x22U
typedef union{
	struct __attribute__((packed)) {
		uint8_t  DTR :1;
		uint8_t  RTS :1;
		uint16_t unused :14;
	};
	uint16_t u16;
} USB_CDC_Control_Line_State_Typedef;

typedef struct {
	union{
		USB_CDC_Line_Encoding_Typedef       uart_cfg;
		uint32_t                            uart_cfg_u32[2];
	};
	USB_CDC_Control_Line_State_Typedef  ctrl_cfg;
	
	void (*TX_Cplt_Callback)(PCD_HandleTypeDef* hpcd, uint8_t idx);
	void (*RX_Cplt_Callback)(PCD_HandleTypeDef* hpcd, uint8_t idx, uint16_t alen);
	int8_t (*RTS_State_Callback)(PCD_HandleTypeDef* hpcd, uint8_t idx, USB_CDC_Control_Line_State_Typedef* status);
	int8_t (*UART_Config_Callback)(PCD_HandleTypeDef* hpcd, uint8_t idx, USB_CDC_Line_Encoding_Typedef* config);
	
	void* pData;
} SWHAL_USB_CDC_HandleTypeDef;

void SWHAL_USB_CDC_Init(PCD_HandleTypeDef* hpcd, SWHAL_USB_PCD_HandleTypeDef* swpcd, SWHAL_USB_CDC_HandleTypeDef* swcdc);
void SWHAL_USB_CDC_Transmit(PCD_HandleTypeDef* hpcd, uint8_t idx, void* buf, uint32_t size);
void SWHAL_USB_CDC_Receive(PCD_HandleTypeDef* hpcd, uint8_t idx, void* buf);

#endif
