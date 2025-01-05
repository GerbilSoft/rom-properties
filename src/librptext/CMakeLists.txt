# rom-properties base library
PROJECT(rptext LANGUAGES C CXX)

IF(NOT WIN32)
	# Non-Windows library checks.

	# On Win32, we use MultiByteToWideChar() and WideCharToMultiByte().
	# On other systems, we use iconv(), which could be in libc or libiconv.
	# Figure out which library has iconv().

	# Check for iconv().
	FIND_PACKAGE(Iconv REQUIRED)
	SET(HAVE_ICONV 1)
	IF(NOT Iconv_IS_BUILT_IN)
		SET(HAVE_ICONV_LIBICONV 1)
	ENDIF(NOT Iconv_IS_BUILT_IN)
ENDIF(NOT WIN32)

# ZLIB and libpng are checked in the top-level CMakeLists.txt.

# Check wchar_t size.
INCLUDE(CheckTypeSize)
SET(CMAKE_REQUIRED_INCLUDES "wchar.h")
CHECK_TYPE_SIZE(wchar_t SIZEOF_WCHAR_T)
IF(SIZEOF_WCHAR_T EQUAL 2)
	SET(RP_WIS16 1)
ENDIF(SIZEOF_WCHAR_T EQUAL 2)
UNSET(CMAKE_REQUIRED_INCLUDES)

# Check for C library functions.
INCLUDE(CheckSymbolExists)
INCLUDE(CheckStructHasMember)
# MSVCRT doesn't have nl_langinfo() and probably never will.
IF(NOT WIN32)
	CHECK_SYMBOL_EXISTS(nl_langinfo "langinfo.h" HAVE_NL_LANGINFO)
ELSE(NOT WIN32)
	# Win32: MinGW's `struct lconv` does have wchar_t fields,
	# but only if __MSVCRT_VERSION__ >= 0xA00 || _WIN32_WINNT >= 0x601.
	CHECK_STRUCT_HAS_MEMBER("struct lconv" _W_decimal_point "locale.h"
		HAVE_STRUCT_LCONV_WCHAR_T
		LANGUAGE C)
ENDIF(NOT WIN32)

CHECK_SYMBOL_EXISTS(strnlen "string.h" HAVE_STRNLEN)

# memmem() requires _GNU_SOURCE.
SET(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE)
CHECK_SYMBOL_EXISTS(memmem "string.h" HAVE_MEMMEM)
UNSET(CMAKE_REQUIRED_DEFINITIONS)

# Added in glibc-2.38; also the various BSDs and Mac OS X.
CHECK_SYMBOL_EXISTS(strlcat "string.h" HAVE_STRLCAT)

# wcwidth() requires _XOPEN_SOURCE.
SET(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_XOPEN_SOURCE)
CHECK_SYMBOL_EXISTS(wcwidth "wchar.h" HAVE_WCWIDTH)
UNSET(CMAKE_REQUIRED_DEFINITIONS)

# Sources
SET(${PROJECT_NAME}_SRCS
	conversion.cpp
	fourCC.cpp
	libc.c
	printf.cpp
	specialized.cpp
	utf8_strlen.cpp)
# Headers
SET(${PROJECT_NAME}_H
	conversion.hpp
	fourCC.hpp
	libc.h
	NULL-check.hpp
	printf.hpp
	specialized.cpp
	utf8_strlen.hpp
	wchar.hpp
	RP_CP_tbls.hpp)

IF(WIN32)
	SET(${PROJECT_NAME}_OS_SRCS conversion_win32.cpp)
	SET(${PROJECT_NAME}_OS_H    wchar.hpp)
ELSE(WIN32)
	SET(${PROJECT_NAME}_OS_SRCS conversion_iconv.cpp)
ENDIF(WIN32)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

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
		${${PROJECT_NAME}_OS_SRCS} ${${PROJECT_NAME}_OS_H}
		${${PROJECT_NAME}_CRYPTO_SRCS} ${${PROJECT_NAME}_CRYPTO_H}
		${${PROJECT_NAME}_CRYPTO_OS_SRCS} ${${PROJECT_NAME}_CRYPTO_OS_H}
		${${PROJECT_NAME}_SSSE3_SRCS}
		)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds; enable PIC.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		POSITION_INDEPENDENT_CODE TRUE
		)

	# Make sure git_version.h is created before compiling this target.
	IF(TARGET git_version)
		ADD_DEPENDENCIES(${_target} git_version)
	ENDIF(TARGET git_version)

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

	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# librptext
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# librptext
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
		)

	# Required compile definitions for memmem() and wcwidth().
	IF(HAVE_MEMMEM)
		TARGET_COMPILE_DEFINITIONS(${_target} PUBLIC _GNU_SOURCE)
	ENDIF(HAVE_MEMMEM)
	IF(HAVE_WCWIDTH)
		TARGET_COMPILE_DEFINITIONS(${_target} PUBLIC _XOPEN_SOURCE)
	ENDIF(HAVE_WCWIDTH)

	# Other libraries
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpthreads${_target_suffix} rpbyteswap${_target_suffix})

	IF(Iconv_LIBRARY AND NOT Iconv_IS_BUILT_IN)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE Iconv::Iconv)
	ENDIF(Iconv_LIBRARY AND NOT Iconv_IS_BUILT_IN)
	IF(NOT HAVE_WCWIDTH)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE uniwidth)
	ENDIF(NOT HAVE_WCWIDTH)

	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE i18n${_target_suffix})
	ENDIF(ENABLE_NLS)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(TARGET ${PROJECT_NAME} AND NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

# Test suite
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
