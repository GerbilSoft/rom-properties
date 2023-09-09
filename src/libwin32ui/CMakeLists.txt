# Windows UI common code
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(win32ui LANGUAGES C CXX)

# Check for Windows SDK headers.
CHECK_INCLUDE_FILES("shellscalingapi.h" HAVE_SHELLSCALINGAPI_H)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

# Sources and headers.
SET(${PROJECT_NAME}_SRCS
	DialogBuilder.cpp
	HiDPI.c
	LoadResource_i18n.cpp
	RegKey.cpp
	WinUI.cpp
	)
SET(${PROJECT_NAME}_H
	AutoGetDC.hpp
	DialogBuilder.hpp
	HiDPI.h
	LoadResource_i18n.hpp
	RegKey.hpp
	WinUI.hpp
	WTSSessionNotification.hpp
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

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpthreads)
# for DefSubclassProc and related
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC comctl32)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libwin32ui
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libwin32ui
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
	# Can't link directly to libwin32common due to DLL shenanigans.
	# We only need its headers, though.
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../libwin32common>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../libwin32common>
	)
