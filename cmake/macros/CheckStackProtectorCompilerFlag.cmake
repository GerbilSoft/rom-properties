# - Check what flag is needed to enable stack smashing protection
# CHECK_STACK_PROTECTOR_COMPILER_FLAG(VARIABLE)
#
#  VARIABLE - variable to store the result
# 
#  This actually calls the check_c_source_compiles macro.
#  See help for CheckCSourceCompiles for a listing of variables
#  that can modify the build.

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
# C++ 2011 version Copyright (c) 2011 by David Korth.
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Based on CHECK_C99_COMPILER_FLAG(). (CheckC99CompilerFlag.cmake)

INCLUDE(CheckCSourceCompiles)

FUNCTION(CHECK_STACK_PROTECTOR_COMPILER_FLAG _result)
	UNSET(${_result} PARENT_SCOPE)

	IF(NOT DEFINED _SYS_STACK_PROTECTOR_COMPILER_FLAG)
	IF(MSVC)
		# MSVC 2002 introduced the /GS option.
		# MSVC 2005+ enables it by default.
		IF(MSVC_VERSION GREATER 1399)
			# MSVC 2005+.
			SET(_SYS_STACK_PROTECTOR_COMPILER_FLAG "" CACHE INTERNAL "CFLAG required for stack smashing protection.")
			MESSAGE(STATUS "Checking what CFLAG is required for stack smashing protection: none, enabled by default")
		ELSEIF(MSVC_VERSION GREATER 1299)
			# MSVC 2002 or 2003.
			SET(_SYS_STACK_PROTECTOR_COMPILER_FLAG "/GS" CACHE INTERNAL "CFLAG required for stack smashing protection.")
			MESSAGE(STATUS "Checking what CFLAG is required for stack smashing protection: ${_SYS_STACK_PROTECTOR_COMPILER_FLAG}")
		ELSE()
			# MSVC 2002 or earlier.
			SET(_SYS_STACK_PROTECTOR_COMPILER_FLAG "" CACHE INTERNAL "CFLAG required for stack smashing protection.")
			MESSAGE(STATUS "Checking what CFLAG is required for stack smashing protection: not available")
		ENDIF()
	ELSE(MSVC)
		# gcc-4.1: -fstack-protector, -fstack-protector-all
		# gcc-4.9: -fstack-protector-strong
		MESSAGE(STATUS "Checking what CFLAG is required for stack smashing protection:")
		FOREACH(CHECK_STACK_CFLAG "-fstack-protector-strong" "-fstack-protector-all" "-fstack-protector")
			# CMake doesn't like "+" characters in variable names.
			STRING(REPLACE "+" "_" CHECK_STACK_CFLAG_VARNAME "CHECK_CFLAG_${CHECK_STACK_CFLAG}")

			SET(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
			SET(SAFE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
			SET(CMAKE_REQUIRED_DEFINITIONS "${CHECK_STACK_CFLAG}")
			IF(WIN32)
				SET(CMAKE_REQUIRED_LIBRARIES "-lkernel32")
			ELSE(WIN32)
				SET(CMAKE_REQUIRED_LIBRARIES "-lc")
			ENDIF(WIN32)
			# NOTE: We need a function that triggers stack smashing protection
			# in order to determine if libssp is needed on MinGW-w64.
			# Based on wrapper functions in c99-compat.msvcrt.h.
			CHECK_C_SOURCE_COMPILES("
#include <stdio.h>
#include <stdarg.h>

#ifndef _MSC_VER
#define _vsnprintf vsnprintf
#endif

int C99_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int ret = _vsnprintf(str, size, format, ap);
	if (ret >= (int)size) {
		// Make sure the buffer is NULL-terminated.
		str[size-1] = 0;
	} else if (ret < 0) {
		// Make sure the buffer is empty.
		// MSVCRT *should* do this, but just in case...
		str[0] = 0;
	}
	return ret;
}

int C99_snprintf(char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = C99_vsnprintf(str, size, format, ap);
	va_end(ap);
	return ret;
}

int main(int argc, char *argv[])
{
	char buf[128];
	snprintf(buf, sizeof(buf), \"test: %s\", argv[0]);
	puts(buf);
	return 0;
}" ${CHECK_STACK_CFLAG_VARNAME})
			SET(CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")
			SET(CMAKE_REQUIRED_LIBRARIES "${SAFE_CMAKE_REQUIRED_LIBRARIES}")

			IF(${${CHECK_STACK_CFLAG_VARNAME}})
				SET(_SYS_STACK_PROTECTOR_COMPILER_FLAG "${CHECK_STACK_CFLAG}" CACHE INTERNAL "CFLAG required for stack smashing protection.")
				BREAK()
			ENDIF(${${CHECK_STACK_CFLAG_VARNAME}})
		ENDFOREACH()
		IF(_SYS_STACK_PROTECTOR_COMPILER_FLAG)
			MESSAGE(STATUS "Checking what CFLAG is required for stack smashing protection: ${_SYS_STACK_PROTECTOR_COMPILER_FLAG}")
		ELSE(_SYS_STACK_PROTECTOR_COMPILER_FLAG)
			SET(_SYS_STACK_PROTECTOR_COMPILER_FLAG "" CACHE INTERNAL "CFLAG required for stack smashing protection.")
			MESSAGE(STATUS "Checking what CFLAG is required for stack smashing protection: not available")
			MESSAGE(WARNING "Stack smashing protection is not available.\nPlease check your toolchain installation.")
		ENDIF(_SYS_STACK_PROTECTOR_COMPILER_FLAG)
	ENDIF(MSVC)
	ENDIF(NOT DEFINED _SYS_STACK_PROTECTOR_COMPILER_FLAG)

	SET(${_result} "${_SYS_STACK_PROTECTOR_COMPILER_FLAG}" PARENT_SCOPE)
ENDFUNCTION(CHECK_STACK_PROTECTOR_COMPILER_FLAG)
