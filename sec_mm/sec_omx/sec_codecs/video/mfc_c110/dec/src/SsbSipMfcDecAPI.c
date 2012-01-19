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

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_MFC"
#ifdef SEC_LOG_OFF
#undef SEC_LOG_OFF
#endif
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

#define _MFCLIB_MAGIC_NUMBER    0x92241000

#define USR_DATA_START_CODE     (0x000001B2)
#define VOP_START_CODE          (0x000001B6)
#define MP4_START_CODE          (0x000001)

static void getAByte(char *buff, int *code)
{
    int byte;

    *code = (*code << 8);
    byte = (int)*buff;
    byte &= 0xFF;
    *code |= byte;
}

static mfc_packed_mode isPBPacked(_MFCLIB *pCtx, int length)
{
    char *strmBuffer = NULL;
    char *strmBufferEnd = NULL;
    int startCode = 0xFFFFFFFF;

    strmBuffer = (char *)pCtx->virStrmBuf.luma;
    strmBufferEnd = (char *)pCtx->virStrmBuf.luma + length;

    while (1) {
        while (startCode != USR_DATA_START_CODE) {
            if (startCode == VOP_START_CODE) {
                SEC_OSAL_Log(SEC_LOG_TRACE, "isPBPacked: VOP START Found !!.....return\n");
                SEC_OSAL_Log(SEC_LOG_TRACE, "isPBPacked: Non Packed PB\n");
                return MFC_UNPACKED_PB;
            }
            getAByte(strmBuffer, &startCode);
            strmBuffer++;
            if (strmBuffer >= strmBufferEnd)
                goto out;
        }
        SEC_OSAL_Log(SEC_LOG_TRACE, "isPBPacked: User Data Found !!\n");

        do {
            if (*strmBuffer == 'p') {
                SEC_OSAL_Log(SEC_LOG_TRACE, "isPBPacked: Packed PB\n");
                return MFC_PACKED_PB;
            }
            getAByte(strmBuffer, &startCode);
            strmBuffer++;
            if (strmBuffer >= strmBufferEnd)
                goto out;
        } while ((startCode >> 8) != MP4_START_CODE);
    }

out:
    SEC_OSAL_Log(SEC_LOG_TRACE, "isPBPacked: Non Packed PB\n");
    return MFC_UNPACKED_PB;
}

void * SsbSipMfcDecOpen(void)
{
    int hMFCOpen;
    unsigned char *mapped_addr;
    _MFCLIB *pCTX;

    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecOpen: malloc failed.\n");
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    hMFCOpen = open(SAMSUNG_MFC_DEV_NAME, O_RDWR | O_NDELAY);
    if (hMFCOpen < 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecOpen: MFC Open failure\n");
        return NULL;
    }

    //////////////////////////////////////////
	//	Mapping the MFC Input/Output Buffer	//
	//////////////////////////////////////////
	// mapping shared in/out buffer between application and MFC device driver
    mapped_addr = (unsigned char *)mmap(0, MFC_DATA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, hMFCOpen, 0);
    if (!mapped_addr) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecOpen: FIMV5.0 driver address mapping failed\n");
        return NULL;
    }

    pCTX->magic         = _MFCLIB_MAGIC_NUMBER;
    pCTX->hOpen   		= hMFCOpen;
    pCTX->mapped_addr   = mapped_addr;
    
    SEC_OSAL_Log(SEC_LOG_TRACE, "FIMV5.0 Loaded");

    return (void *)pCTX;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecInit(void *openHandle, SSBSIP_MFC_CODEC_TYPE codecType, int lengthBufFill)
{
    int             ret_code;
    struct mfc_dec_init_arg Dec_args;
    _MFCLIB         *pCTX;
    
    int  ioctl_cmd;

    
    ////////////////////////////////
	//  Input Parameter Checking  //
	////////////////////////////////
	if (openHandle == NULL) {
		SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeExe : openHandle is NULL\n");
		return MFC_RET_DEC_INIT_FAIL;
	}
	if ((lengthBufFill < 0) || (lengthBufFill > 0x100000)) {
		SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeExe : lengthBufFill is invalid. (lengthBufFill=%d)\n", lengthBufFill);
		return MFC_RET_DEC_INIT_FAIL;
	}

    pCTX = (_MFCLIB *)openHandle;
    memset(&Dec_args, 0x00, sizeof(Dec_args));

    pCTX->decoder = codecType;
    
    switch (pCTX->decoder) {
        case MPEG4_DEC:
        case H263_DEC:
            ioctl_cmd = IOCTL_MFC_MPEG4_DEC_INIT;
            break;
            
        case H264_DEC:
            ioctl_cmd = IOCTL_MFC_H264_DEC_INIT;
            break;
            
        case VC1_DEC:
            ioctl_cmd = IOCTL_MFC_VC1_DEC_INIT;
            break;
            
        default:
            SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecInit : Undefined codec type.\n");
            return MFC_RET_DEC_INIT_FAIL;
    }
    
    /////////////////////////////////////////////////
    /////           (DeviceIoControl)           /////
    /////      	IOCTL_MFC_H264_DEC_EXE          /////
    /////////////////////////////////////////////////
    Dec_args.in_strmSize = lengthBufFill;
    ret_code = ioctl(pCTX->hOpen, ioctl_cmd, &Dec_args);
    if ((ret_code < 0) || (Dec_args.ret_code < 0)) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "IOCTLERROR: 0x%08x, 0x%08x", ret_code, Dec_args.ret_code);
        return MFC_RET_DEC_INIT_FAIL;
    }
    
    // Output argument (width , height)
    pCTX->width  = Dec_args.out_width;
    pCTX->height = Dec_args.out_height;
    pCTX->buf_width = Dec_args.out_buf_width;
    pCTX->buf_height = Dec_args.out_buf_height;
    
    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecExe(void *openHandle, int lengthBufFill)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_dec_exe_arg       Dec_args;
    
    int  ioctl_cmd;

    if (openHandle == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecExe: openHandle is NULL\n");
        return MFC_RET_DEC_EXE_ERR;
    }

    if ((lengthBufFill < 0) || (lengthBufFill > MFC_LINE_BUF_SIZE)) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecExe: lengthBufFill is invalid. (lengthBufFill=%d)\n", lengthBufFill);
        return MFC_RET_DEC_EXE_ERR;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&Dec_args, 0x00, sizeof(Dec_args));

    switch (pCTX->decoder) {
        case MPEG4_DEC:
        case H263_DEC:
            ioctl_cmd = IOCTL_MFC_MPEG4_DEC_EXE;
            break;
            
        case H264_DEC:
            ioctl_cmd = IOCTL_MFC_H264_DEC_EXE;
            break;
            
        case VC1_DEC:
            ioctl_cmd = IOCTL_MFC_VC1_DEC_EXE;
            break;
            
        default:
            SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeExe : Undefined codec type.\n");
            return MFC_RET_DEC_EXE_ERR;
	}
    
    /////////////////////////////////////////////////
	/////           (DeviceIoControl)           /////
	/////       IOCTL_MFC_H264_DEC_EXE          /////
	/////////////////////////////////////////////////
	Dec_args.in_strmSize = lengthBufFill;
	ret_code = ioctl(pCTX->hOpen, ioctl_cmd, &Dec_args);
	if ((ret_code < 0) || (Dec_args.ret_code < 0)) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "Decode ERROR (0x%08x),(0x%08)\n", ret_code, Dec_args.ret_code);
		return MFC_RET_DEC_EXE_ERR;
	}
    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecClose(void *openHandle)
{
    _MFCLIB *pCTX;

    
    ////////////////////////////////
	//  Input Parameter Checking  //
	////////////////////////////////
    if (openHandle == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecClose: openHandle is NULL\n");
        return MFC_RET_CLOSE_FAIL;
    }

    pCTX = (_MFCLIB *)openHandle;

    munmap((void *)pCTX->mapped_addr, MFC_DATA_BUF_SIZE);
    close(pCTX->hOpen);
    free(pCTX);

    return MFC_RET_OK;
}

void *SsbSipMfcDecGetInBuf(void *openHandle, int codecType, void **phyInBuf, int inputBufferSize)
{
    int ret_code;
    _MFCLIB *pCTX;
    struct mfc_get_buf_addr_arg	args;

    void	*pStrmBuf;
	int		nStrmBufSize; 
    int     ioctl_cmd;
    
    SEC_OSAL_Log(SEC_LOG_TRACE,"SsbSipMfcDecGetInBuf");
    ////////////////////////////////
	//  Input Parameter Checking  //
	////////////////////////////////
    if (inputBufferSize < 0) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetInBuf: inputBufferSize = %d is invalid\n", inputBufferSize);
        return NULL;
    }

    if (openHandle == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetInBuf: openHandle is NULL\n");
        return NULL;
    }

    pCTX = (_MFCLIB *)openHandle;
    
    switch (codecType) {
        case MPEG4_DEC:
        case H263_DEC:
        case H264_DEC:
            ioctl_cmd = IOCTL_MFC_GET_LINE_BUF_ADDR;
            break;
            
        case VC1_DEC:
            ioctl_cmd = IOCTL_MFC_GET_RING_BUF_ADDR;
            break;
            
        default:
            SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeGetInBuf: Unknown codec\n");
            return NULL;
            break;
    }
    
    /////////////////////////////////////////////////
	/////           (DeviceIoControl)           /////
	/////      IOCTL_MFC_GET_STRM_BUF_ADDR      /////
	/////////////////////////////////////////////////
	args.in_usr_data = (int)pCTX->mapped_addr;
	ret_code = ioctl(pCTX->hOpen, ioctl_cmd, &args);
	if ((ret_code < 0) || (args.ret_code < 0)) {
		SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeGetInBuf : Failed in get Input Buffer address\n");
		return NULL;
	}
	
	// Output arguments
	pStrmBuf     = (void *) args.out_buf_addr;	
	nStrmBufSize = args.out_buf_size;
    
    *phyInBuf = (void *)ioctl(pCTX->hOpen, IOCTL_VIRT_TO_PHYS, pStrmBuf);
    
    return pStrmBuf;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetInBuf(void *openHandle, void *phyInBuf, void *virInBuf, int inputBufferSize)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecSetInBuf: openHandle is NULL\n");
        return MFC_RET_DEC_SET_INBUF_FAIL;
    }

    pCTX  = (_MFCLIB *)openHandle;

    pCTX->phyStrmBuf.luma = (int)phyInBuf;
    pCTX->virStrmBuf.luma = (int)virInBuf;
    pCTX->sizeStrmBuf = inputBufferSize;
    return MFC_RET_OK;
}

/*This is strange the function doesn't return address of output buffer */
SSBSIP_MFC_DEC_OUTBUF_STATUS SsbSipMfcDecGetOutBuf(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info)
{
    _MFCLIB     *pCTX;
    struct mfc_get_buf_addr_arg	Dec_args;
    int         ret_code = 0;
    
    SSBSIP_MFC_CROP_INFORMATION crop_info;
    
    ////////////////////////////////
	//  Input Parameter Checking  //
	////////////////////////////////
    if (openHandle == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetOutBuf: openHandle is NULL\n");
        return MFC_DEC_GET_OUTBUF_FAIL;
    }

    pCTX = (_MFCLIB *)openHandle;

    output_info->img_width = pCTX->width;
    output_info->img_height= pCTX->height;

    output_info->buf_width = pCTX->buf_width;
    output_info->buf_height= pCTX->buf_height;
    
    /* by RainAde : for crop information */
    SsbSipMfcDecGetConfig(pCTX, MFC_DEC_GETCONF_CROP_INFO, &crop_info);
    
    output_info->crop_top_offset    = crop_info.crop_top_offset;
    output_info->crop_bottom_offset = crop_info.crop_bottom_offset;
    output_info->crop_left_offset   = crop_info.crop_left_offset;
    output_info->crop_right_offset  = crop_info.crop_right_offset;
    
    int aligned_y_size = ALIGN_TO_8KB(ALIGN_TO_128B(pCTX->width) * ALIGN_TO_32B(pCTX->height));
    
    /////////////////////////////////////////////////
	/////           (DeviceIoControl)           /////
	/////      IOCTL_MFC_GET_FRAM_BUF_ADDR      /////
	/////////////////////////////////////////////////
	Dec_args.in_usr_data = (int)pCTX->mapped_addr;
	ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_GET_FRAM_BUF_ADDR, &Dec_args);
	if ((ret_code < 0) || (Dec_args.ret_code < 0)) {
		SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeGetOutBuf : Failed in get FRAM_BUF address\n");
		return MFC_DEC_GET_OUTBUF_FAIL;
	}
    
    /* Added by LeBidou */
    // Splitting the YUV420 buffer into Y and C buffers
    // For Virtual addresses
    // Size of Y Layer is (buffer width * buffer height)
    // So Y Buffer is Frame buffer address
    // and C buffer is Y Buffer + (width * height)
    output_info->YVirAddr = (void *)Dec_args.out_buf_addr;
    output_info->CVirAddr = (void *)(Dec_args.out_buf_addr + aligned_y_size);
    pCTX->virFrmBuf.luma = (int)output_info->YVirAddr;
    pCTX->virFrmBuf.chroma = (int)output_info->CVirAddr;
    
    // For Physical address
    unsigned int PhyBuff[2];
    
    ret_code = SsbSipMfcDecGetConfig(pCTX, MFC_DEC_GETCONF_PHYADDR_FRAM_BUF, &PhyBuff);
    if (ret_code < 0) {
		SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeGetOutBuf : Failed in get FRAM_BUF address\n");
		return MFC_DEC_GET_OUTBUF_FAIL;
	}
    
    output_info->YPhyAddr = (void *)PhyBuff[0];
    output_info->CPhyAddr = (void *)(PhyBuff[0] + aligned_y_size);
    
    /*if (pCTX->displayStatus == 0)
        return MFC_GETOUTBUF_DISPLAY_END;
    else if (pCTX->displayStatus == 1)*/
        return MFC_GETOUTBUF_DISPLAY_DECODING;
    /*else if (pCTX->displayStatus == 2) */
        //return MFC_GETOUTBUF_DISPLAY_ONLY;
    /*else*/
       // return MFC_GETOUTBUF_DECODING_ONLY;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    _MFCLIB *pCTX;
    struct mfc_set_config_arg	set_config;
	int		ret_code = 0;

    ////////////////////////////////
	//  Input Parameter Checking  //
	////////////////////////////////
    if (openHandle == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecSetConfig: openHandle is NULL\n");
        return MFC_RET_DEC_SET_CONF_FAIL;
    }

    if (value == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecSetConfig: value is NULL\n");
        return MFC_RET_DEC_SET_CONF_FAIL;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&set_config, 0x00, sizeof(set_config));

    switch(conf_type) {
		case MFC_DEC_SETCONF_POST_ROTATE:
            set_config.in_config_param     = MFC_SET_CONFIG_DEC_ROTATE;
            set_config.in_config_value[0]  = *((unsigned int *) value);
            set_config.in_config_value[1]  = 0;
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_SET_CONFIG, &set_config);
            if ( (ret_code < 0) || (set_config.ret_code < 0) ) {
                SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeSetConfig: Error in MFC_DEC_SETCONF_POST_ROTATE.\n");
                return MFC_RET_DEC_SET_CONF_FAIL;
            }
            break;
            
        case MFC_SET_CONFIG_MP4_DBK_ON:
            set_config.in_config_param     = MFC_SET_CONFIG_MP4_DBK_ON;
            set_config.in_config_value[0]  = *((unsigned int *) value);
            set_config.in_config_value[1]  = 0;
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_SET_CONFIG, &set_config);
            if ( (ret_code < 0) || (set_config.ret_code < 0) ) {
                SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeSetConfig: Error in MFC_DEC_SETCONF_POST_ROTATE.\n");
                return MFC_RET_DEC_SET_CONF_FAIL;
            }
            break;
            
        case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
        case MFC_DEC_SETCONF_DISPLAY_DELAY:
            break;
            
        case MFC_DEC_SETCONF_FRAME_TAG:
            pCTX->frametag = ((unsigned int *) value)[0];
            break;
            
        default:
            SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeSetConfig: No such conf_type is supported.\n");
            return MFC_RET_DEC_SET_CONF_FAIL;
	}

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecGetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    _MFCLIB  	*pCTX;
	int				ret_code;
	struct mfc_get_config_arg		Dec_args;
    struct mfc_get_buf_addr_arg     get_buf;

    SSBSIP_MFC_IMG_RESOLUTION *img_resolution;
    SSBSIP_MFC_CROP_INFORMATION *crop_information;
    
    ////////////////////////////////
	//  Input Parameter Checking  //
	////////////////////////////////
    if (openHandle == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetConfig: openHandle is NULL\n");
        return MFC_RET_DEC_GET_CONF_FAIL;
    }

    if (value == NULL) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetConfig: value is NULL\n");
        return MFC_RET_DEC_GET_CONF_FAIL;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&Dec_args, 0x00, sizeof(Dec_args));

    switch (conf_type) {
        case MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT:
            img_resolution = (SSBSIP_MFC_IMG_RESOLUTION *)value;
            img_resolution->width = pCTX->width;
            img_resolution->height = pCTX->height;
            img_resolution->buf_width = pCTX->buf_width;
            img_resolution->buf_height = pCTX->buf_height;
            break;
            
        case MFC_DEC_GETCONF_CROP_INFO:
            Dec_args.in_config_param = MFC_GET_CONFIG_DEC_H264_CROPINFO;
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_GET_CONFIG, &Dec_args);
            if ((ret_code < 0) || (Dec_args.ret_code < 0)) {
                return MFC_RET_DEC_GET_CONF_FAIL;
            }
            crop_information = (SSBSIP_MFC_CROP_INFORMATION *)value;
            crop_information->crop_left_offset = Dec_args.out_config_value[0] >> 16;
            crop_information->crop_right_offset = Dec_args.out_config_value[0] & 0xFFFF;
            crop_information->crop_top_offset = Dec_args.out_config_value[1] >> 16;
            crop_information->crop_bottom_offset = Dec_args.out_config_value[1] & 0xFFFF;
            break;

        case MFC_DEC_GETCONF_PHYADDR_FRAM_BUF:
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR, &get_buf);
            if ((ret_code < 0) || (Dec_args.ret_code < 0)) {
                SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeGetConfig: Failed in get FRAM_BUF physical address.\n");
                return MFC_RET_DEC_GET_CONF_FAIL;
            }
            ((unsigned int *) value)[0] = get_buf.out_buf_addr;
            ((unsigned int *) value)[1] = get_buf.out_buf_size;
            break;
            
        case MFC_DEC_GETCONF_FRAM_NEED_COUNT:
            Dec_args.in_config_param 	= MFC_GET_CONFIG_DEC_FRAME_NEED_COUNT;
            Dec_args.out_config_value[0]	= 0;
            Dec_args.out_config_value[1]	= 0;
            
            ret_code = ioctl(pCTX->hOpen, IOCTL_MFC_GET_CONFIG, &Dec_args);
            if ((ret_code <0) || (Dec_args.ret_code < 0)) {
                SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecodeGetConfig: Error in MFC_DEC_GETCONF_FRAM_NEED_COUNT.\n");
                return MFC_RET_DEC_GET_CONF_FAIL;
            }
            ((int *) value)[0] = Dec_args.out_config_value[0];
            
            break;
            
        case MFC_DEC_GETCONF_FRAME_TAG:
            ((unsigned int *) value)[0] = pCTX->frametag;
            break;

    default:
        SEC_OSAL_Log(SEC_LOG_ERROR, "SsbSipMfcDecGetConfig: No such conf_type is supported.\n");
        return MFC_DEC_GET_CONF_FAIL;
    }

    return MFC_RET_OK;
}

int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos)
{
    int pixel_x_m1, pixel_y_m1;
    int roundup_x, roundup_y;
    int linear_addr0, linear_addr1, bank_addr ;
    int x_addr;
    int trans_addr;

    pixel_x_m1 = x_size -1;
    pixel_y_m1 = y_size -1;

    roundup_x = ((pixel_x_m1 >> 7) + 1);
    roundup_y = ((pixel_x_m1 >> 6) + 1);

    x_addr = x_pos >> 2;

    if ((y_size <= y_pos+32) && ( y_pos < y_size) &&
        (((pixel_y_m1 >> 5) & 0x1) == 0) && (((y_pos >> 5) & 0x1) == 0)) {
        linear_addr0 = (((y_pos & 0x1f) <<4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 6) & 0x3f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    } else {
        linear_addr0 = (((y_pos & 0x1f) << 4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 5) & 0x7f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    }

    linear_addr0 = linear_addr0 << 2;
    trans_addr = (linear_addr1 <<13) | (bank_addr << 11) | linear_addr0;

    return trans_addr;
}

void Y_tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size)
{
    int trans_addr;
    unsigned int i, j, k, index;
    unsigned char data8[4];
    unsigned int max_index = x_size * y_size;

    for (i = 0; i < y_size; i = i + 16) {
        for (j = 0; j < x_size; j = j + 16) {
            trans_addr = tile_4x2_read(x_size, y_size, j, i);
            for (k = 0; k < 16; k++) {
                /* limit check - prohibit segmentation fault */
                index = (i * x_size) + (x_size * k) + j;
                /* remove equal condition to solve thumbnail bug */
                if (index + 16 > max_index) {
                    continue;
                }

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 0];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 1];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 2];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 3];

                p_linear_addr[index] = data8[0];
                p_linear_addr[index + 1] = data8[1];
                p_linear_addr[index + 2] = data8[2];
                p_linear_addr[index + 3] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 4];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 5];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 6];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 7];

                p_linear_addr[index + 4] = data8[0];
                p_linear_addr[index + 5] = data8[1];
                p_linear_addr[index + 6] = data8[2];
                p_linear_addr[index + 7] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 8];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 9];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 10];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 11];

                p_linear_addr[index + 8] = data8[0];
                p_linear_addr[index + 9] = data8[1];
                p_linear_addr[index + 10] = data8[2];
                p_linear_addr[index + 11] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 12];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 13];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 14];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 15];

                p_linear_addr[index + 12] = data8[0];
                p_linear_addr[index + 13] = data8[1];
                p_linear_addr[index + 14] = data8[2];
                p_linear_addr[index + 15] = data8[3];
            }
        }
    }
}

void CbCr_tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size)
{
    int trans_addr;
    unsigned int i, j, k, index;
    unsigned char data8[4];
	unsigned int half_y_size = y_size / 2;
    unsigned int max_index = x_size * half_y_size;
    unsigned char *pUVAddr[2];
    
    pUVAddr[0] = p_linear_addr;
    pUVAddr[1] = p_linear_addr + ((x_size * half_y_size) / 2);
    
    for (i = 0; i < half_y_size; i = i + 16) {
        for (j = 0; j < x_size; j = j + 16) {
            trans_addr = tile_4x2_read(x_size, half_y_size, j, i);
            for (k = 0; k < 16; k++) {
                /* limit check - prohibit segmentation fault */
                index = (i * x_size) + (x_size * k) + j;
                /* remove equal condition to solve thumbnail bug */
                if (index + 16 > max_index) {
                    continue;
                }

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 0];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 1];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 2];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 3];

				pUVAddr[index%2][index/2] = data8[0];
				pUVAddr[(index+1)%2][(index+1)/2] = data8[1];
				pUVAddr[(index+2)%2][(index+2)/2] = data8[2];
				pUVAddr[(index+3)%2][(index+3)/2] = data8[3];

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 4];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 5];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 6];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 7];

				pUVAddr[(index+4)%2][(index+4)/2] = data8[0];
				pUVAddr[(index+5)%2][(index+5)/2] = data8[1];
				pUVAddr[(index+6)%2][(index+6)/2] = data8[2];
				pUVAddr[(index+7)%2][(index+7)/2] = data8[3];

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 8];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 9];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 10];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 11];

				pUVAddr[(index+8)%2][(index+8)/2] = data8[0];
				pUVAddr[(index+9)%2][(index+9)/2] = data8[1];
				pUVAddr[(index+10)%2][(index+10)/2] = data8[2];
				pUVAddr[(index+11)%2][(index+11)/2] = data8[3];

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 12];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 13];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 14];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 15];

				pUVAddr[(index+12)%2][(index+12)/2] = data8[0];
				pUVAddr[(index+13)%2][(index+13)/2] = data8[1];
				pUVAddr[(index+14)%2][(index+14)/2] = data8[2];
				pUVAddr[(index+15)%2][(index+15)/2] = data8[3];
            }
        }
    }
}
