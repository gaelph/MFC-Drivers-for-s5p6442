/*
 * libstagefrighthw/PostProc.cpp
 *
 * Copyrights:	2011 by Gaël PHILIPPE < gaelph at gmail.com >
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define LOG_TAG "PostProc"
#define LOGNDEBUG 0

#include "PostProc.h"
#include "v4l2_utils.h"

/*****************************************************************************/

#if 0
#pragma mark -
#pragma mark Defines
#endif

#define LOG_FUNCTION_NAME LOGV(" %s %s", __FILE__, __func__)

#define DEFAULT_LCD_WIDTH 240
#define DEFAULT_LCD_HEIGHT 400

/*****************************************************************************/

#if 0
#pragma mark -
#pragma mark Utility Funcs
#endif

static int32_t set_color_space(int32_t format) {
    int ret = format;
    
    switch (format) {
        case OVERLAY_FORMAT_RGB_565:
            ret = V4L2_PIX_FMT_RGB565;
            break;
            
        case OVERLAY_FORMAT_YCbYCr_422_I:
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_422_I:
            ret = V4L2_PIX_FMT_YUYV;
            break;
            
        case OVERLAY_FORMAT_CbYCrY_422_I:
        case HAL_PIXEL_FORMAT_CUSTOM_CbYCrY_422_I:
            ret = V4L2_PIX_FMT_UYVY;
            break;
            
        case HAL_PIXEL_FORMAT_YCbCr_420_P:
            ret = V4L2_PIX_FMT_YUV420;
            break;
            
        case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP:
            ret = V4L2_PIX_FMT_YUV420;//V4L2_PIX_FMT_NV12T;
            break;
            
        case HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP:
            ret = V4L2_PIX_FMT_YUV420;//V4L2_PIX_FMT_NV21;
            break;
            
        default :
            LOGE("unsupported pixel format (0x%x)", format);
            ret = -1;
    }
    
    return ret;
}

static int check_src_constraints(struct frame * frame) {
    int ret = 0;
    if (frame->w < 16 || frame->h < 8)
        return -1;
    
    switch (frame->format) {
        case V4L2_PIX_FMT_YUV420:
            if (frame->h%2 != 0) 
                frame->h -= frame->h%2;
            if (frame->w%2 != 0)
                frame->w -= frame->w%2;
            ret = 1;
            break;
            
        case V4L2_PIX_FMT_UYVY:
            if (frame->w%2 != 0)
                frame->w -= frame->w%2;
            ret = 1;
            break;
    }
    
    return ret;
}

static int check_dst_constraints(struct frame * frame) {
    int ret = 1;
    int tmp = 0;
    
    if ((frame->h > 0) && (frame->h < 16)) frame->h = 16;
    
    if (frame->w%8 != 0) {
        tmp = frame->w - (frame->w%8);
        if (tmp <= 0) 
            return -1;
        else
            frame->w = tmp;
    }
    
    return ret;
}

static int get_fb_screeninfo(struct frame * frame) {
    int ret = 0;
    
    int fd = -1;
    int i=0;
    char name[64];
    struct fb_var_screeninfo info;
        
    char const * const device_template[] = {
        "/dev/graphics/fb%u",
        "/dev/fb%u",
        0 };
    
    while ((fd==-1) && device_template[i]) {
        snprintf(name, 64, device_template[i], DEFAULT_FB_INDEX);
        fd = open(name, O_RDWR, 0);
        i++;
    }
    
    if (fd < 0)
        ret = -EINVAL;
    
    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
        ret = -EINVAL;
    
    if (fd > 0)
        close(fd);
    
    frame->w = info.xres;
    frame->h = info.yres;
    
    return ret;
}

/*****************************************************************************/

#if 0
#pragma mark -
#pragma mark Class FIMC
#endif

FIMC::FIMC() : mVideoFd(-1),
               mNumBufs(NUM_BUFFERS),
               mQdBuffers(0),
               mIsStreaming(false),
               mIsReady(false),
               mNeedsReset(true) {
    memset(&mInputFrame, 0, sizeof(mInputFrame));
    memset(&mOutputFrame, 0, sizeof(mOutputFrame));
}

FIMC::~FIMC() {}

int FIMC::init(uint32_t w, uint32_t h, int32_t format) {
    int ret = 0;
    
    LOG_FUNCTION_NAME;
    
    if (format == OVERLAY_FORMAT_DEFAULT) {
        LOGD("format == OVERLAY_FORMAT_DEFAULT\n");
        LOGD("set to HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP\n");
        format = HAL_PIXEL_FORMAT_CUSTOM_YCrCb_420_SP;
    }
    
    if (pthread_mutexattr_init(&attr) != 0) {
        LOGE("Failed to initialize overlay mutex attr");
        goto error;
    }
    
    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
        LOGE("Failed to set the overlay mutex attr to be shared across-processes");
        pthread_mutexattr_destroy(&attr);
        goto error;
    }
    
    if (pthread_mutex_init(&lock, &attr) !=0) {
        LOGE("Failed to initialize overlay mutex\n");
        pthread_mutexattr_destroy(&attr);
        goto error;
    }
    
    mVideoFd = v4l2_overlay_open(V4L2_DEVICE);
    if (mVideoFd < 0) {
        LOGE("Failed to open overlay device : %s\n", strerror(errno));
        goto error;
    }
    
    mInputFrame.w = w;
    mInputFrame.h = h;
    mInputFrame.format = set_color_space(format);
    
    ret = check_src_constraints(&mInputFrame);
    if (ret != 0) {
        if (ret < 0) {
            LOGE("Not supported Image Size");
            goto error1;
        } else {
            LOGD("src width, height are changed [w= %d, h= %d]->[w=%d, h= %d]", w, h, mInputFrame.w, mInputFrame.h);
            w = mInputFrame.w;
            h = mInputFrame.h;
        }
    }
    
    if (v4l2_overlay_init(mVideoFd, w, h, format, 0)) {
        LOGE("Failed initializing overlays\n");
        goto error1;
    }
    
    if (v4l2_overlay_set_crop(mVideoFd, 0, 0, w, h)) {
        LOGE("Failed defaulting crop window\n");
        goto error1;
    }
    
    if (v4l2_overlay_set_rotation(mVideoFd, 0, 0)) {
        LOGE("Failed defaulting rotation\n");
        goto error1;
    }
    
    if (v4l2_overlay_req_buf(mVideoFd, &mNumBufs, 0, 1)) {
        LOGE("Failed requesting buffers\n");
        goto error1;
    }
    
    if (get_fb_screeninfo(&mOutputFrame) < 0) {
        LOGE("Failed Getting Info from fb"); 
        mOutputFrame.w = DEFAULT_LCD_WIDTH;
        mOutputFrame.h = DEFAULT_LCD_HEIGHT;
    }
    
    LOGI("Successfully Opened FIMC Device");
    return ret;

error1:
    close(mVideoFd);
error:
    return -1;
}

int FIMC::closeFIMC() {
    LOG_FUNCTION_NAME;
    
    if (pthread_mutex_destroy(&lock)) {
        LOGE("Failed to uninitialize overlay mutex!\n");
    }
    
    if (pthread_mutexattr_destroy(&attr)) {
        LOGE("Failed to uninitialize the overlay mutex attr!\n");
    }
    
    return close(mVideoFd);
}

int FIMC::enableStreaming() {
    int ret = 0;
    
    LOG_FUNCTION_NAME;
    
    if (!mIsReady) {
        LOGW("Should not Enable Streaming just yet");
    } else {
        mIsStreaming = true;
        ret = v4l2_overlay_stream_on(mVideoFd);
        if (ret) {
            LOGE("Stream Enable Failed! /%d", ret);
            mIsStreaming = false;
        }
    }
    
    return ret;
}

int FIMC::disableStreaming() {
    int ret = 0;
    
    LOG_FUNCTION_NAME;
    
    if (mIsStreaming) {
        ret = v4l2_overlay_stream_off(mVideoFd);
        if (ret) {
            LOGE("Stream Disable Failed! /%d", ret);
        } else {
            mNeedsReset = true;
            mIsStreaming = true;
        }
    }
    
    return ret;
}

int FIMC::setInput(uint32_t w, uint32_t h, int32_t format) {
    LOG_FUNCTION_NAME;
    
    int ret = 0;
    
    if ((w == mInputFrame.w) && (h == mInputFrame.h)) {
        LOGV("Size doesn't change");
        return 0;
    }
    
    if (mIsReady) {
        LOGV("QueueBuffer already called");
        return -1;
    }
    
    pthread_mutex_lock(&lock);
    
    if (disableStreaming() != 0) return -1;
    
    mInputFrame.w = w;
    mInputFrame.h = h;
    mInputFrame.format = set_color_space(format);
    
    ret = check_src_constraints(&mInputFrame);
    
    if (ret != 0) {
        if (ret < 0) {
            LOGE ("Not supported source image size");
            goto end;
        } else {
            LOGD("src width, height are changed [w= %d, h= %d]->[w=%d, h= %d]", w, h, mInputFrame.w, mInputFrame.h);
            w = mInputFrame.w;
            h = mInputFrame.h;
        }
    }
    
    ret = v4l2_overlay_init(mVideoFd, w, h, mInputFrame.format, 0);
    if (ret) {
        LOGE("Error initializing overlay");
        goto end;
    }
    
    if (v4l2_overlay_req_buf(mVideoFd, &mNumBufs, 0, 1)) {
        LOGE("Failed requesting buffers\n");
        goto end;
    }
    
    ret = v4l2_overlay_set_crop(mVideoFd, 0, 0, w, h);
    if (ret) {
        LOGE("Error setting crop window\n");
        goto end;
    }
    
    if (enableStreaming() != 0) return -1;

end:
    pthread_mutex_unlock(&lock);
    
    return ret;
}

int FIMC::setOutput(uint32_t w, uint32_t h, int32_t format, int32_t rotation) {
    LOG_FUNCTION_NAME;
    
    int ret = 0;
    
    mOutputFrame.w = w;
    mOutputFrame.h = h;
    mOutputFrame.format = V4L2_PIX_FMT_RGB565;
    
    pthread_mutex_lock(&lock);
    
    ret = check_dst_constraints(&mOutputFrame);
    if (ret != 0) {
        if (ret < 0) {
            LOGE("Unsupported destination image size");
            goto end;
        } else {
            LOGD("dst width, height have changed [w= %d, h= %d] -> [w=%d, h= %d]",
                 w, h, mOutputFrame.w, mOutputFrame.h);
            w = mOutputFrame.w;
            h = mOutputFrame.h;
        }
    }
    
    if (disableStreaming() != 0) goto end;
    
    if (mOutputFrame.rotation != rotation) {
        ret = v4l2_overlay_set_rotation(mVideoFd, rotation, 0);
        if (ret) {
            LOGE("Set Rotation Failed! /%d", ret);
            goto end;
        }
        mOutputFrame.rotation = rotation;
    }
    v4l2_overlay_s_fbuf(mVideoFd, rotation, 0);
    
    ret = v4l2_overlay_set_position(mVideoFd, 0, 0 , mOutputFrame.w, mOutputFrame.h, rotation);
    if (ret) {
        LOGE("Set Position Failed! /%d", ret);
        goto end;
    }
    
    if (enableStreaming() != 0) goto end;
 
end:
    pthread_mutex_unlock(&lock);
    return ret;
}

int FIMC::queueBuffer(void *buffer) {
    LOG_FUNCTION_NAME;
    
    int ret = 0;
    
    pthread_mutex_lock(&lock);
    if (mNeedsReset) {
        mNeedsReset = false;
        pthread_mutex_unlock(&lock);
        return ALL_BUFFERS_FLUSHED;
    }
    pthread_mutex_unlock(&lock);
    
    if (!mIsReady) {
        mIsReady = true;
        enableStreaming();
    }
    
    ret = v4l2_overlay_q_buf(mVideoFd, (int) buffer, 1);
    if ((ret == 0) && (mQdBuffers < mNumBufs)) {
        mQdBuffers++;
    }
    
    return ret;
}

int FIMC::dequeueBuffer(void **buffer) {
    LOG_FUNCTION_NAME;
    
    int ret = 0;
    int i = -1;
    
    pthread_mutex_lock(&lock);
    if (mNeedsReset) {
        mNeedsReset = false;
        pthread_mutex_unlock(&lock);
        return ALL_BUFFERS_FLUSHED;
    }
    pthread_mutex_unlock(&lock);
    
    if (mIsStreaming && mQdBuffers) {
        ret = v4l2_overlay_dq_buf(mVideoFd, &i, 1);
        if ( ret != 0) {
            LOGE("Failed to DQ /%d", ret);
        } else if ( i < 0 || i > mNumBufs) {
            ret = -EINVAL;
        } else {
            *((int *)buffer) = i;
            mQdBuffers--;
        }
    } else {
        ret = -1;
    }
    
    return ret;
}

int FIMC::getBufferCount() {
    LOG_FUNCTION_NAME;
    return mNumBufs;
}