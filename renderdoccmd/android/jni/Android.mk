# Copyright 2015 The Android Open Source Project
# Copyright (C) 2015 Valve Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(abspath $(call my-dir))
SRC_DIR := $(LOCAL_PATH)/../../..
CMD_DIR := $(SRC_DIR)/renderdoccmd

include $(CLEAR_VARS)
LOCAL_MODULE := renderdoccmd
LOCAL_SRC_FILES += $(CMD_DIR)/renderdoccmd.cpp \
                   $(CMD_DIR)/renderdoccmd_android.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/renderdoc/api sources/android/native_app_glue
LOCAL_CFLAGS += -DRENDERDOC_PLATFORM_POSIX -w
LOCAL_CPP_FEATURES += rtti
LOCAL_WHOLE_STATIC_LIBRARIES += android_native_app_glue
LOCAL_LDLIBS    := -llog -landroid
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
