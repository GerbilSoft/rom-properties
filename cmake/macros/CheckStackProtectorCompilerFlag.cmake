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

INCLUDE(CheckCXXSourceCompiles)

MACRO(CHECK_STACK_PROTECTOR_COMPILER_FLAG _RESULT)
	UNSET(${_RESULT})
	
	IF(MSVC)
		# MSVC 2002 introduced the /GS option.
		# MSVC 2005+ enables it by default.
		IF(MSVC_VERSION GREATER 1399)
			# MSVC 2005+.
			MESSAGE(STATUS "Checking what CXXFLAG is required for stack smashing protection: none")
		ELSEIF(MSVC_VERSION GREATER 1299)
			# MSVC 2002 or 2003.
			SET(${_RESULT} "/GS")
			MESSAGE(STATUS "Checking what CXXFLAG is required for stack smashing protection: ${${_RESULT}}")
		ELSE()
			# MSVC 2002 or earlier.
			MESSAGE(STATUS "Checking what CXXFLAG is required for stack smashing protection: not available")
		ENDIF()
	ELSE(MSVC)
		# gcc-4.1: -fstack-protector, -fstack-protector-all
		# gcc-4.9: -fstack-protector-strong
		MESSAGE(STATUS "Checking what CXXFLAG is required for stack smashing protection:")
		FOREACH(CHECK_STACK_CXXFLAG "-fstack-protector-strong" "-fstack-protector-all" "-fstack-protector")
			# CMake doesn't like "+" characters in variable names.
			STRING(REPLACE "+" "_" CHECK_STACK_CXXFLAG_VARNAME "CHECK_CXXFLAG_${CHECK_STACK_CXXFLAG}")

			SET(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
			SET(CMAKE_REQUIRED_DEFINITIONS "${CHECK_STACK_CXXFLAG}")
			CHECK_CXX_SOURCE_COMPILES("int main() { return 0; }" ${CHECK_STACK_CXXFLAG_VARNAME})
			SET(CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")

			IF(${${CHECK_STACK_CXXFLAG_VARNAME}})
				SET(${_RESULT} ${CHECK_STACK_CXXFLAG})
				UNSET(${CHECK_STACK_CXXFLAG_VARNAME})
				UNSET(CHECK_STACK_CXXFLAG_VARNAME)
				BREAK()
			ENDIF(${${CHECK_STACK_CXXFLAG_VARNAME}})
			UNSET(${CHECK_STACK_CXXFLAG_VARNAME})
			UNSET(CHECK_STACK_CXXFLAG_VARNAME)
		ENDFOREACH()
		IF(${_RESULT})
			MESSAGE(STATUS "Checking what CXXFLAG is required for stack smashing protection: ${${_RESULT}}")
		ELSE(${_RESULT})
			MESSAGE(STATUS "Checking what CXXFLAG is required for stack smashing protection: not available")
		ENDIF(${_RESULT})
	ENDIF(MSVC)
ENDMACRO(CHECK_STACK_PROTECTOR_COMPILER_FLAG)
