LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_SRC_FILES := \
			test_server.o cfg_ipc_server.c cfg_fdevent.c cfg_fdevent_select.c

LOCAL_CFLAGS :=-DUSE_SELECT

LOCAL_MODULE := cfg_ipc_server

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_SRC_FILES := \
			cfg_ipc_client.c

LOCAL_MODULE := cfg_ipc_client

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
