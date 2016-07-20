# - Check what flag is needed to disable C++ RTTI.
# CHECK_CXX_NO_RTTI_COMPILER_FLAG(VARIABLE)
#
#  VARIABLE - variable to store the result
# 
#  This actually calls the check_cxx_source_compiles macro.
#  See help for CheckCXXSourceCompiles for a listing of variables
#  that can modify the build.

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Based on CHECK_CXX_COMPILER_FLAG(). (CheckCXXCompilerFlag.cmake)

# TODO: Add more CXXFLAGS!

INCLUDE(CheckCXXSourceCompiles)

MACRO(CHECK_CXX_NO_RTTI_COMPILER_FLAG _RESULT)
	UNSET(${_RESULT})
	
	MESSAGE(STATUS "Checking what CXXFLAG is required to disable C++ RTTI:")
	IF(MSVC)
		SET(${_RESULT} "-GR-")
	ELSE(MSVC)
	FOREACH(CHECK_CXX_NO_RTTI_CFLAG "-fno-rtti")
		SET(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
		SET(CMAKE_REQUIRED_DEFINITIONS "${CHECK_CXX_NO_RTTI_CFLAG}")
		CHECK_CXX_SOURCE_COMPILES("int main() { return 0; }" CFLAG_${CHECK_CXX_NO_RTTI_CFLAG})
		SET(CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")
		IF(CFLAG_${CHECK_CXX_NO_RTTI_CFLAG})
			SET(${_RESULT} ${CHECK_CXX_NO_RTTI_CFLAG})
			BREAK()
		ENDIF(CFLAG_${CHECK_CXX_NO_RTTI_CFLAG})
		UNSET(CFLAG_${CHECK_CXX_NO_RTTI_CFLAG})
	ENDFOREACH()
	ENDIF(MSVC)
	
	IF(${_RESULT})
		MESSAGE(STATUS "Checking what CXXFLAG is required to disable C++ RTTI: ${${_RESULT}}")
	ELSE(${_RESULT})
		MESSAGE(STATUS "Checking what CXXFLAG is required to disable C++ RTTI: none")
	ENDIF(${_RESULT})
ENDMACRO (CHECK_CXX_NO_RTTI_COMPILER_FLAG)
