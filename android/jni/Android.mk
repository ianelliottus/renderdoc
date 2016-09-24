# FIXME/TODO: May need a module called VkLayer_RenderDoc, in order to get
# libVkLayer_RenderDoc.so

LOCAL_PATH := $(abspath $(call my-dir))
SRC_DIR := $(LOCAL_PATH)/../../

include $(CLEAR_VARS)
LOCAL_MODULE := renderdoc
RDOC_SRC_DIR := $(SRC_DIR)/renderdoc/
LOCAL_SRC_FILES += $(RDOC_SRC_DIR)/common/common.cpp \
                   $(RDOC_SRC_DIR)/common/dds_readwrite.cpp \
                   $(RDOC_SRC_DIR)/core/core.cpp \
                   $(RDOC_SRC_DIR)/core/image_viewer.cpp \
                   $(RDOC_SRC_DIR)/core/target_control.cpp \
                   $(RDOC_SRC_DIR)/core/remote_server.cpp \
                   $(RDOC_SRC_DIR)/core/replay_proxy.cpp \
                   $(RDOC_SRC_DIR)/core/resource_manager.cpp \
                   $(RDOC_SRC_DIR)/data/glsl_shaders.cpp \
                   $(RDOC_SRC_DIR)/hooks/hooks.cpp \
                   $(RDOC_SRC_DIR)/maths/camera.cpp \
                   $(RDOC_SRC_DIR)/maths/matrix.cpp \
                   $(RDOC_SRC_DIR)/os/os_specific.cpp \
                   $(RDOC_SRC_DIR)/replay/app_api.cpp \
                   $(RDOC_SRC_DIR)/replay/capture_options.cpp \
                   $(RDOC_SRC_DIR)/replay/entry_points.cpp \
                   $(RDOC_SRC_DIR)/replay/replay_output.cpp \
                   $(RDOC_SRC_DIR)/replay/replay_renderer.cpp \
                   $(RDOC_SRC_DIR)/replay/type_helpers.cpp \
                   $(RDOC_SRC_DIR)/serialise/grisu2.cpp \
                   $(RDOC_SRC_DIR)/serialise/serialiser.cpp \
                   $(RDOC_SRC_DIR)/serialise/string_utils.cpp \
                   $(RDOC_SRC_DIR)/serialise/utf8printf.cpp \
                   $(RDOC_SRC_DIR)/3rdparty/jpeg-compressor/jpgd.cpp \
                   $(RDOC_SRC_DIR)/3rdparty/jpeg-compressor/jpge.cpp \
                   $(RDOC_SRC_DIR)/3rdparty/lz4/lz4.c \
                   $(RDOC_SRC_DIR)/3rdparty/stb/stb_impl.c \
                   $(RDOC_SRC_DIR)/3rdparty/tinyexr/tinyexr.cpp \
                   $(RDOC_SRC_DIR)/os/posix/android/android_stringio.cpp \
                   $(RDOC_SRC_DIR)/os/posix/android/android_callstack.cpp \
                   $(RDOC_SRC_DIR)/os/posix/android/android_process.cpp \
                   $(RDOC_SRC_DIR)/os/posix/android/android_threading.cpp \
                   $(RDOC_SRC_DIR)/os/posix/posix_hook.cpp \
                   $(RDOC_SRC_DIR)/os/posix/posix_network.cpp \
                   $(RDOC_SRC_DIR)/os/posix/posix_process.cpp \
                   $(RDOC_SRC_DIR)/os/posix/posix_stringio.cpp \
                   $(RDOC_SRC_DIR)/os/posix/posix_threading.cpp \
                   $(RDOC_SRC_DIR)/os/posix/posix_libentry.cpp
# Note: posix_libentry must be the last (above) so that library_loaded is
# called after static objects are constructed.

LOCAL_C_INCLUDES += $(RDOC_SRC_DIR) \
                    $(RDOC_SRC_DIR)/3rdparty \

LOCAL_CFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR \
                -DRENDERDOC_SUPPORT_VULKAN \
                -DRENDERDOC_PLATFORM_POSIX \
                -DRENDERDOC_PLATFORM_ANDROID \
                -DRENDERDOC_EXPORTS \
                -std=c++11 -fstrict-aliasing \
                -fvisibility=hidden -fvisibility-inlines-hidden \
                -Wall \
                -Wextra \
                -Wno-unused-variable \
                -Wno-unused-parameter \
                -Wno-unused-result \
                -Wno-type-limits \
                -Wno-missing-field-initializers \
                -Wno-unknown-pragmas \
                -Wno-reorder
# Do we need the following?: --include=$(SRC_DIR)/common/vulkan_wrapper.h

LOCAL_WHOLE_STATIC_LIBRARIES += android_native_app_glue
LOCAL_LDLIBS    := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
