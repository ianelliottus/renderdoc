APP_ABI := armeabi-v7a arm64-v8a x86 x86_64 mips mips64
APP_PLATFORM := android-23
APP_STL := gnustl_static
APP_MODULES := renderdoc renderdoccmd
APP_CPPFLAGS += -std=c++11 -fexceptions -Wall -Werror -Wextra -Wno-unused-parameter -DVK_NO_PROTOTYES -DGLM_FORCE_RADIANS
APP_CFLAGS += -Wall -Werror -Wextra -Wno-unused-parameter -DVK_NO_PROTOTYES -DGLM_FORCE_RADIANS
NDK_TOOLCHAIN_VERSION := clang
