PROJECT(rom-properties-win32)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

# Sources and headers.
SET(rom-properties-win32_SRCS
	DllMain.cpp
	RegKey.cpp
	RP_ExtractIcon.cpp
	RP_ComBase.cpp
	RP_ExtractImage.cpp
	RpImageWin32.cpp
	RP_ShellPropSheetExt.cpp
	)
SET(rom-properties-win32_H
	RegKey.hpp
	RP_ExtractIcon.hpp
	RP_ClassFactory.hpp
	RP_ComBase.hpp
	RP_ExtractImage.hpp
	RpImageWin32.hpp
	RP_ShellPropSheetExt.hpp
	)

#####################
# Build the plugin. #
#####################

# FIXME: Build both Win32 and Win64 DLLs?
ADD_LIBRARY(rom-properties MODULE
		${rom-properties-win32_SRCS}
		${rom-properties-win32_H}
		)
SET_TARGET_PROPERTIES(rom-properties PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rom-properties)
TARGET_INCLUDE_DIRECTORIES(rom-properties
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_LINK_LIBRARIES(rom-properties romdata16 cachemgr16 gdiplus)

# Use the .def file for DLL exports.
IF(MSVC)
	SET_TARGET_PROPERTIES(rom-properties
		PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def"
		)
ELSEIF(MINGW)
	# NOTE: Enabling stdcall fixups so we can use
	# the same .def file for Win32 and Win64,
	# since Win64 doesn't use stdcall.
	SET_TARGET_PROPERTIES(rom-properties
		PROPERTIES LINK_FLAGS "-Wl,-enable-stdcall-fixup ${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def"
		)
ENDIF()
