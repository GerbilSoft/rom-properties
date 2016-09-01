# - Check what flag is needed to activate C++ 2011 mode.
# CHECK_CXX11_COMPILER_FLAG(VARIABLE)
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

INCLUDE(CheckCXXSourceCompiles)

MACRO(CHECK_CXX11_COMPILER_FLAG _RESULT)
	UNSET(${_RESULT})
	
	# MSVC doesn't allow setting the C standard.
	IF(NOT MSVC)
	# Check if C++ 2011 is present without any flags.
	# g++-5.1 uses C++ 1998 by default, but this may change
	# in future versions of gcc.
	MESSAGE(STATUS "Checking if C++ 2011 is enabled by default:")
	CHECK_CXX_SOURCE_COMPILES("
#if !defined(__cplusplus) || __cplusplus < 201103L
#error C++ 2011 is not enabled
#endif

int main() { return 0; }" CHECK_CXX11_ENABLED_DEFAULT)
	IF (${CHECK_CXX11_ENABLED_DEFAULT})
		UNSET(${_RESULT})
		MESSAGE(STATUS "Checking if C++ 2011 is enabled by default: yes")
	ELSE()
		MESSAGE(STATUS "Checking if C++ 2011 is enabled by default: no")
		MESSAGE(STATUS "Checking what CXXFLAG is required for C++ 2011:")
		FOREACH(CHECK_CXX11_CXXFLAG "-std=gnu++11" "-std=gnu++0x" "-std=c++11" "-std=c++0x")
			# CMake doesn't like "+" characters in variable names.
			STRING(REPLACE "+" "_" CHECK_CXX11_CXXFLAG_VARNAME "CHECK_CXXFLAG_${CHECK_CXX11_CXXFLAG}")

			SET(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
			SET(CMAKE_REQUIRED_DEFINITIONS "${CHECK_CXX11_CXXFLAG}")
			CHECK_CXX_SOURCE_COMPILES("int main() { static_assert(0 == 0, \"test assertion\"); return 0; }" ${CHECK_CXX11_CXXFLAG_VARNAME})
			SET(CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")

			IF(${${CHECK_CXX11_CXXFLAG_VARNAME}})
				SET(${_RESULT} ${CHECK_CXX11_CXXFLAG})
				UNSET(${CHECK_CXX11_CXXFLAG_VARNAME})
				UNSET(CHECK_CXX11_CXXFLAG_VARNAME)
				BREAK()
			ENDIF(${${CHECK_CXX11_CXXFLAG_VARNAME}})
			UNSET(${CHECK_CXX11_CXXFLAG_VARNAME})
			UNSET(CHECK_CXX11_CXXFLAG_VARNAME)
		ENDFOREACH()
		IF(${_RESULT})
			MESSAGE(STATUS "Checking what CXXFLAG is required for C++ 2011: ${${_RESULT}}")
		ELSE(${_RESULT})
			MESSAGE(STATUS "Checking what CXXFLAG is required for C++ 2011: none")
		ENDIF(${_RESULT})
	ENDIF()
	UNSET(CHECK_CXX11_ENABLED_DEFAULT)
	ENDIF(NOT MSVC)
ENDMACRO(CHECK_CXX11_COMPILER_FLAG)
