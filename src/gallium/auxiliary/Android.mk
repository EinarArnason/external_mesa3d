# Mesa 3-D graphics library
#
# Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
# Copyright (C) 2010-2011 LunarG Inc.
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

LOCAL_PATH := $(call my-dir)

# get C_SOURCES and GENERATED_SOURCES
include $(LOCAL_PATH)/Makefile.sources

include $(CLEAR_VARS)

# filter-out tessellator/tessellator.hpp to avoid "Unused source files" error
LOCAL_SRC_FILES := \
	$(filter-out tessellator/tessellator.hpp, $(C_SOURCES)) \
	$(NIR_SOURCES) \
	$(RENDERONLY_SOURCES) \
	$(VL_STUB_SOURCES)

ifeq ($(USE_LIBBACKTRACE),true)
	LOCAL_CFLAGS += -DHAVE_ANDROID_PLATFORM
	LOCAL_SHARED_LIBRARIES += libbacktrace
	LOCAL_SRC_FILES += ../../util/u_debug_stack_android.cpp
endif

LOCAL_C_INCLUDES := \
	$(GALLIUM_TOP)/auxiliary/util \
	$(MESA_TOP)/src/util

ifeq ($(MESA_ENABLE_LLVM),true)
LOCAL_SRC_FILES += \
	$(GALLIVM_SOURCES)
$(call mesa-build-with-llvm)
endif

LOCAL_CPPFLAGS += -std=c++14

# We need libmesa_nir to get NIR's generated include directories.
LOCAL_MODULE := libmesa_gallium
LOCAL_SHARED_LIBRARIES += libsync
LOCAL_STATIC_LIBRARIES += libmesa_nir

LOCAL_WHOLE_STATIC_LIBRARIES += cpufeatures
LOCAL_CFLAGS += -DHAS_ANDROID_CPUFEATURES

# generate sources
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
intermediates := $(call local-generated-sources-dir)
LOCAL_GENERATED_SOURCES := $(addprefix $(intermediates)/, $(GENERATED_SOURCES))

u_indices_gen_deps := \
	$(MESA_TOP)/src/gallium/auxiliary/indices/u_indices_gen.py

$(intermediates)/indices/u_indices_gen.c: $(u_indices_gen_deps)
	@mkdir -p $(dir $@)
	$(hide) $(MESA_PYTHON3) $< > $@

u_unfilled_gen_deps := \
	$(MESA_TOP)/src/gallium/auxiliary/indices/u_unfilled_gen.py

$(intermediates)/indices/u_unfilled_gen.c: $(u_unfilled_gen_deps)
	@mkdir -p $(dir $@)
	$(hide) $(MESA_PYTHON3) $< > $@

$(intermediates)/util/u_tracepoints.c: $(prebuilt_intermediates)/util/u_tracepoints.c
	@mkdir -p $(dir $@)
	@cp -f $< $@

$(intermediates)/util/u_tracepoints.h: $(prebuilt_intermediates)/util/u_tracepoints.h
	@mkdir -p $(dir $@)
	@cp -f $< $@

LOCAL_GENERATED_SOURCES += $(MESA_GEN_NIR_H)

include $(GALLIUM_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# Build libmesa_galliumvl used by radeonsi
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(VL_SOURCES)

LOCAL_MODULE := libmesa_galliumvl

include $(GALLIUM_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
