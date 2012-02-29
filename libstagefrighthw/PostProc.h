/*
 * libstagefrighthw/PostProc.h
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

#ifndef _PostProc_h
#define _PostProc_h

#include <hardware/hardware.h>
#include <hardware/overlay.h>

extern "C" {
#include "v4l2_utils.h"
}

#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include "linux/fb.h"

/*****************************************************************************/

#if 0
#pragma mark -
#pragma mark Defines
#endif

#define NUM_BUFFERS (2)

#define DEFAULT_FB_INDEX (0)
#define V4L2_DEVICE "/dev/video1"

#define CACHEABLE_BUFFERS 0x1 //Not sure about this one

/* shared with Camera/Video Playback HAL */
#define ALL_BUFFERS_FLUSHED -66

/*****************************************************************************/

struct frame {
    int32_t  x;
    int32_t  y;
    uint32_t w;
    uint32_t h;
    int32_t  format;
    int32_t  rotation;
};


#if 0
#pragma mark -
#pragma mark FIMC_Class
#endif

class PostProc {
    int mVideoFd;
    
    uint32_t mNumBufs;
    uint32_t mQdBuffers;
    
    struct frame mInputFrame;
    struct frame mOutputFrame;
    
    bool mIsStreaming;
    bool mIsReady;
    bool mNeedsReset;
    
    uint32_t mPhyAddress;
    
    pthread_mutex_t lock;
    pthread_mutexattr_t attr;
    
public:
    PostProc();
    
    int init(uint32_t w, uint32_t h, int32_t format, int pmem_fd);
    int closePostProc();
    
    int setInput(uint32_t w, uint32_t h, int32_t format);
    int setOutput(uint32_t w, uint32_t h, int32_t format, int32_t rotation);
    
    int queueBuffer(void *buffer);
    int dequeueBuffer(void **buffer);
    
    int getBufferCount();
    
protected:
    ~PostProc();
    
private:
    int enableStreaming();
    int disableStreaming();
};

#if 0
#pragma mark -
#pragma mark Utility Funcs
#endif

static int32_t  set_color_space(int32_t format);
static int      check_src_constraints(struct frame * frame);
static int      check_dst_constraints(struct frame * frame);
static int      get_fb_screeninfo(struct frame * frame);

#endif
