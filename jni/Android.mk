LOCAL_PATH := $(abspath $(call my-dir))
SRC_DIR := $(LOCAL_PATH)/../app/src/main/cpp/

include $(CLEAR_VARS)
LOCAL_MODULE := rainbowDice
LOCAL_SRC_FILES += $(SRC_DIR)/rainbowDice.cpp
LOCAL_C_INCLUDES += $(ANDROID_DIR)/external/shaderc/libshaderc/include

#LOCAL_STATIC_LIBRARIES := googletest_main layer_utils
#LOCAL_SHARED_LIBRARIES += shaderc-prebuilt glslang-prebuilt OGLCompiler-prebuilt OSDependent-prebuilt HLSL-prebuilt shaderc_util-prebuilt SPIRV-prebuilt SPIRV-Tools-prebuilt SPIRV-Tools-opt-prebuilt
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR -fvisibility=hidden -DVALIDATION_APK -I$(NDK)/sources/third_party/vulkan/src/libs/glm
#LOCAL_WHOLE_STATIC_LIBRARIES += android_native_app_glue
LOCAL_LDLIBS := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
