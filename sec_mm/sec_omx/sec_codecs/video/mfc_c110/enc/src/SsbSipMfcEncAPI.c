/*
 * Copyright 2010 Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "SsbSipMfcApi.h"
#include "mfc_interface.h"

#define _MFCLIB_MAGIC_NUMBER     0x92241001

void *SsbSipMfcEncOpen(void)
{
    int hMFCOpen;
    unsigned int mapped_addr;
    _MFCLIB *pCTX;

    hMFCOpen = open(SAMSUNG_MFC_DEV_NAME, O_RDWR | O_NDELAY);
    if (hMFCOpen < 0) {
        LOGE("SsbSipMfcEncOpen: MFC Open failure\n");
        return NULL;
    }

    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("SsbSipMfcEncOpen: malloc failed.\n");
        close(hMFCOpen);
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    //////////////////////////////////////////
	//	Mapping the MFC Input/Output Buffer	//
	//////////////////////////////////////////
	// mapping shared in/out buffer between application and MFC device driver
    mapped_addr = (unsigned int)mmap(0, MFC_DATA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, hMFCOpen, 0);
    if (!mapped_addr) {
        LOGE("SsbSipMfcDecOpen: FIMV5.0 driver address mapping failed\n");
        return NULL;
    }
    
    pCTX->magic         = _MFCLIB_MAGIC_NUMBER;
    pCTX->hOpen   		= hMFCOpen;
    pCTX->mapped_addr   = (unsigned char *)mapped_addr;

    return (void *)pCTX;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncInit(void *openHandle, void *param)
{
    int ret_code;

    _MFCLIB *pCTX;
    struct mfc_enc_init_arg EncArg;
    SSBSIP_MFC_ENC_H264_PARAM *h264_arg;
    SSBSIP_MFC_ENC_MPEG4_PARAM *mpeg4_arg;
    SSBSIP_MFC_ENC_H263_PARAM *h263_arg;
    SSBSIP_MFC_CODEC_TYPE codecType;

    pCTX = (_MFCLIB *)openHandle;
    memset(&EncArg, 0, sizeof(struct mfc_enc_init_arg));

    LOGV("SsbSipMfcEncInit: Encode Init start\n");

    mpeg4_arg = (SSBSIP_MFC_ENC_MPEG4_PARAM *)param;
    codecType = mpeg4_arg->codecType;

    if ((codecType != MPEG4_ENC) &&
        (codecType != H264_ENC)  &&
        (codecType != H263_ENC)) {
        LOGE("SsbSipMfcEncOpen: Undefined codec type.\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX->encoder = codecType;

    switch (pCTX->encoder) {
        case MPEG4_ENC:
            LOGV("SsbSipMfcEncInit: MPEG4 Encode\n");
            mpeg4_arg = (SSBSIP_MFC_ENC_MPEG4_PARAM *)param;

            pCTX->width = mpeg4_arg->SourceWidth;
            pCTX->height = mpeg4_arg->SourceHeight;
            pCTX->framerate = mpeg4_arg->TimeIncreamentRes;
            pCTX->bitrate   = mpeg4_arg->Bitrate;
            pCTX->gop_num   = mpeg4_arg->IDRPeriod;
            
            pCTX->enc_strm_size = 0;
            break;

        case H263_ENC:
            LOGV("SsbSipMfcEncInit: H263 Encode\n");
            h263_arg = (SSBSIP_MFC_ENC_H263_PARAM *)param;

            pCTX->width = h263_arg->SourceWidth;
            pCTX->height = h263_arg->SourceHeight;
            pCTX->framerate = h263_arg->FrameRate;
            pCTX->bitrate   = h263_arg->Bitrate;
            pCTX->gop_num   = h263_arg->IDRPeriod;
            
            pCTX->enc_strm_size = 0;
            break;

        case H264_ENC:
            LOGV("SsbSipMfcEncInit: H264 Encode\n");
            h264_arg = (SSBSIP_MFC_ENC_H264_PARAM *)param;

            pCTX->width = h264_arg->SourceWidth;
            pCTX->height = h264_arg->SourceHeight;
            pCTX->framerate = h264_arg->FrameRate;
            pCTX->bitrate   = h264_arg->Bitrate;
            pCTX->gop_num   = h264_arg->IDRPeriod;
            
            pCTX->enc_strm_size = 0;
            break;

        default:
            break;
    }

    switch (pCTX->encoder) {
        case MPEG4_ENC:
            mpeg4_arg = (SSBSIP_MFC_ENC_MPEG4_PARAM*)param;

            EncArg.in_width = mpeg4_arg->SourceWidth;
            EncArg.in_height = mpeg4_arg->SourceHeight;
            EncArg.in_gopNum = mpeg4_arg->IDRPeriod;

            EncArg.in_frameRateRes = mpeg4_arg->TimeIncreamentRes;
            EncArg.in_frameRateDiv = 0;
            
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_MPEG4_ENC_INIT, &EncArg);
            if (EncArg.ret_code != MFC_RET_OK) {
                LOGE("SsbSipMfcEncInit: IOCTL_MFC_MPEG4_ENC_INIT (%d) failed\n", EncArg.ret_code);
                return MFC_RET_ENC_INIT_FAIL;
            }
            
            break;

        case H263_ENC:
            h263_arg = (SSBSIP_MFC_ENC_H263_PARAM *)param;

            EncArg.in_width = h263_arg->SourceWidth;
            EncArg.in_height = h263_arg->SourceHeight;
            EncArg.in_gopNum = h263_arg->IDRPeriod;
            
            EncArg.in_frameRateRes = h263_arg->FrameRate;
            EncArg.in_frameRateDiv = 0;
            
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_H263_ENC_INIT, &EncArg);
            if (EncArg.ret_code != MFC_RET_OK) {
                LOGE("SsbSipMfcEncInit: IOCTL_MFC_H263_ENC_INIT (%d) failed\n", EncArg.ret_code);
                return MFC_RET_ENC_INIT_FAIL;
            }

            break;

        case H264_ENC:
            h264_arg = (SSBSIP_MFC_ENC_H264_PARAM *)param;

            EncArg.in_width = h264_arg->SourceWidth;
            EncArg.in_height = h264_arg->SourceHeight;
            EncArg.in_gopNum = h264_arg->IDRPeriod;
            
            EncArg.in_frameRateRes = h264_arg->FrameRate;
            EncArg.in_frameRateDiv = 0;
            
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_H264_ENC_INIT, &EncArg);
            if (EncArg.ret_code != MFC_RET_OK) {
                LOGE("SsbSipMfcEncInit: IOCTL_MFC_H264_ENC_INIT (%d) failed\n", EncArg.ret_code);
                return MFC_RET_ENC_INIT_FAIL;
            }

            break;

        default:
            break;
    }

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncExe(void *openHandle)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_enc_exe_arg EncArg;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncExe: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    memset(&EncArg, 0x00, sizeof(struct mfc_enc_exe_arg));
    
    if ((pCTX->encoder != MPEG4_ENC) &&
        (pCTX->encoder != H264_ENC)  &&
        (pCTX->encoder != H263_ENC)) {
        LOGE("SsbSipMfcEncOpen: Undefined codec type.\n");
        return MFC_RET_INVALID_PARAM;
    }
    
    switch (pCTX->encoder) {
        case MPEG4_ENC:
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_MPEG4_ENC_EXE, &EncArg);
            if (EncArg.ret_code != MFC_RET_OK) {
                LOGE("SsbSipMfcEncInit: IOCTL_MFC_MPEG4_ENC_EXE (%d) failed\n", EncArg.ret_code);
                return MFC_RET_ENC_INIT_FAIL;
            }
            break;
        
        case H263_ENC:
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_H263_ENC_EXE, &EncArg);
            if (EncArg.ret_code != MFC_RET_OK) {
                LOGE("SsbSipMfcEncInit: IOCTL_MFC_MPEG4_ENC_EXE (%d) failed\n", EncArg.ret_code);
                return MFC_RET_ENC_INIT_FAIL;
            }
            break;
        
        case H264_ENC:
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_H264_ENC_EXE, &EncArg);
            if (EncArg.ret_code != MFC_RET_OK) {
                LOGE("SsbSipMfcEncInit: IOCTL_MFC_MPEG4_ENC_EXE (%d) failed\n", EncArg.ret_code);
                return MFC_RET_ENC_INIT_FAIL;
            }
            break;
            
        default:
            break;
    }
    
    // Encoded stream size is saved. (This value is returned in SsbSipH264EncodeGetOutBuf)
	pCTX->enc_strm_size = EncArg.out_encoded_size;
    
	if(EncArg.out_header_size > 0) {
		pCTX->enc_hdr_size = EncArg.out_header_size;
		LOGI("SsbSipH264EncodeExe: HEADER SIZE = %d\n", pCTX->enc_hdr_size);
	}
    
    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncClose(void *openHandle)
{
    _MFCLIB  *pCTX;
    
    
	////////////////////////////////
	//  Input Parameter Checking  //
	////////////////////////////////
	if (openHandle == NULL) {
		LOGE("SsbSipMPEG4EncodeDeInit: openHandle is NULL\n");
		return MFC_RET_CLOSE_FAIL;
	}
    
	pCTX  = (_MFCLIB *) openHandle;
    
	munmap(pCTX->mapped_addr, MFC_DATA_BUF_SIZE);
	close(pCTX->hOpen);

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetInBuf(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_get_buf_addr_arg user_addr_arg;
    int y_size, c_size;
    int aligned_y_size, aligned_c_size;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncGetInBuf: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    y_size = pCTX->width * pCTX->height;
    c_size = (pCTX->width * pCTX->height) >> 1;

    aligned_y_size = ALIGN_TO_8KB(ALIGN_TO_128B(pCTX->width) * ALIGN_TO_32B(pCTX->height));
    aligned_c_size = ALIGN_TO_8KB(ALIGN_TO_128B(pCTX->width) * ALIGN_TO_32B(pCTX->height/2));

    /* Allocate luma & chroma buf */
    user_addr_arg.in_usr_data = (int)pCTX->mapped_addr;
    ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_GET_FRAM_BUF_ADDR, &user_addr_arg);
    if (ret_code < 0) {
        LOGE("SsbSipMfcEncGetInBuf: IOCTL_MFC_GET_FRAM_BUF_ADDR failed\n");
        return MFC_RET_ENC_GET_INBUF_FAIL;
    }

    /* Get physical address information */
    unsigned int phy_addr[2];
    ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR, &phy_addr);
    if (ret_code < 0) {
        LOGE("SsbSipMfcEncGetInBuf: IOCTL_MFC_GET_REAL_ADDR failed\n");
        return MFC_RET_ENC_GET_INBUF_FAIL;
    }
    
    pCTX->virFrmBuf.luma = user_addr_arg.out_buf_addr;
    pCTX->virFrmBuf.chroma = user_addr_arg.out_buf_addr + (unsigned int)aligned_y_size;
    pCTX->phyFrmBuf.luma = phy_addr[0];
    pCTX->phyFrmBuf.chroma = phy_addr[0] + (unsigned int)aligned_y_size;

    input_info->YPhyAddr = (void*)pCTX->phyFrmBuf.luma;
    input_info->CPhyAddr = (void*)pCTX->phyFrmBuf.chroma;
    input_info->YVirAddr = (void*)pCTX->virFrmBuf.luma;
    input_info->CVirAddr = (void*)pCTX->virFrmBuf.chroma;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetInBuf(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncSetInBuf: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    LOGV("SsbSipMfcEncSetInBuf: input_info->YPhyAddr & input_info->CPhyAddr should be 64KB aligned\n");

    pCTX = (_MFCLIB *)openHandle;

    pCTX->phyFrmBuf.luma = (unsigned int)input_info->YPhyAddr;
    pCTX->phyFrmBuf.chroma = (unsigned int)input_info->CPhyAddr;
    pCTX->virFrmBuf.luma = (unsigned int)input_info->YVirAddr;
    pCTX->virFrmBuf.chroma = (unsigned int)input_info->CVirAddr;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetOutBuf(void *openHandle, SSBSIP_MFC_ENC_OUTPUT_INFO *output_info)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncGetOutBuf: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    output_info->headerSize = pCTX->enc_hdr_size;
    output_info->dataSize = pCTX->enc_strm_size;
    output_info->StrmPhyAddr = (void *)pCTX->phyStrmBuf.luma;
    output_info->StrmVirAddr = (void *)pCTX->virStrmBuf.luma;

    output_info->encodedYPhyAddr = (void *)pCTX->phyFrmBuf.luma;
    output_info->encodedCPhyAddr = (void *)pCTX->phyFrmBuf.chroma;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetOutBuf(void *openHandle, void *phyOutbuf, void *virOutbuf, int outputBufferSize)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncSetOutBuf: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    pCTX->phyStrmBuf.luma = (int)phyOutbuf;
    pCTX->virStrmBuf.luma = (int)virOutbuf;
    pCTX->sizeStrmBuf = outputBufferSize;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncSetConfig(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_set_config_arg EncArg;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncSetConfig: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("SsbSipMfcEncSetConfig: value is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&EncArg, 0x00, sizeof(struct mfc_set_config_arg));

    switch (conf_type) {
    case MFC_ENC_SETCONF_FRAME_TYPE:
    case MFC_ENC_SETCONF_CHANGE_FRAME_RATE:
    case MFC_ENC_SETCONF_CHANGE_BIT_RATE:
    case MFC_ENC_SETCONF_ALLOW_FRAME_SKIP:
        EncArg.in_config_param = conf_type;
        EncArg.in_config_value[0] = *((unsigned int *) value);
        EncArg.in_config_value[1] = 0;
        break;

    case MFC_ENC_SETCONF_FRAME_TAG:
        pCTX->frametag = *((int *)value);
        return MFC_RET_OK;

    default:
        LOGE("SsbSipMfcEncSetConfig: No such conf_type is supported.\n");
        return MFC_RET_INVALID_PARAM;
    }

    ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_SET_CONFIG, &EncArg);
    if (EncArg.ret_code != MFC_RET_OK) {
        LOGE("SsbSipMfcEncSetConfig: IOCTL_MFC_SET_CONFIG failed(ret : %d)\n", EncArg.ret_code);
        return MFC_RET_ENC_SET_CONF_FAIL;
    }

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcEncGetConfig(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value)
{
    _MFCLIB *pCTX;
    struct mfc_get_config_arg EncArg;

    pCTX = (_MFCLIB *)openHandle;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcEncGetConfig: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }
    if (value == NULL) {
        LOGE("SsbSipMfcEncGetConfig: value is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&EncArg, 0x00, sizeof(struct mfc_get_config_arg));

    switch (conf_type) {
    case MFC_ENC_GETCONF_FRAME_TAG:
        *((unsigned int *)value) = pCTX->frametag;
        break;

    default:
        LOGE("SsbSipMfcEncGetConfig: No such conf_type is supported.\n");
        return MFC_RET_INVALID_PARAM;
    }

    return MFC_RET_OK;
}
