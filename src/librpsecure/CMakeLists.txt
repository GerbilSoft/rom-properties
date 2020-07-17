# Security functionality
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(librpsecure LANGUAGES C)

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	OPTION(USE_SECCOMP "Use libseccomp filters in rp-download and rpcli." ON)
	OPTION(ENABLE_SECCOMP_DEBUG "Enable libseccomp debugging." OFF)
ELSE()
	SET(USE_SECCOMP OFF CACHE INTERNAL "Use libseccomp filters in rp-download and rpcli." FORCE)
	SET(ENABLE_SECCOMP_DEBUG OFF CACHE INTERNAL "Enable libseccomp debugging." FORCE)
ENDIF()

IF(WIN32)
	SET(librpsecure_SRCS
		win32/integrity_level.c
		win32/secoptions.c
		)
	SET(librpsecure_H
		os-secure.h
		win32/integrity_level.h
		win32/secoptions.h
		win32/secoptions_win8.h
		)
	SET(librpsecure_OS_SRCS os-secure_win32.c)
ELSEIF(UNIX AND NOT APPLE)
	# Check for system security functionality.
	IF(USE_SECCOMP)
		# Linux: Use seccomp().
		FIND_PACKAGE(SECCOMP REQUIRED)
		IF(SECCOMP_FOUND)
			SET(librpsecure_OS_SRCS os-secure_linux.c)
			IF(ENABLE_SECCOMP_DEBUG)
				SET(librpsecure_OS_SRCS ${librpsecure_OS_SRCS} seccomp-debug.c)
				SET(librpsecure_OS_H ${librpsecure_OS_H} seccomp-debug.h)
			ENDIF(ENABLE_SECCOMP_DEBUG)
			SET(HAVE_SECCOMP 1)
		ENDIF(SECCOMP_FOUND)
	ELSE()
		# OpenBSD: Use pledge()/tame().
		INCLUDE(CheckOpenBSDPledge)
		CHECK_OPENBSD_PLEDGE()
		IF(HAVE_PLEDGE OR HAVE_TAME)
			SET(librpsecure_OS_SRCS os-secure_openbsd.c)
		ENDIF()
	ENDIF()
ENDIF()

IF(NOT librpsecure_OS_SRCS)
	# TODO: Add support for other systems.
	SET(librpsecure_OS_SRCS os-secure_dummy.c)
ENDIF(NOT librpsecure_OS_SRCS)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.librpsecure.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.librpsecure.h")

######################
# Build the library. #
######################

ADD_LIBRARY(rpsecure STATIC
	${librpsecure_SRCS}
	${librpsecure_H}
	${librpsecure_OS_SRCS}
	${librpsecure_OS_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(rpsecure)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(rpsecure PROPERTIES EXCLUDE_FROM_ALL TRUE)
IF(USE_SECCOMP AND SECCOMP_FOUND)
	TARGET_LINK_LIBRARIES(rpsecure PUBLIC Seccomp::seccomp)
ENDIF(USE_SECCOMP AND SECCOMP_FOUND)

TARGET_INCLUDE_DIRECTORIES(rpsecure
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpsecure
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpsecure
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
