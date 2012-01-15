/*
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Global header for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, Licensed under the Apache License, Version 2.0 (the "License");
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

#ifndef __MFC_INTERFACE_H
#define __MFC_INTERFACE_H __FILE__

#include "mfc_errno.h"
#include "SsbSipMfcApi.h"

#define IOCTL_MFC_MPEG4_DEC_INIT				(0x00800001)
#define IOCTL_MFC_MPEG4_ENC_INIT				(0x00800002)
#define IOCTL_MFC_MPEG4_DEC_EXE					(0x00800003)
#define IOCTL_MFC_MPEG4_ENC_EXE					(0x00800004)

#define IOCTL_MFC_H264_DEC_INIT					(0x00800005)
#define IOCTL_MFC_H264_ENC_INIT					(0x00800006)
#define IOCTL_MFC_H264_DEC_EXE					(0x00800007)
#define IOCTL_MFC_H264_ENC_EXE					(0x00800008)

#define IOCTL_MFC_H263_DEC_INIT					(0x00800009)
#define IOCTL_MFC_H263_ENC_INIT					(0x0080000A)
#define IOCTL_MFC_H263_DEC_EXE					(0x0080000B)
#define IOCTL_MFC_H263_ENC_EXE					(0x0080000C)

#define IOCTL_MFC_VC1_DEC_INIT					(0x0080000D)
#define IOCTL_MFC_VC1_DEC_EXE					(0x0080000E)

#define IOCTL_MFC_GET_LINE_BUF_ADDR				(0x0080000F)
#define IOCTL_MFC_GET_RING_BUF_ADDR				(0x00800010)
#define IOCTL_MFC_GET_FRAM_BUF_ADDR				(0x00800011)
#define IOCTL_MFC_GET_POST_BUF_ADDR				(0x00800012)
#define IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR			(0x00800013)
#define IOCTL_MFC_GET_CONFIG					(0x00800016)
#define IOCTL_MFC_GET_MPEG4_ASP_PARAM			(0x00800017)

#define IOCTL_MFC_SET_H263_MULTIPLE_SLICE		(0x00800014)
#define IOCTL_MFC_SET_CONFIG					(0x00800015)

#define IOCTL_MFC_GET_DBK_BUF_ADDR				(0x00800018)	// yj

#define IOCTL_MFC_SET_DISP_CONFIG				(0x00800111)
#define IOCTL_MFC_GET_FRAME_SIZE				(0x00800112)
#define IOCTL_MFC_SET_PP_DISP_SIZE				(0x00800113)
#define IOCTL_MFC_SET_DEC_INBUF_TYPE			(0x00800114)

#define IOCTL_VIRT_TO_PHYS						0x12345678

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
#define IOCTL_CACHE_FLUSH_B_FRAME				(0x00800115)
#define IOCTL_MFC_GET_PHY_B_FRAME_BUF_ADDR		(0x00800116)
#define IOCTL_MFC_GET_B_FRAME_BUF_ADDR			(0x00800117)
#endif

typedef struct
{
	int  rotate;
	int  deblockenable;
} MFC_DECODE_OPTIONS;

/* MFC H/W support maximum 32 extra DPB. */
#define MFC_MAX_EXTRA_DPB               5
#define MFC_MAX_DISP_DELAY              0xF

#define MFC_LIB_VER_MAJOR               1
#define MFC_LIB_VER_MINOR               00

#define BUF_L_UNIT			(1024)
#define Align(x, alignbyte)		(((x)+(alignbyte)-1)/(alignbyte)*(alignbyte))

// Physical Base Address for the MFC Data Buffer
// Data Buffer = {STRM_BUF, FRME_BUF}
//#define S3C6400_BASEADDR_MFC_DATA_BUF	0x57316000
#define BUF_SIZE						0x489400	// input and output buffer size
#define VIDEO_BUFFER_SIZE				(150*1024)	// 150KB

typedef enum {
    MFC_UNPACKED_PB = 0,
    MFC_PACKED_PB = 1
} mfc_packed_mode;


typedef enum
{
	MFC_USE_NONE = 0x00,
	MFC_USE_YUV_BUFF = 0x01,
	MFC_USE_STRM_BUFF = 0x10
} s3c_mfc_interbuff_status;


struct mfc_strm_ref_buf_arg {
	unsigned int strm_ref_y;
	unsigned int mv_ref_yc;
};

struct mfc_frame_buf_arg {
	unsigned int luma;
	unsigned int chroma;
};


struct mfc_enc_init_arg {
    int	ret_code;			// [OUT] Return code
	int	in_width;			// [IN]  width  of YUV420 frame to be encoded
	int	in_height;			// [IN]  height of YUV420 frame to be encoded
	int	in_bitrate;			// [IN]  Encoding parameter: Bitrate (kbps)
	int	in_gopNum;			// [IN]  Encoding parameter: GOP Number (interval of I-frame)
	int	in_frameRateRes;	// [IN]  Encoding parameter: Frame rate (Res)
	int	in_frameRateDiv;	// [IN]  Encoding parameter: Frame rate (Divider)
};

struct mfc_enc_exe_arg {
	int	ret_code;			// [OUT] Return code
	int	out_encoded_size;	// [OUT] Length of Encoded video stream
	int	out_header_size;	// [OUT] Length of video stream header
	int	out_header0_size;	// [OUT] Length of video stream header Sps (for avc) / VOL (for mpeg4)
	int	out_header1_size;	// [OUT] Length of video stream header Pps (for avc)	 / VOS (for mpeg4)
	int	out_header2_size;	// [OUT] Length of video stream header VO(VIS) (only for mpeg4)	
};

struct mfc_dec_init_arg {
	int	ret_code;			// [OUT] Return code
	int	in_strmSize;		// [IN]  Size of video stream filled in STRM_BUF
	int	out_width;			// [OUT] width  of YUV420 frame
	int	out_height;			// [OUT] height of YUV420 frame
	int	out_buf_width;		// [OUT] buffer's width of YUV420 frame
	int	out_buf_height;		// [OUT] buffer's height of YUV420 frame
};

struct mfc_dec_exe_arg {
	int	ret_code;			// [OUT] Return code
	int	in_strmSize;		// [IN]  Size of video stream filled in STRM_BUF
};

struct mfc_get_config_arg {
	int	ret_code;				// [OUT] Return code
	int	in_config_param;		// [IN]  Configurable parameter type
	int	in_config_param2;
	int	out_config_value[2];	// [IN]  Values to get for the configurable parameter.
    // Maximum two integer values can be obtained;
};

struct mfc_set_config_arg {
	int	ret_code;					// [OUT] Return code
	int	in_config_param;			// [IN]  Configurable parameter type
	int	in_config_value[3];			// [IN]  Values to be set for the configurable parameter.
    // Maximum two integer values can be set.
	int	out_config_value_old[2];	// [OUT] Old values of the configurable parameters
};

struct mfc_get_real_addr_arg {
	unsigned int key;
	unsigned int addr;
};

struct mfc_get_buf_addr_arg {
	int	ret_code;			// [OUT] Return code
	int	in_usr_data;		// [IN]  User data for translating Kernel-mode address to User-mode address
	int	in_usr_data2;
	int	out_buf_addr;		// [OUT] Buffer address
	int	out_buf_size;		// [OUT] Size of buffer address
};

struct mfc_buf_free_arg {
	unsigned int addr;
};


struct mfc_get_mpeg4asp_arg {
	int				in_usr_mapped_addr;
	int				ret_code;			// [OUT] Return code
	int				mv_addr;
	int				mb_type_addr;
	unsigned int	mv_size;
	unsigned int	mb_type_size;
	unsigned int	mp4asp_vop_time_res;
	unsigned int	byte_consumed;
	unsigned int	mp4asp_fcode;
	unsigned int	mp4asp_time_base_last;
	unsigned int	mp4asp_nonb_time_last;
	unsigned int	mp4asp_trd;
};

/*
 * structure of DBK_BUF is used for ioctl(IOCTL_MFC_GET_DBK_BUF_ADDR)
 *
 * 2009.5.12 by yj (yunji.kim@samsung.com)
 */
struct mfc_get_dbk_buf_arg {
	int	ret_code;				// [OUT] Return code
	int	in_usr_mapped_addr;		// [IN]  User data for translating Kernel-mode address to User-mode address
	int	out_buf_addr;			// [OUT] Buffer address
	int	out_buf_size;			// [OUT] Size of buffer address
};

union mfc_args {
	struct mfc_enc_init_arg		enc_init;
	struct mfc_enc_exe_arg			enc_exe;
	struct mfc_dec_init_arg		dec_init;
	struct mfc_dec_exe_arg			dec_exe;
	struct mfc_get_buf_addr_arg	get_buf_addr;
	struct mfc_get_config_arg		get_config;
	struct mfc_set_config_arg		set_config;
	struct mfc_get_mpeg4asp_arg	mpeg4_asp_param;
	struct mfc_get_dbk_buf_arg		get_dbkbuf_addr;	// yj
};

struct mfc_common_args {
	enum mfc_ret_code ret_code;	/* [OUT] error code */
	union mfc_args args;
};

struct mfc_crc_info {
    unsigned int luma;
    unsigned int chroma;
};


typedef struct
{
	int		magic;
	int		hOpen;
	void	*p_buf;
	int		size;
	int		fInit;
    
	unsigned char	*mapped_addr;
	unsigned int	width, height, buf_width, buf_height;
	unsigned int    frametag;
	int     		decoder;
    
    /*buffer addresses*/
    struct mfc_crc_info virStrmBuf;
    struct mfc_crc_info phyStrmBuf;
    
    struct mfc_crc_info virFrmBuf;
    struct mfc_crc_info phyFrmBuf;
    
    int sizeStrmBuf;
    
    /*encoding part*/
    int     encoder;
    int     enc_strm_size;
	int		enc_hdr_size;
    unsigned int  	framerate, bitrate;
	unsigned int  	gop_num; 
    
} _MFCLIB;

#define MFC_GET_CONFIG_DEC_FRAME_NEED_COUNT			(0x0AA0C001)
#define MFC_GET_CONFIG_DEC_MP4ASP_MV				(0x0AA0C002)
#define MFC_GET_CONFIG_DEC_MP4ASP_MBTYPE			(0x0AA0C003)
// RainAde for Encoding pic type (I/P)
#define MFC_GET_CONFIG_ENC_PIC_TYPE					(0x0AA0C004)
// RainAde : added to get crop information (6410 since FW 1.3.E)
#define MFC_GET_CONFIG_DEC_H264_CROPINFO			(0x0AA0C005)

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
#define MFC_GET_CONFIG_DEC_MP4ASP_FCODE				(0x0AA0C011)
#define MFC_GET_CONFIG_DEC_MP4ASP_VOP_TIME_RES		(0x0AA0C012)
#define MFC_GET_CONFIG_DEC_MP4ASP_TIME_BASE_LAST	(0x0AA0C013)
#define MFC_GET_CONFIG_DEC_MP4ASP_NONB_TIME_LAST	(0x0AA0C014)
#define MFC_GET_CONFIG_DEC_MP4ASP_TRD				(0x0AA0C015)
#define MFC_GET_CONFIG_DEC_BYTE_CONSUMED			(0x0AA0C016)
#endif

#define MFC_SET_CONFIG_DEC_ROTATE					(0x0ABDE001)
#define MFC_SET_CONFIG_DEC_OPTION					(0x0ABDE002)
#define MFC_SET_PADDING_SIZE						(0x0ABDE003)
#define MFC_SET_CONFIG_MP4_DBK_ON					(0x0ABDE004)

#define MFC_SET_CONFIG_ENC_H263_PARAM				(0x0ABDC001)
#define MFC_SET_CONFIG_ENC_SLICE_MODE				(0x0ABDC002)
#define MFC_SET_CONFIG_ENC_PARAM_CHANGE				(0x0ABDC003)
#define MFC_SET_CONFIG_ENC_CUR_PIC_OPT				(0x0ABDC004)
// RainAde : for setting RC parameters 
#define MFC_SET_CONFIG_ENC_SEQ_RC_PARA				(0x0ABDC005)
// RainAde : for setting Picture Quantization Step on RC disable
#define MFC_SET_CONFIG_ENC_PIC_QS					(0x0ABDC006)

#define MFC_SET_CACHE_CLEAN							(0x0ABDD001)
#define MFC_SET_CACHE_INVALIDATE					(0x0ABDD002)
#define MFC_SET_CACHE_CLEAN_INVALIDATE				(0x0ABDD003)

#define ENC_PARAM_GOP_NUM							(0x7000A001)
#define ENC_PARAM_INTRA_QP							(0x7000A002)
#define ENC_PARAM_BITRATE							(0x7000A003)
#define ENC_PARAM_F_RATE							(0x7000A004)
#define ENC_PARAM_INTRA_REF							(0x7000A005)
#define ENC_PARAM_SLICE_MODE						(0x7000A006)

#define ENC_PIC_OPT_IDR								(0x7000B001)
#define ENC_PIC_OPT_SKIP							(0x7000B002)
#define ENC_PIC_OPT_RECOVERY						(0x7000B003)

#define DEC_PIC_OPT_MP4ASP							(0x7000C001)

#define ENC_PROFILE_LEVEL(profile, level)      ((profile) | ((level) << 8))
#define ENC_RC_QBOUND(min_qp, max_qp)          ((min_qp) | ((max_qp) << 8))

#define ALIGN_TO_16B(x)   ((((x) + (1 <<  4) - 1) >>  4) <<  4)
#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_64B(x)   ((((x) + (1 <<  6) - 1) >>  6) <<  6)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_2KB(x)   ((((x) + (1 << 11) - 1) >> 11) << 11)
#define ALIGN_TO_4KB(x)   ((((x) + (1 << 12) - 1) >> 12) << 12)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)
#define ALIGN_TO_64KB(x)  ((((x) + (1 << 16) - 1) >> 16) << 16)
#define ALIGN_TO_128KB(x) ((((x) + (1 << 17) - 1) >> 17) << 17)

#endif /* __MFC_INTERFACE_H */
