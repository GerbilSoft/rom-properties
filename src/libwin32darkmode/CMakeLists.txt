# Win32 dark mode
# https://github.com/ysc3839/win32-darkmode
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(win32darkmode LANGUAGES CXX)

# Sources and headers.
SET(${PROJECT_NAME}_SRCS
	DarkMode.cpp
	DarkModeCtrl.cpp
	ListViewUtil.cpp
	NppDarkMode.cpp
	TGDarkMode.cpp
	)
SET(${PROJECT_NAME}_H
	DarkMode.hpp
	DarkModeCtrl.hpp
	IatHook.hpp
	ListViewUtil.hpp
	NppDarkMode.hpp
	TGDarkMode.hpp
	)

######################
# Build the library. #
######################

ADD_LIBRARY(${PROJECT_NAME} STATIC
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(${PROJECT_NAME})
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE win32ui)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE gdiplus comctl32 uxtheme)

IF(RP_LIBROMDATA_IS_DLL AND MSVC)
	# Delay-load gdiplus.dll.
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG	"${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DELAYLOAD:gdiplus.dll")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE	"${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DELAYLOAD:gdiplus.dll")
ENDIF(RP_LIBROMDATA_IS_DLL AND MSVC)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/..	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/..	# src
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libwin32darkmode
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libwin32darkmode
	)
