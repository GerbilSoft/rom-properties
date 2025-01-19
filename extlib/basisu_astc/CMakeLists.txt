# Basis Universal ASTC decoder
PROJECT(basisu_astc LANGUAGES CXX)

# rom-properties: Hide symbols by default, since we don't want them
# leaking from the static library to the plugins.
CMAKE_POLICY(SET CMP0063 NEW)
SET(CMAKE_CXX_VISIBILITY_PRESET "hidden")
SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

ADD_LIBRARY(${PROJECT_NAME} STATIC
	basisu_astc_decomp.cpp
	basisu_astc_decomp.h
	)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	)
