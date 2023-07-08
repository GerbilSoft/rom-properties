# Security functionality
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rpsecure LANGUAGES C)

OPTION(ENABLE_EXTRA_SECURITY "Enable extra security functionality if available." ON)
IF(CMAKE_C_FLAGS MATCHES -fsanitize OR CMAKE_CXX_FLAGS MATCHES -fsanitize)
	# Disable extra security if a sanitizer is in use.
	SET(ENABLE_EXTRA_SECURITY OFF CACHE BOOL "Enable extra security functionality if available." FORCE)
ENDIF()

IF(ENABLE_EXTRA_SECURITY)
	IF(WIN32)
		SET(${PROJECT_NAME}_SRCS
			win32/integrity_level.c
			win32/secoptions.c
			)
		SET(${PROJECT_NAME}_H
			os-secure.h
			win32/integrity_level.h
			win32/secoptions.h
			win32/secoptions_win8.h
			)
		SET(${PROJECT_NAME}_OS_SRCS os-secure_win32.c)
		SET(SECURITY_MECHANISM_int "Win32 API")
	ELSEIF(UNIX AND NOT APPLE)
		# Check for system security functionality.
		IF(CMAKE_SYSTEM MATCHES "Linux")
			# Linux: Use seccomp().
			FIND_PACKAGE(SECCOMP REQUIRED)
			IF(SECCOMP_FOUND)
				SET(${PROJECT_NAME}_OS_SRCS os-secure_linux.c seccomp-debug.c)
				SET(${PROJECT_NAME}_OS_H seccomp-debug.h)
				SET(HAVE_SECCOMP 1)
				SET(SECURITY_MECHANISM_int "seccomp()")
			ENDIF(SECCOMP_FOUND)
		ELSE()
			# OpenBSD: Use pledge()/tame().
			INCLUDE(CheckOpenBSDPledge)
			CHECK_OPENBSD_PLEDGE()
			IF(HAVE_PLEDGE)
				SET(${PROJECT_NAME}_OS_SRCS os-secure_openbsd.c)
				SET(SECURITY_MECHANISM_int "pledge()")
			ELSEIF(HAVE_TAME)
				SET(${PROJECT_NAME}_OS_SRCS os-secure_openbsd.c)
				SET(SECURITY_MECHANISM_int "tame()")
			ENDIF()
		ENDIF()
	ENDIF()
ENDIF(ENABLE_EXTRA_SECURITY)
SET(SECURITY_MECHANISM "${SECURITY_MECHANISM_int}" CACHE INTERNAL "Security mechanism" FORCE)

IF(NOT ${PROJECT_NAME}_OS_SRCS)
	# TODO: Add support for other systems.
	SET(ENABLE_EXTRA_SECURITY OFF CACHE INTERNAL "Enable extra security functionality if available." FORCE)
	SET(${PROJECT_NAME}_OS_SRCS os-secure_dummy.c)
ENDIF(NOT ${PROJECT_NAME}_OS_SRCS)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

######################
# Build the library. #
######################

ADD_LIBRARY(${PROJECT_NAME} STATIC
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_OS_SRCS}
	${${PROJECT_NAME}_OS_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(${PROJECT_NAME})
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
IF(ENABLE_EXTRA_SECURITY)
	IF(CMAKE_SYSTEM MATCHES "Linux" AND SECCOMP_FOUND)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Seccomp::seccomp)
	ELSEIF(WIN32)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE advapi32)
	ENDIF()
ENDIF(ENABLE_EXTRA_SECURITY)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpsecure
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpsecure
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
