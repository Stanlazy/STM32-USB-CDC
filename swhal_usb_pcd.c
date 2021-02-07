#include "swhal_usb_pcd.h"
#include <string.h>

void SWHAL_USB_PCD_Init(PCD_HandleTypeDef* hpcd, SWHAL_USB_PCD_HandleTypeDef* swpcd){
	//SWHAL_USB_PCD_Reset(hpcd);
	
	swpcd->In_EP_Config[0].active = 1;
	swpcd->In_EP_Config[0].ep_mps = 64;
	swpcd->In_EP_Config[0].ep_type = EP_TYPE_CTRL;
	
	swpcd->Out_EP_Config[0].active = 1;
	swpcd->Out_EP_Config[0].ep_mps = 64;
	swpcd->Out_EP_Config[0].ep_type = EP_TYPE_CTRL;
	
	swpcd->reset_count = 0;
	
	hpcd->pData = swpcd;
	HAL_PCD_Start(hpcd);
}

void SWHAL_USB_PCD_Reset(PCD_HandleTypeDef* hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	for(uint8_t i = 0; i < EP_AMOUNT; i++){
		HAL_PCD_EP_Close(hpcd, i);
		HAL_PCD_EP_Close(hpcd, i|0x80);
		HAL_PCD_EP_Flush(hpcd, i);
		HAL_PCD_EP_Flush(hpcd, i|0x80);
	}
	memset(swpcd->In_EP_State, 0, sizeof(swpcd->In_EP_State));
	memset(swpcd->Out_EP_State, 0, sizeof(swpcd->Out_EP_State));
	swpcd->reset_count = 0;
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

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	//nothing
	if(swpcd->SOFCallback)(*swpcd->SOFCallback)(hpcd);
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
						USB_Configuration_Descriptor_TypeDef* cfg_desc = (*swpcd->Get_Desc)(hpcd, USB_DESC_TYPE_CONFIGURATION, 0)->desc;
						retval.self_powered = cfg_desc->bmAttributes.self_powered;
						retval.remote_wakeup = cfg_desc->bmAttributes.remote_wakeup;
						SWHAL_USB_PCD_Transmit(hpcd, 0, &retval, sizeof(retval));
					}break;
					case USB_STD_DEV_REQ_SET_ADDRESS:{
						HAL_PCD_SetAddress(hpcd, request->wValue);
						SWHAL_USB_PCD_Transmit(hpcd, 0, NULL, 0);
					}break;
					case USB_STD_DEV_REQ_GET_DESCRIPTOR:{
						if(request->bDescType == USB_DESC_TYPE_DEVICE_QUALIFIER){
							__asm__ volatile ("nop");
						}
						SWHAL_USB_PCD_Desc_Typedef* desc = (*swpcd->Get_Desc)(hpcd, request->bDescType, request->iDescIdx);
						if(desc && desc->desc){
							SWHAL_USB_PCD_Transmit(hpcd, 0, desc->desc, (request->wLength>desc->length)?desc->length:request->wLength);
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
						SWHAL_USB_PCD_Stall_EP0(hpcd);
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
						SWHAL_USB_PCD_Stall_EP0(hpcd);
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
						SWHAL_USB_PCD_Stall_EP0(hpcd);
					}break;
				}
			}break;
			default:{
				SWHAL_USB_PCD_Stall_EP0(hpcd);
			}break;
		}
	} else {
		if(swpcd->Nonstnadard_Setup){
			int8_t retval = (*swpcd->Nonstnadard_Setup)(hpcd, request);
			if(retval < 0){
				SWHAL_USB_PCD_Stall_EP0(hpcd);
			}
		} else {
			SWHAL_USB_PCD_Stall_EP0(hpcd);
		}
	}
	if(swpcd->SetupStageCallback)(*swpcd->SetupStageCallback)(hpcd);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd){
	SWHAL_USB_PCD_HandleTypeDef* swpcd = hpcd->pData;
	#if defined (USB)
		uint16_t last_pma_addr = 0x20;
	#endif
	#if defined (USB_OTG_FS)
		uint16_t total_rx = 0;
	#endif
	for(uint8_t i = 0; i < EP_AMOUNT; i++){
		HAL_PCD_EP_Close(hpcd, i);
		HAL_PCD_EP_Close(hpcd, i|0x80);
		HAL_PCD_EP_Flush(hpcd, i);
		HAL_PCD_EP_Flush(hpcd, i|0x80);
		
		SWHAL_USB_PCD_EP_Config_Typedef* In_cfg = &(swpcd->Out_EP_Config[i]);
		SWHAL_USB_PCD_EP_Config_Typedef* Out_cfg = &(swpcd->In_EP_Config[i]);
		if(In_cfg->active){
			#if defined (USB)
				HAL_PCDEx_PMAConfig(hpcd, i|0x80, PCD_SNG_BUF, last_pma_addr);
				last_pma_addr += In_cfg->ep_mps;
			#endif
			#if defined (USB_OTG_FS)
				HAL_PCDEx_SetTxFiFo(hpcd, i, In_cfg->ep_mps*2/4);
			#endif
			HAL_PCD_EP_Open(hpcd, i|0x80, In_cfg->ep_mps, In_cfg->ep_type);
		}
		if(Out_cfg->active){
			#if defined (USB)
				HAL_PCDEx_PMAConfig(hpcd, i, PCD_SNG_BUF, last_pma_addr);
				last_pma_addr += Out_cfg->ep_mps;
			#endif
			#if defined (USB_OTG_FS)
				total_rx += Out_cfg->ep_mps;
			#endif
			HAL_PCD_EP_Open(hpcd, i, Out_cfg->ep_mps, Out_cfg->ep_type);
		}
	}
	#if defined (USB_OTG_FS)
		HAL_PCDEx_SetRxFiFo(hpcd, total_rx*2/4);
	#endif
	
	swpcd->reset_count++;
	if(swpcd->reset_count >= 2){
		if(swpcd->Real_Connected_Callback)(*swpcd->Real_Connected_Callback)(hpcd);
	}
	
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
