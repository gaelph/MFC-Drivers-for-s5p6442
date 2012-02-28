LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq (BOARD_USES_HW_RENDER,true)
    LOCAL_SRC_FILES := \
        SEC_OMX_Plugin.cpp  \
        SecHardwareRenderer.cpp \
        stagefright_overlay_output.cpp \
        v4l2_utils.c \
        PostProc.cpp
else
    LOCAL_SRC_FILES := \
        SEC_OMX_Plugin.cpp
endif

LOCAL_CFLAGS += $(PV_CFLAGS_MINUS_VISIBILITY)



LOCAL_C_INCLUDES:= \
      $(TOP)/frameworks/base/include/media/stagefright/openmax \
      $(LOCAL_PATH)/../include 

LOCAL_SHARED_LIBRARIES :=    \
        libbinder            \
        libutils             \
        libcutils            \
        libui                \
        libdl                \
        libsurfaceflinger_client 

LOCAL_MODULE := libstagefrighthw

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
