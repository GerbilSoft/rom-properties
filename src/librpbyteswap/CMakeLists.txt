# rom-properties byteswapping library
PROJECT(rpbyteswap LANGUAGES C)

# Check for C headers.
CHECK_INCLUDE_FILES("features.h" HAVE_FEATURES_H)

# Sources
SET(${PROJECT_NAME}_SRCS
	byteswap.c
	bitstuff.c
	)
# Headers
SET(${PROJECT_NAME}_H
	byteorder.h
	byteswap_rp.h
	bitstuff.h
	)

# CPU-specific and optimized sources
INCLUDE(CPUInstructionSetFlags)
IF(CPU_i386 OR CPU_amd64)
	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://docs.microsoft.com/en-us/cpp/cpp/m64?view=msvc-160
	#
	# amd64 CPUs support SSE2 as a minimum anyway, so there's
	# no point in building MMX code for 64-bit.
	IF(CPU_i386)
		SET(${PROJECT_NAME}_MMX_SRCS byteswap_mmx.c)
	ENDIF(CPU_i386)

	SET(${PROJECT_NAME}_SSE2_SRCS byteswap_sse2.c)
	SET(${PROJECT_NAME}_SSSE3_SRCS byteswap_ssse3.c)

	IF(MMX_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_MMX_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
	ENDIF(MMX_FLAG)

	IF(SSE2_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SSE2_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
	ENDIF(SSE2_FLAG)

	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)

	SET(${PROJECT_NAME}_CPU_SRCS ${${PROJECT_NAME}_MMX_SRCS} ${${PROJECT_NAME}_SSE2_SRCS} ${${PROJECT_NAME}_SSSE3_SRCS})
ELSEIF((CPU_arm OR CPU_arm64) AND HAVE_ARM_NEON_H)
	SET(${PROJECT_NAME}_NEON_SRCS byteswap_neon.c)

	IF(NEON_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_NEON_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${NEON_FLAG} ")
	ENDIF(NEON_FLAG)

	SET(${PROJECT_NAME}_CPU_SRCS ${${PROJECT_NAME}_NEON_SRCS})
ENDIF()
UNSET(arch)

# gcc byteswapping intrinsics.
IF(NOT MSVC)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap16((unsigned short)argc); }" HAVE___BUILTIN_BSWAP16)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap32((unsigned int)argc); }" HAVE___BUILTIN_BSWAP32)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap64((unsigned long long)argc); }" HAVE___BUILTIN_BSWAP64)
ENDIF(NOT MSVC)

# Write the config.h files
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.byteswap.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.byteswap.h")

######################
# Build the library. #
######################

# NOTE: ${PROJECT_NAME} is the original target.
# ${PROJECT_NAME}-dll is the target with dllimport/dllexport for libromdata.
IF(RP_LIBROMDATA_IS_DLL AND WIN32)
	# On Win32, the dll target needs dllimport/dllexport.
	SET(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-dll)
ELSE()
	# On other platforms, the dll target will be an alias to the regular target.
	# That will be set up later, since the regular target doesn't exist yet.
	SET(TARGETS ${PROJECT_NAME})
ENDIF()

FOREACH(_target ${TARGETS})
	ADD_LIBRARY(${_target} STATIC
		${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
		${${PROJECT_NAME}_CPU_SRCS}
		)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds; enable PIC.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		POSITION_INDEPENDENT_CODE TRUE
		)

	# DLL targets need the appropriate macros.
	IF(${_target} MATCHES "-dll$")
		SET(_target_suffix "-dll")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ELSE()
		UNSET(_target_suffix)
	ENDIF()

	# rpcpuid is needed for CPU identification.
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpcpuid)

	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpbyteswap
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpbyteswap
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
		)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
