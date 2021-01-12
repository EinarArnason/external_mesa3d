# Copyright © 2021 Android-RPi Project
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

include $(CLEAR_VARS)
LOCAL_MODULE := libmesa_vulkan_broadcom
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

intermediates := $(call local-generated-sources-dir)
prebuilt_intermediates := $(MESA_TOP)/prebuilt-intermediates

LOCAL_SRC_FILES := $(BROADCOM_VULKAN_FILES)

LOCAL_STATIC_LIBRARIES := \
    libmesa_nir \
    libmesa_vulkan_util \
    libmesa_util \
    libmesa_broadcom_genxml

LOCAL_C_INCLUDES += \
    $(MESA_TOP)/src/gallium/auxiliary \
    $(MESA_TOP)/src/gallium/include \
    $(MESA_TOP)/src/util \
    $(MESA_TOP)/src/vulkan/util \
    $(MESA_TOP)/src/vulkan/wsi \
    $(MESA_TOP)/src/broadcom/vulkan \
    frameworks/native/libs/nativebase/include \
    frameworks/native/libs/nativewindow/include \
    frameworks/native/libs/arect/include \
    frameworks/native/vulkan/include \
    system/core/libsync/include \
    $(intermediates)

LOCAL_HEADER_LIBRARIES += \
    libcutils_headers \
    libsystem_headers \
    libhardware_headers \
    liblog_headers

LOCAL_CFLAGS += -DV3D_VERSION=42 -DDEBUG

LOCAL_GENERATED_SOURCES := \
    $(intermediates)/v3dv_entrypoints.h \
    $(intermediates)/v3dv_entrypoints.c \
    $(intermediates)/v3dv_extensions.h \
    $(intermediates)/v3dv_extensions.c

$(intermediates)/v3dv_entrypoints.h:$(prebuilt_intermediates)/v3dv/v3dv_entrypoints.h
	@mkdir -p $(dir $@)
	@cp -f $< $@

$(intermediates)/v3dv_entrypoints.c:$(prebuilt_intermediates)/v3dv/v3dv_entrypoints.c
	@mkdir -p $(dir $@)
	@cp -f $< $@

$(intermediates)/v3dv_extensions.h:$(prebuilt_intermediates)/v3dv/v3dv_extensions.h
	@mkdir -p $(dir $@)
	@cp -f $< $@

$(intermediates)/v3dv_extensions.c:$(prebuilt_intermediates)/v3dv/v3dv_extensions.c
	@mkdir -p $(dir $@)
	@cp -f $< $@

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
