# rom-properties CPU support library
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(librpcpu LANGUAGES C)

# Check for C headers.
CHECK_INCLUDE_FILES("features.h" HAVE_FEATURES_H)

# Sources.
SET(librpcpu_SRCS
	byteswap.c
	)
# Headers.
SET(librpcpu_H
	byteorder.h
	byteswap.h
	bitstuff.h
	)

# CPU-specific and optimized sources.
IF(CPU_i386 OR CPU_amd64)
	SET(librpcpu_CPU_SRCS cpuflags_x86.c)
	SET(librpcpu_CPU_H cpuflags_x86.h)

	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx
	#
	# amd64 CPUs support SSE2 as a minimum anyway, so there's
	# no point in building MMX code for 64-bit.
	IF(CPU_i386)
		SET(librpcpu_MMX_SRCS byteswap_mmx.c)
	ENDIF(CPU_i386)

	SET(librpcpu_SSE2_SRCS byteswap_sse2.c)
	SET(librpcpu_SSSE3_SRCS byteswap_ssse3.c)

	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(librpcpu_IFUNC_SRCS byteswap_ifunc.c)
		# Disable LTO on the IFUNC files if LTO is known to be broken.
		IF(GCC_5xx_LTO_ISSUES)
			SET_SOURCE_FILES_PROPERTIES(${librpcpu_IFUNC_SRCS}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
		ENDIF(GCC_5xx_LTO_ISSUES)
	ENDIF(UNIX AND NOT APPLE)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSE2_FLAG "/arch:SSE2")
		SET(SSSE3_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		SET(MMX_FLAG "-mmmx")
		SET(SSE2_FLAG "-msse2")
		SET(SSSE3_FLAG "-mssse3")
	ENDIF()

	IF(MMX_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librpcpu_MMX_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
	ENDIF(MMX_FLAG)

	IF(SSE2_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librpcpu_SSE2_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
	ENDIF(SSE2_FLAG)

	IF(SSSE3_FLAG)
		SET_SOURCE_FILES_PROPERTIES(${librpcpu_SSSE3_SRCS}
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSSE3_FLAG} ")
	ENDIF(SSSE3_FLAG)
ENDIF()
UNSET(arch)

# gcc byteswapping intrinsics.
IF(NOT MSVC)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap16((unsigned short)argc); }" HAVE___BUILTIN_BSWAP16)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap32((unsigned int)argc); }" HAVE___BUILTIN_BSWAP32)
	CHECK_C_SOURCE_COMPILES("int main(int argc, char *argv[]) { return __builtin_bswap64((unsigned long long)argc); }" HAVE___BUILTIN_BSWAP64)
ENDIF(NOT MSVC)

# Write the config.h files.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.byteswap.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.byteswap.h")

######################
# Build the library. #
######################

ADD_LIBRARY(rpcpu STATIC
	${librpcpu_SRCS} ${librpcpu_H}
	${librpcpu_OS_SRCS} ${librpcpu_OS_H}
	${librpcpu_CPU_SRCS} ${librpcpu_CPU_H}
	${librpcpu_IFUNC_SRCS}
	${librpcpu_MMX_SRCS}
	${librpcpu_SSE2_SRCS}
	${librpcpu_SSSE3_SRCS}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(rpcpu)

# rpthreads is needed for atomic functions.
TARGET_LINK_LIBRARIES(rpcpu PRIVATE rpthreads)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rpcpu
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librpcpu
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librpcpu
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(rpcpu PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
