# Copyright (c) 2017-2025 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV REQUIRED opencv4)
find_package(Vulkan REQUIRED)

set(LOCAL_HEADERS
    vr_camera_app.h
    camera/camera_capture.h
    utils/timer.h
)

set(LOCAL_SOURCE
    main.cpp
    vr_camera_app.cpp
    camera/camera_capture.cpp
    utils/timer.cpp
)

# Copy graphics plugin from hello_xr for Vulkan support
set(GRAPHICS_PLUGIN_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/../hello_xr/graphicsplugin_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hello_xr/graphicsplugin_factory.cpp
)

set(SHARED_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../hello_xr/graphicsapi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../hello_xr/graphicsplugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../hello_xr/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../hello_xr/check.h
)

# Simple camera test executable
add_executable(test_camera
    test_camera.cpp
    ${LOCAL_SOURCE}
    ${LOCAL_HEADERS}
)

# Include directories for camera test
target_include_directories(test_camera PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

# Link libraries for camera test
target_link_libraries(test_camera
    ${OpenCV_LIBS}
)

# Set C++ standard for camera test
set_target_properties(test_camera PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# VR Camera Stream executable (commented out for now)
# add_executable(vr_camera_stream
#     ${LOCAL_SOURCE}
#     ${LOCAL_HEADERS}
#     ${GRAPHICS_PLUGIN_SOURCE}
#     ${SHARED_HEADERS}
# )

# Include directories for camera test
target_include_directories(test_camera PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

# Link libraries for camera test
target_link_libraries(test_camera
    ${OpenCV_LIBS}
)

# Set C++ standard for camera test
set_target_properties(test_camera PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# VR Camera Stream executable
add_executable(vr_camera_stream
    ${LOCAL_SOURCE}
    ${LOCAL_HEADERS}
)

# Include directories
target_include_directories(vr_camera_stream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENCV_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(vr_camera_stream
    openxr_loader
    ${OPENCV_LIBRARIES}
    ${Vulkan_LIBRARIES}
)

# Compiler definitions
target_compile_definitions(vr_camera_stream PRIVATE
    XR_USE_GRAPHICS_API_VULKAN
    XR_USE_PLATFORM_XLIB
)

# Set C++ standard
set_target_properties(vr_camera_stream PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Platform-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(vr_camera_stream X11)
endif()

if(MSVC)
    target_compile_definitions(vr_camera_stream PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(vr_camera_stream PRIVATE /Zc:wchar_t /Zc:forScope /W4 /WX)
endif()