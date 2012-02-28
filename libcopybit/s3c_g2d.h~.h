/* g2d/s3c_g2d.h
 *
 * Copyright (c) 2010 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * Samsung S3C G2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _S3C_G2D_H_
#define _S3C_G2D_H_

#include <linux/ioctl.h>

#define G2D_IOCTL_MAGIC			'G'

#define S3C_G2D_ROTATOR_0		_IO(G2D_IOCTL_MAGIC,0)
#define S3C_G2D_ROTATOR_90		_IO(G2D_IOCTL_MAGIC,1)
#define S3C_G2D_ROTATOR_180		_IO(G2D_IOCTL_MAGIC,2)
#define S3C_G2D_ROTATOR_270		_IO(G2D_IOCTL_MAGIC,3)
#define S3C_G2D_ROTATOR_X_FLIP		_IO(G2D_IOCTL_MAGIC,4)
#define S3C_G2D_ROTATOR_Y_FLIP		_IO(G2D_IOCTL_MAGIC,5)

/*
 * S3C_G2D_BITBLT
 * Start hardware bitblt operation.
 * Argument:	a pointer to struct s3c_g2d_req with operation parameters
 * Returns:	  0 on success,
 *		< 0, on error
 */
#define S3C_G2D_BITBLT			_IOW(G2D_IOCTL_MAGIC, 6, struct s3c_g2d_req)

/*
 * S3C_G2D_FILLRECT
 * Start hardware fillrect operation.
 * Argument:	a pointer to struct s3c_g2d_fillrect with operation parameters
 * Returns:	  0 on success,
 *		< 0, on error
 */
#define S3C_G2D_FILLRECT		_IOW(G2D_IOCTL_MAGIC, 7, struct s3c_g2d_fillrect)


/*
 * S3C_G2D_SET_ALPHA_VAL
 * Set requested plane alpha value.
 * Argument:	a value from <0, ALPHA_VALUE_MAX> range
 */
#define S3C_G2D_SET_ALPHA_VAL		_IO(G2D_IOCTL_MAGIC, 8)
#define ALPHA_VALUE_MAX			255

/*
 * S3C_G2D_SET_RASTER_OP
 * Set requested raster operation. 
 * Argument:	an 8-bit value defining the operation
 */
#define S3C_G2D_SET_RASTER_OP		_IO(G2D_IOCTL_MAGIC, 9)
#define G2D_ROP_SRC_ONLY		(0xCC)
#define G2D_ROP_3RD_OPRND_ONLY		(0xF0)
#define G2D_ROP_DST_ONLY		(0xAA)
#define G2D_ROP_SRC_OR_DST		(0xEE)
#define G2D_ROP_SRC_OR_3RD_OPRND	(0xFC)
#define G2D_ROP_SRC_AND_DST		(0x88) 
#define G2D_ROP_SRC_AND_3RD_OPRND	(0xC0)
#define G2D_ROP_SRC_XOR_3RD_OPRND	(0x3C)
#define G2D_ROP_DST_OR_3RD_OPRND	(0xFA)

/*
 * S3C_G2D_SET_BLENDING
 * Set requested alpha blending mode.
 * Argument:	one of G2D_*_ALPHA values
 */
#define S3C_G2D_SET_BLENDING		_IO(G2D_IOCTL_MAGIC, 10)
typedef enum
{
	G2D_NO_ALPHA_BLEND_MODE,
	G2D_EN_ALPHA_BLEND_MODE,
	G2D_EN_ALPHA_BLEND_CONST_ALPHA,
	G2D_EN_ALPHA_BLEND_PERPIXEL_ALPHA,
	G2D_EN_FADING_MODE
} G2D_ALPHA_BLENDING_MODE;

/* Maximum values for the hardware */
#define G2D_MAX_WIDTH				(8000)
#define G2D_MAX_HEIGHT				(8000)

#define S3C_G2D_SET_TRANSFORM		_IO(G2D_IOCTL_MAGIC, 12)
enum
{
	G2D_ROT_0	= 1 << 0,
	G2D_ROT_90	= 1 << 1,
	G2D_ROT_180	= 1 << 2,
	G2D_ROT_270	= 1 << 3,
	G2D_ROT_FLIP_X	= 1 << 4,
	G2D_ROT_FLIP_Y	= 1 << 5
};

/* Image data */
struct s3c_g2d_image
{
	uint32_t	base;	// image base address (NULL to use fd)
	int	        fd;	// image file descriptor (for PMEM)
	uint32_t	offs;	// buffer offset
	uint32_t	w;	// image full width
	uint32_t	h;	// image full height
	uint32_t	l;	// x coordinate of left edge
	uint32_t	t;	// y coordinate of top edge
	uint32_t	r;	// x coordinage of right edge
	uint32_t	b;	// y coordinate of bottom edge
	uint32_t	fmt;	// color format
};

typedef enum
{
	G2D_RGBA_8888 = 1,
	G2D_RGBX_8888 = 2,
	G2D_ARGB_8888 = 3,
	G2D_XRGB_8888 = 4,
	G2D_BGRA_8888 = 5,
	G2D_BGRX_8888 = 6,
	G2D_ABGR_8888 = 7,
	G2D_XBGR_8888 = 8,
	G2D_RGB_888   = 9,
	G2D_BGR_888   = 10,
	G2D_RGB_565   = 11,
	G2D_BGR_565   = 12,
	G2D_RGBA_5551 = 13,
	G2D_ARGB_5551 = 14,
	G2D_RGBA_4444 = 15,
	G2D_ARGB_4444 = 16
} G2D_COLOR_FMT;

/* Bitblt request */
struct s3c_g2d_req
{
	struct s3c_g2d_image src; // source image
	struct s3c_g2d_image dst; // destination image
};

/* Fillrect request */
struct s3c_g2d_fillrect
{
	struct s3c_g2d_image dst;
	uint32_t color;
	uint8_t alpha;
};

#endif
