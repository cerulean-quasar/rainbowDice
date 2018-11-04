# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set (platform64_files)
if ((NOT ${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86) AND (NOT ${CMAKE_ANDROID_ARCH_ABI} STREQUAL armeabi-v7a))
    list(APPEND platform64_files src/main/cpp/graphicsVulkan.cpp src/main/cpp/rainbowDiceVulkan.cpp src/main/cpp/vulkanWrapper.cpp)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCQ_ENABLE_VULKAN")
endif ((NOT ${CMAKE_ANDROID_ARCH_ABI} STREQUAL x86) AND (NOT ${CMAKE_ANDROID_ARCH_ABI} STREQUAL armeabi-v7a))

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp
             ${platform64_files}
             src/main/cpp/android.cpp
             src/main/cpp/rainbowDiceGL.cpp
             src/main/cpp/rainbowDiceGlobal.cpp
             src/main/cpp/random.cpp
             src/main/cpp/dice.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

include_directories(${ANDROID_NDK}/sysroot/usr/include/android)
include_directories(${ANDROID_NDK}/sources/third_party/vulkan/src/libs)
include_directories(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -std=c++14")

target_compile_definitions(native-lib PUBLIC -DVK_USE_PLATFORM_ANDROID_KHR)


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       EGL
                       GLESv2
                       android)
