
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	src/SsbSipMfcDecAPI.c

LOCAL_MODULE := libsecmfcdecapi.s5p6442

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES := libutils             \
        libcutils            \
        libui                \
        libdl                \
	libsecosal.s5p6442

LOCAL_C_INCLUDES := \
	$(SEC_CODECS)/video/mfc_c110/include \
	$(SEC_CODECS)/../sec_osal

include $(BUILD_STATIC_LIBRARY)

