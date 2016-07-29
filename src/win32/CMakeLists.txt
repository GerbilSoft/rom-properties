PROJECT(rom-properties-win32)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

# Sources and headers.
SET(rom-properties-win32_SRCS
	DllMain.cpp
	RP_ExtractIcon.cpp
	RP_ComBase.cpp
	)
SET(rom-properties-win32_H
	RP_ExtractIcon.hpp
	RP_ClassFactory.hpp
	RP_ComBase.hpp
	)

#####################
# Build the plugin. #
#####################

# FIXME: Build both Win32 and Win64 DLLs?
ADD_LIBRARY(rom-properties MODULE
		${rom-properties-win32_SRCS}
		${rom-properties-win32_H}
		)
DO_SPLIT_DEBUG(rom-properties)
TARGET_INCLUDE_DIRECTORIES(rom-properties
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_LINK_LIBRARIES(rom-properties
	romdata16
	)

# Use the .def file.
# TODO: MinGW support.
IF(MSVC)
	SET_TARGET_PROPERTIES(rom-properties
		PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def"
		)
ENDIF(MSVC)
