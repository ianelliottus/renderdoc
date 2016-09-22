# FIXME/TODO: May need a module called VkLayer_RenderDoc, in order to get
# libVkLayer_RenderDoc.so

LOCAL_PATH := $(abspath $(call my-dir))
SRC_DIR := $(LOCAL_PATH)/../../

include $(CLEAR_VARS)
LOCAL_MODULE := renderdoc
LOCAL_SRC_FILES += $(SRC_DIR)/common/common.cpp \
                   $(SRC_DIR)/common/dds_readwrite.cpp \
                   $(SRC_DIR)/core/core.cpp \
                   $(SRC_DIR)/core/image_viewer.cpp \
                   $(SRC_DIR)/core/target_control.cpp \
                   $(SRC_DIR)/core/remote_server.cpp \
                   $(SRC_DIR)/core/replay_proxy.cpp \
                   $(SRC_DIR)/core/resource_manager.cpp \
                   $(SRC_DIR)/data/glsl_shaders.cpp \
                   $(SRC_DIR)/hooks/hooks.cpp \
                   $(SRC_DIR)/maths/camera.cpp \
                   $(SRC_DIR)/maths/matrix.cpp \
                   $(SRC_DIR)/os/os_specific.cpp \
                   $(SRC_DIR)/replay/app_api.cpp \
                   $(SRC_DIR)/replay/capture_options.cpp \
                   $(SRC_DIR)/replay/entry_points.cpp \
                   $(SRC_DIR)/replay/replay_output.cpp \
                   $(SRC_DIR)/replay/replay_renderer.cpp \
                   $(SRC_DIR)/replay/type_helpers.cpp \
                   $(SRC_DIR)/serialise/grisu2.cpp \
                   $(SRC_DIR)/serialise/serialiser.cpp \
                   $(SRC_DIR)/serialise/string_utils.cpp \
                   $(SRC_DIR)/serialise/utf8printf.cpp \
                   $(SRC_DIR)/3rdparty/jpeg-compressor/jpgd.cpp \
                   $(SRC_DIR)/3rdparty/jpeg-compressor/jpge.cpp \
                   $(SRC_DIR)/3rdparty/lz4/lz4.c \
                   $(SRC_DIR)/3rdparty/stb/stb_impl.c \
                   $(SRC_DIR)/3rdparty/tinyexr/tinyexr.cpp \
                   $(SRC_DIR)/os/posix/android/android_stringio.cpp \
                   $(SRC_DIR)/os/posix/android/android_callstack.cpp \
                   $(SRC_DIR)/os/posix/android/android_process.cpp \
                   $(SRC_DIR)/os/posix/android/android_threading.cpp \
                   $(SRC_DIR)/os/posix/posix_hook.cpp \
                   $(SRC_DIR)/os/posix/posix_network.cpp \
                   $(SRC_DIR)/os/posix/posix_process.cpp \
                   $(SRC_DIR)/os/posix/posix_stringio.cpp \
                   $(SRC_DIR)/os/posix/posix_threading.cpp \
                   $(SRC_DIR)/os/posix/posix_libentry.cpp
# Note: posix_libentry must be the last (above) so that library_loaded is
# called after static objects are constructed.


