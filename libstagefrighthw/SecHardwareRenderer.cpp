/*
 * Copyright (C) 2010 The Android Open Source Project
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

#define LOG_TAG "SecHardwareRenderer"
#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "SecHardwareRenderer.h"

#include <binder/MemoryHeapBase.h>
#include <binder/MemoryHeapPmem.h>
#include <media/stagefright/MediaDebug.h>
#include <surfaceflinger/ISurface.h>
#include <ui/Overlay.h>

#include <hardware/hardware.h>

#include "v4l2_utils.h"
#include "utils/Timers.h"
#include "PostProc.h"

#define CACHEABLE_BUFFERS 0x1

#define USE_ZERO_COPY
#define SEC_DEBUG

#define LCD_WIDTH 400
#define LCD_HEIGHT 240
#define OUTPUT_BPP 16

struct rect {
    int w;
    int w_stride;
    int h;
    int h_stride;
};

namespace android {
    
    ////////////////////////////////////////////////////////////////////////////////
    
    
    static int setRect(struct rect *orig, struct rect *dest) {
        int ret = 0;
        
        if (orig->w <= 0 || orig->h <= 0) return -1;
        
        float display_aspect = (float)LCD_WIDTH / (float)LCD_HEIGHT;
        float frame_aspect = (float)orig->w / (float)orig->h;
        
        LOGD("display aspect %f", display_aspect);
        LOGD("frame aspect %f", frame_aspect);
        
        bool use_width = (display_aspect < frame_aspect) ? true : false;
        
        if (use_width) {    
            dest->w = LCD_WIDTH;
            dest->h = LCD_HEIGHT;
            dest->w_stride = dest->w;
            dest->h_stride = (LCD_WIDTH * orig->h) / orig->w;
        } else {
            dest->h = LCD_HEIGHT;
            dest->h_stride = LCD_HEIGHT;
            dest->w = LCD_WIDTH;
            dest->w_stride = (LCD_HEIGHT * orig->w) / orig->h;
        }
        
        LOGD("setRect = %d %d", dest->w, dest->h);
        
        return ret;
    }
    
    SecHardwareRenderer::SecHardwareRenderer(
                                             const sp<ISurface> &surface,
                                             size_t displayWidth, size_t displayHeight,
                                             size_t decodedWidth, size_t decodedHeight,
                                             OMX_COLOR_FORMATTYPE colorFormat,
                                             int32_t rotationDegrees,
                                             bool fromHardwareDecoder)
    : mISurface(surface),
    mDisplayWidth(displayWidth),
    mDisplayHeight(displayHeight),
    mDecodedWidth(decodedWidth),
    mDecodedHeight(decodedHeight),
    mColorFormat(colorFormat),
    mInitCheck(NO_INIT),
    mFrameSize(mDecodedWidth * mDecodedHeight * 2),
    mIsFirstFrame(true),
    mCustomFormat(false),
    mIndex(0) {
        
        CHECK(mISurface.get() != NULL);
        CHECK(mDecodedWidth > 0);
        CHECK(mDecodedHeight > 0);
        
        if (colorFormat != OMX_COLOR_FormatCbYCrY
            && colorFormat != OMX_COLOR_FormatYUV420Planar
            && colorFormat != OMX_COLOR_FormatYUV420SemiPlanar) {
            LOGE("Invalid colorFormat (0x%x)", colorFormat);
            return;
        }
        
        uint32_t orientation;
        switch (rotationDegrees) {
            case 0: orientation = ISurface::BufferHeap::ROT_0; break;
            case 90: orientation = ISurface::BufferHeap::ROT_90; break;
            case 180: orientation = ISurface::BufferHeap::ROT_180; break;
            case 270: orientation = ISurface::BufferHeap::ROT_270; break;
            default: orientation = ISurface::BufferHeap::ROT_0; break;
        }
        
        if (decodedWidth%16 != 0) {
            decodedWidth += 16 - decodedWidth%16;
        }
        
        mPostProc = new PostProc();
        if (mPostProc->init(decodedWidth, decodedHeight, colorFormat) < 0) {
            LOGE("Failed initializing PostProc");
            return;
        }
        
        mNumBuf = mPostProc->getBufferCount();
        
        struct rect Orig; 
        Orig.w = mDisplayWidth; 
        Orig.h = mDisplayHeight;
        
        struct rect Dest = { 0 };
        
        setRect(&Orig, &Dest);
        
        mVideoHeap = new MemoryHeapBase("/dev/pmem_render", Dest.w * Dest.h * 2);
        if (mVideoHeap->heapID() < 0) {
            LOGI("Creating physical memory over pmem_render failed");
        } else {
            mRenderPmem = new MemoryHeapPmem(mVideoHeap);
            mRenderPmem->slap();
            mVideoHeap = mRenderPmem;
        }
        
        CHECK(mVideoHeap->heapID() >= 0);
        
        ISurface::BufferHeap bufferHeap(
                                        Dest.w, Dest.h,
                                        Dest.w_stride, Dest.h_stride,
                                        PIXEL_FORMAT_RGB_565,
                                        mVideoHeap);
        
        status_t err = mISurface->registerBuffers(bufferHeap);
        
        if (err != OK) {
            LOGW("ISurface failed to register buffers (0x%08x)", err);
        }
        
        mFrameSize = 32;
        mMemoryHeap = new MemoryHeapBase(mNumBuf * mFrameSize);
        
        mPostProc->setOutput(Dest.w, Dest.h, OUTPUT_BPP, 0);
        
        mInitCheck = err;
    }
    
    SecHardwareRenderer::~SecHardwareRenderer() {
        
        mISurface->unregisterBuffers();
        if(mMemoryHeap != NULL)
            mMemoryHeap.clear();
        
        if (mRenderPmem != NULL) {
            mRenderPmem->unslap();
            mRenderPmem.clear();
        }
        
        if (mVideoHeap != NULL)
            mVideoHeap.clear();
        
        mPostProc->closePostProc();
        
        if (mOverlay.get() != NULL) {
            mOverlay->destroy();
            mOverlay.clear();
        }
    }
    
    void SecHardwareRenderer::handleYUV420Planar(
                                                 const void *data, size_t size) {
        
        int FrameSize;
        uint8_t* pPhyYAddr;
        uint8_t* pPhyCAddr;
        int AddrSize;
        size_t offset;
        
        CHECK(size >= (mDecodedWidth * mDecodedHeight * 3) / 2);
        
        offset = mIndex * mFrameSize;
        void *dst = (uint8_t *)mMemoryHeap->getBase() + offset;
        
        AddrSize = sizeof(void *);
        memcpy(&FrameSize, data, sizeof(FrameSize));
        memcpy(&pPhyYAddr, data + sizeof(FrameSize), sizeof(pPhyYAddr));
        memcpy(&pPhyCAddr, data + sizeof(FrameSize) + (AddrSize * 1), sizeof(pPhyCAddr));
        
        memcpy(dst , &pPhyYAddr, sizeof(pPhyYAddr));
        memcpy(dst  + sizeof(pPhyYAddr) , &pPhyCAddr, sizeof(pPhyCAddr));
        memcpy(dst  + sizeof(pPhyYAddr) + sizeof(pPhyCAddr), &mIndex, sizeof(mIndex));
    }
    
    void SecHardwareRenderer::render(
                                     const void *data, size_t size, void *platformPrivate) {
        
        overlay_buffer_t dst = (uint8_t *)mMemoryHeap->getBase() + mIndex*mFrameSize;
        
        if (mColorFormat == OMX_COLOR_FormatYUV420Planar ||
            mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
            handleYUV420Planar(data, size);
        }
        
        if (mPostProc->queueBuffer(dst) == ALL_BUFFERS_FLUSHED) {
            mIsFirstFrame = true;
            if (mPostProc->queueBuffer((void *)dst) != 0) {
                LOGV("QueueBuffer Failure");
                return;
            }
        }
        
        if (++mIndex == mNumBuf) {
            mIndex = 0;
        }
        
        overlay_buffer_t overlay_buffer;
        if (!mIsFirstFrame) {
            status_t err = mPostProc->dequeueBuffer(&overlay_buffer);
            if (err == ALL_BUFFERS_FLUSHED) {
                mIsFirstFrame = true;
            } else {
                mISurface->postBuffer(0);
                return;
            }
        } else {
            mIsFirstFrame = false;
        }
    }
    
}  // namespace android

