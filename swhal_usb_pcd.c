#include "swhal_usb_pcd.h"
#include <string.h>

static inline void SWHAL_USB_PCD_Open_EP(PCD_HandleTypeDef* hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	
	#define FIFO_MULTI_FACTOR 4
	
	for(uint8_t i = 0; i < EP_AMOUNT; i++){
		HAL_PCD_EP_Flush(hpcd, i);
		HAL_PCD_EP_Close(hpcd, i);
		HAL_PCD_EP_Flush(hpcd, i|0x80);
		HAL_PCD_EP_Close(hpcd, i|0x80);
	}
	
	#if defined (USB)
		uint16_t last_pma_addr = 0x20;
		for(uint8_t i = 0; i < EP_AMOUNT; i++){
			SWHAL_USB_PCD_EP_Config_Typedef* In_cfg = &(swpcd->Out_EP_Config[i]);
			SWHAL_USB_PCD_EP_Config_Typedef* Out_cfg = &(swpcd->In_EP_Config[i]);
			if(In_cfg->active){
				HAL_PCDEx_PMAConfig(hpcd, i|0x80, PCD_SNG_BUF, last_pma_addr);
				last_pma_addr += In_cfg->ep_mps;
			}
			if(Out_cfg->active){
				HAL_PCDEx_PMAConfig(hpcd, i, PCD_SNG_BUF, last_pma_addr);
				last_pma_addr += Out_cfg->ep_mps;
			}
		}
	#endif
	
	#if defined (USB_OTG_FS) || defined (USB_OTG_HS)
		uint16_t total_rx = 0;
		for(uint8_t i = 0; i < EP_AMOUNT; i++){
			SWHAL_USB_PCD_EP_Config_Typedef* Out_cfg = &(swpcd->Out_EP_Config[i]);
			if(Out_cfg->active){
				total_rx += Out_cfg->ep_mps;
			}
		}
		HAL_PCDEx_SetRxFiFo(hpcd, total_rx/4*FIFO_MULTI_FACTOR);
		for(uint8_t i = 0; i < EP_AMOUNT; i++){
			SWHAL_USB_PCD_EP_Config_Typedef* In_cfg = &(swpcd->In_EP_Config[i]);
			if(In_cfg->active){
				HAL_PCDEx_SetTxFiFo(hpcd, i, In_cfg->ep_mps/4*FIFO_MULTI_FACTOR);
			}
		}
	#endif
	
	for(uint8_t i = 0; i < EP_AMOUNT; i++){
		SWHAL_USB_PCD_EP_Config_Typedef* In_cfg = &(swpcd->In_EP_Config[i]);
		SWHAL_USB_PCD_EP_Config_Typedef* Out_cfg = &(swpcd->Out_EP_Config[i]);
		if(In_cfg->active){
			HAL_PCD_EP_Open(hpcd, i|0x80, In_cfg->ep_mps, In_cfg->ep_type);
		}
		if(Out_cfg->active){
			HAL_PCD_EP_Open(hpcd, i, Out_cfg->ep_mps, Out_cfg->ep_type);
		}
	}
}

void SWHAL_USB_PCD_Init(PCD_HandleTypeDef* hpcd, SWHAL_USB_PCD_HandleTypeDef* swpcd){
	swpcd->In_EP_Config[0].active = 1;
	swpcd->In_EP_Config[0].ep_mps = 64;
	swpcd->In_EP_Config[0].ep_type = EP_TYPE_CTRL;
	
	swpcd->Out_EP_Config[0].active = 1;
	swpcd->Out_EP_Config[0].ep_mps = 64;
	swpcd->Out_EP_Config[0].ep_type = EP_TYPE_CTRL;
	
	hpcd->pData = swpcd;
	
	#ifdef REAL_CONN_RESET
	reset_count = 0;
	#endif
	
	//SWHAL_USB_PCD_Open_EP(hpcd);
	
	HAL_PCD_Start(hpcd);
}

void SWHAL_USB_PCD_Transmit(PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t* buf, uint32_t len){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	epnum &= 0x7f;
	SWHAL_USB_PCD_IN_EP_State_Typedef* ep_state = &(swpcd->In_EP_State[epnum]);
	uint16_t mps = swpcd->In_EP_Config[epnum].ep_mps;
	if(ep_state->buffer) HAL_PCD_EP_Flush(hpcd, epnum|0x80);
	if(len > mps){
		ep_state->buffer = buf + mps;
		ep_state->length = len - mps;
		HAL_PCD_EP_Transmit(hpcd, epnum, buf, mps);
	} else {
		ep_state->buffer = buf;
		ep_state->length = 0;
		HAL_PCD_EP_Transmit(hpcd, epnum, buf, len);
	}
}

void SWHAL_USB_PCD_ReceiveA(PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t* buf){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	epnum &= 0x7f;
	SWHAL_USB_PCD_OUT_EP_State_Typedef* ep_state = &(swpcd->Out_EP_State[epnum]);
	uint16_t mps = swpcd->Out_EP_Config[epnum].ep_mps;
	if(ep_state->buffer) HAL_PCD_EP_Flush(hpcd, epnum);
	ep_state->buffer = buf;
	ep_state->remain_len = 0;
	ep_state->xfered_len = 0;
	HAL_PCD_EP_Receive(hpcd, epnum, buf, mps);
}

void SWHAL_USB_PCD_ReceiveL(PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t* buf, uint32_t len){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	epnum &= 0x7f;
	SWHAL_USB_PCD_OUT_EP_State_Typedef* ep_state = &(swpcd->Out_EP_State[epnum]);
	uint16_t mps = swpcd->Out_EP_Config[epnum].ep_mps;
	if(ep_state->buffer) HAL_PCD_EP_Flush(hpcd, epnum);
	ep_state->xfered_len = 0;
	if(len > mps){
		ep_state->buffer = buf + mps;
		ep_state->remain_len = len - mps;
		HAL_PCD_EP_Receive(hpcd, epnum, buf, mps);
	} else {
		ep_state->buffer = buf;
		ep_state->remain_len = 0;
		HAL_PCD_EP_Receive(hpcd, epnum, buf, len);
	}
}

void SWHAL_USB_PCD_Stall(PCD_HandleTypeDef* hpcd, uint8_t epnum, uint8_t is_stall){
	if(is_stall){
		HAL_PCD_EP_SetStall(hpcd, epnum);
	} else {
		HAL_PCD_EP_ClrStall(hpcd, epnum);
	}
}

void SWHAL_USB_PCD_Stall_EP0(PCD_HandleTypeDef* hpcd){
	USB_Request_Packet_TypeDef* request = hpcd->Setup;
	if(request->bmRequestType.direction == USB_REQUEST_DIR_IN){
		if(request->wLength){
			HAL_PCD_EP_SetStall(hpcd, 0x80);
		} else {
			HAL_PCD_EP_SetStall(hpcd, 0x00);
		}
	}
	if(request->bmRequestType.direction == USB_REQUEST_DIR_OUT){
		if(request->wLength){
			HAL_PCD_EP_SetStall(hpcd, 0x00);
		} else {
			HAL_PCD_EP_SetStall(hpcd, 0x80);
		}
	}
}

static inline void print_int(uint8_t* s, int n, uint8_t l){
	if(n < 0){
		n = 0 - n;
		s[0] = '-';
	} else {
		s[0] = '+';
	}
	s[1] = 0x00;
	s += 2;
	s += (l - 1) * 2;
	for(uint8_t i = 0; i < l; i++){
		s[0] = (n % 10) + '0';
		s[1] = 0x00;
		s -= 2;
		n /= 10;
	}
}

SWHAL_USB_PCD_Desc_Typedef SWHAL_USB_PCD_Get_Serial(void){
	#define STM32UID_ASCII
	
	#ifdef STM32UID_ASCII
		#define STR_LEN 7+1+3+1+4+1+4
	#else
		#define STR_LEN 24
	#endif
	__ALIGN_BEGIN static uint8_t serial_str[2+2*(STR_LEN)] __ALIGN_END;
	serial_str[0] = sizeof(serial_str);
	serial_str[1] = USB_DESC_TYPE_STRING;
	uint8_t* uid = (uint8_t*)UID_BASE;
	uint8_t* ptr = &(serial_str[2]);
	
	#ifdef STM32UID_ASCII
		for(int i = 0; i < 7; i++){
			ptr[0] = uid[11-i];
			ptr[1] = 0x00;
			ptr += 2;
		}
		print_int(ptr, uid[4], 3);
		ptr += 8;
		print_int(ptr, ((uint16_t*)uid)[0], 4);
		ptr += 10;
		print_int(ptr, ((uint16_t*)uid)[1], 4);
	#else
		for(int i = STR_LEN-1; i >= 0; i--){
			uint8_t curr_uint8 = uid[i];
			uint8_t temp;
			temp = curr_uint8 >> 4;
			ptr[0] = temp > 9 ? temp + 'A' : temp + '0';
			ptr[1] = 0x00;
			temp = curr_uint8 & 0x0F;
			ptr[2] = temp > 9 ? temp + 'A' : temp + '0';
			ptr[3] = 0x00;
			ptr += 4;
		}
	#endif
	
	return (SWHAL_USB_PCD_Desc_Typedef){serial_str, sizeof(serial_str)};
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//nothing
	if(swpcd->SOFCallback)(*swpcd->SOFCallback)(hpcd);
}

static inline void SWHAL_USB_PCD_NStd_Setup(PCD_HandleTypeDef *hpcd, SWHAL_USB_PCD_HandleTypeDef* swpcd, USB_Request_Packet_TypeDef* request){
	if(swpcd->Nonstnadard_Setup){
		if((*swpcd->Nonstnadard_Setup)(hpcd, request) < 0){
			SWHAL_USB_PCD_Stall_EP0(hpcd);
		}
	} else {
		SWHAL_USB_PCD_Stall_EP0(hpcd);
	}
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	USB_Request_Packet_TypeDef* request = hpcd->Setup;
	if(request->bmRequestType.type == USB_REQUEST_TYPE_STANDARD) {
		switch(request->bmRequestType.recipient){
			case USB_REQUEST_RECIPIENT_DEVICE:{
				switch(request->bRequest){
					case USB_STD_DEV_REQ_GET_STATUS:{
						static __ALIGNED(4) struct __attribute__((packed)) {
							uint8_t  self_powered  :1;
							uint8_t  remote_wakeup :1;
							uint16_t unused        :14;
						} retval;
						USB_Configuration_Descriptor_TypeDef* cfg_desc = (*swpcd->Get_Desc)(hpcd, USB_DESC_TYPE_CONFIGURATION, 0).desc;
						retval.self_powered = cfg_desc->bmAttributes.self_powered;
						retval.remote_wakeup = cfg_desc->bmAttributes.remote_wakeup;
						SWHAL_USB_PCD_Transmit(hpcd, 0, &retval, sizeof(retval));
					}break;
					case USB_STD_DEV_REQ_SET_ADDRESS:{
						HAL_PCD_SetAddress(hpcd, request->wValue);
						SWHAL_USB_PCD_Transmit(hpcd, 0, NULL, 0);
						#ifdef REAL_CONN_SETADDR
							if(swpcd->Real_Connected_Callback)(*swpcd->Real_Connected_Callback)(hpcd);
						#endif
					}break;
					case USB_STD_DEV_REQ_GET_DESCRIPTOR:{
						SWHAL_USB_PCD_Desc_Typedef desc = (*swpcd->Get_Desc)(hpcd, request->bDescType, request->iDescIdx);
						if(desc.desc){
							SWHAL_USB_PCD_Transmit(hpcd, 0, desc.desc, (request->wLength>desc.length)?desc.length:request->wLength);
						} else {
							SWHAL_USB_PCD_Stall_EP0(hpcd);
						}
					}break;
					case USB_STD_DEV_REQ_GET_CONFIGURATION:{
						static const __ALIGNED(4) uint8_t current_cfg = 0x01;
						SWHAL_USB_PCD_Transmit(hpcd, 0, &current_cfg, sizeof(current_cfg));
					}break;
					case USB_STD_DEV_REQ_SET_CONFIGURATION:{
						SWHAL_USB_PCD_Transmit(hpcd, 0, NULL, 0);
					}break;
					//case USB_STD_DEV_REQ_SET_DESCRIPTOR:
					//case USB_STD_DEV_REQ_CLEAR_FEATURE:
					//case USB_STD_DEV_REQ_SET_FEATURE:
					default:{
						SWHAL_USB_PCD_NStd_Setup(hpcd, swpcd, request);
					}break;
				}
			}break;
			case USB_REQUEST_RECIPIENT_INTERFACE:{
				switch(request->bRequest){
					case USB_STD_ITF_REQ_GET_INTERFACE:{
						static const __ALIGNED(4) uint8_t current_itf = 0x00;
						SWHAL_USB_PCD_Transmit(hpcd, 0, &current_itf, sizeof(current_itf));
					}break;
					case USB_STD_ITF_REQ_SET_INTERFACE:{
						SWHAL_USB_PCD_Transmit(hpcd, 0, NULL, 0);
					}break;
					//case USB_STD_ITF_REQ_GET_STATUS:
					//case USB_STD_ITF_REQ_CLEAR_FEATURE:
					//case USB_STD_ITF_REQ_SET_FEATURE:
					default:{
						SWHAL_USB_PCD_NStd_Setup(hpcd, swpcd, request);
					}break;
				}
			}break;
			case USB_REQUEST_RECIPIENT_ENDPOINT:{
				switch(request->bRequest){
					//case USB_STD_EP_REQ_GET_STATUS:
					//case USB_STD_EP_REQ_CLEAR_FEATURE:
					//case USB_STD_EP_REQ_SET_FEATURE:
					//case USB_STD_EP_REQ_SYNCH_FRAME:
					default:{
						SWHAL_USB_PCD_NStd_Setup(hpcd, swpcd, request);
					}break;
				}
			}break;
			default:{
				SWHAL_USB_PCD_NStd_Setup(hpcd, swpcd, request);
			}break;
		}
	} else {
		SWHAL_USB_PCD_NStd_Setup(hpcd, swpcd, request);
	}
	if(swpcd->SetupStageCallback)(*swpcd->SetupStageCallback)(hpcd);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
		
	SWHAL_USB_PCD_Open_EP(hpcd);
	
	memset(swpcd->In_EP_State, 0, sizeof(swpcd->In_EP_State));
	memset(swpcd->Out_EP_State, 0, sizeof(swpcd->Out_EP_State));
	
	#ifdef REAL_CONN_RESET
	swpcd->reset_count++;
	if(swpcd->reset_count >= 2){
		if(swpcd->Real_Connected_Callback)(*swpcd->Real_Connected_Callback)(hpcd);
	}
	#endif
	
	if(swpcd->ResetCallback)(*swpcd->ResetCallback)(hpcd);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->SuspendCallback)(*swpcd->SuspendCallback)(hpcd);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->ResumeCallback)(*swpcd->ResumeCallback)(hpcd);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->ConnectCallback)(*swpcd->ConnectCallback)(hpcd);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->DisconnectCallback)(*swpcd->DisconnectCallback)(hpcd);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	epnum &= 0x7f;
	SWHAL_USB_PCD_OUT_EP_State_Typedef* ep_state = &(swpcd->Out_EP_State[epnum]);
	uint16_t xfercnt = HAL_PCD_EP_GetRxCount(hpcd, epnum);
	uint16_t mps = swpcd->Out_EP_Config[epnum].ep_mps;
	if(ep_state->buffer){
		ep_state->xfered_len += xfercnt;
		if(xfercnt < mps || ep_state->remain_len == 0){
			ep_state->buffer = NULL;
			if(swpcd->Receive_Cplt_Callback)(*swpcd->Receive_Cplt_Callback)(hpcd, epnum, ep_state->xfered_len);
		} else {
			if(ep_state->remain_len > mps){
				HAL_PCD_EP_Receive(hpcd, epnum, ep_state->buffer, mps);
				ep_state->buffer += mps;
				ep_state->remain_len -= mps;
			} else {
				HAL_PCD_EP_Receive(hpcd, epnum, ep_state->buffer, ep_state->remain_len);
				ep_state->remain_len = 0;
			}
		}
	}
	if(swpcd->DataOutStageCallback)(*swpcd->DataOutStageCallback)(hpcd, epnum);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	epnum &= 0x7f;
	SWHAL_USB_PCD_IN_EP_State_Typedef* ep_state = &(swpcd->In_EP_State[epnum]);
	if(ep_state->buffer){
		if(ep_state->length > 0){
			uint16_t mps = swpcd->In_EP_Config[epnum].ep_mps;
			if(ep_state->length > mps){
				HAL_PCD_EP_Transmit(hpcd, epnum, ep_state->buffer, mps);
				ep_state->buffer += mps;
				ep_state->length -= mps;
			} else {
				HAL_PCD_EP_Transmit(hpcd, epnum, ep_state->buffer, ep_state->length);
				ep_state->length = 0;
			}
		} else {
			ep_state->buffer = NULL;
			if(swpcd->Transmit_Cplt_Callback)(*swpcd->Transmit_Cplt_Callback)(hpcd, epnum);
			if(!epnum){
				HAL_PCD_EP_Receive(hpcd, epnum, NULL, 0);
			}
		}
	}
	if(swpcd->DataInStageCallback)(*swpcd->DataInStageCallback)(hpcd, epnum);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->ISOOUTIncompleteCallback)(*swpcd->ISOOUTIncompleteCallback)(hpcd, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->ISOINIncompleteCallback)(*swpcd->ISOINIncompleteCallback)(hpcd, epnum);
}

void HAL_PCDEx_BCD_Callback(PCD_HandleTypeDef *hpcd, PCD_BCD_MsgTypeDef msg){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->BCDCallback)(*swpcd->BCDCallback)(hpcd, msg);
}

void HAL_PCDEx_LPM_Callback(PCD_HandleTypeDef *hpcd, PCD_LPM_MsgTypeDef msg){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//NYI
	if(swpcd->LPMCallback)(*swpcd->LPMCallback)(hpcd, msg);
}
