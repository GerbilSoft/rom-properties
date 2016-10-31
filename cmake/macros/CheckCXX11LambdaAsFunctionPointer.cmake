# - Check if non-capturing lambda functions can be cast to function pointers.
# CHECK_CXX11_LAMBDA_AS_FUNCTION_POINTER(VARIABLE)
#
#  VARIABLE - variable to store the result
#
#  MSVC 2010 supports lambda functions, but does not support casting
#  them to function pointers.
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

INCLUDE(CheckCXXSourceCompiles)

MACRO(CHECK_CXX11_LAMBDA_AS_FUNCTION_POINTER _result)
	UNSET(${_result})
	
	MESSAGE(STATUS "Checking if C++ 2011 lambda functions can be cast to function pointers:")
	CHECK_CXX_SOURCE_COMPILES("
typedef void (*pfntest)(void);

int main() {
	pfntest test = []() { /* do nothing */ };
}" CHECK_CXX11_LAMBDA_RESULT)
	IF (${CHECK_CXX11_LAMBDA_RESULT})
		SET(${_result} TRUE)
		MESSAGE(STATUS "Checking if C++ 2011 lambda functions can be cast to function pointers: yes")
	ELSE()
		UNSET(${_result})
		MESSAGE(STATUS "Checking if C++ 2011 lambda functions can be cast to function pointers: no")
	ENDIF()
	UNSET(CHECK_CXX11_LAMBDA_RESULT)
ENDMACRO(CHECK_CXX11_LAMBDA_AS_FUNCTION_POINTER)
