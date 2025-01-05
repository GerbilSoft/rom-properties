# PowerVR Texture Compression decompressor from the PowerVR Native SDK.
# Copyright (c) Imagination Technologies Ltd.
# Licensed under the MIT License.
# References:
# - PowerVR commit: c1605c99281797e5cd4c8439e1bc679706bbb311
# - https://github.com/powervr-graphics/Native_SDK
PROJECT(PowerVR LANGUAGES CXX)

# rom-properties: Hide symbols by default, since we don't want them
# leaking from the static library to the plugins.
INCLUDE(CheckHiddenVisibility)
CHECK_HIDDEN_VISIBILITY()

# Sources.
SET(libpvrtc_SRCS PVRTDecompress.cpp)
# Headers.
SET(libpvrtc_H PVRTDecompress.h)

######################
# Build the library. #
######################

ADD_LIBRARY(pvrtc STATIC ${libpvrtc_SRCS} ${libpvrtc_H})
TARGET_INCLUDE_DIRECTORIES(pvrtc
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	)
