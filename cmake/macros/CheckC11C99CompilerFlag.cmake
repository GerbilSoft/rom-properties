# - Check what flag is needed to activate C11 or C99 mode.
# CHECK_C11_C99_COMPILER_FLAG(VARIABLE)
#
#  VARIABLE - variable to store the result
#
#  This actually calls the check_c_source_compiles macro.
#  See help for CheckCSourceCompiles for a listing of variables
#  that can modify the build.

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Based on CHECK_C_COMPILER_FLAG(). (CheckCCompilerFlag.cmake)

INCLUDE(CheckCSourceCompiles)

FUNCTION(CHECK_C11_C99_COMPILER_FLAG _result)
	# Flag listing borrowed from GNU autoconf's AC_PROG_CC_C99 macro.
	UNSET(${_result} PARENT_SCOPE)

	# MSVC doesn't allow setting the C standard.
	IF(NOT DEFINED _SYS_C11_C99_CFLAG AND NOT MSVC)
		# Check if C11 is present without any flags.
		# gcc-5.1 uses C11 mode by default.
		MESSAGE(STATUS "Checking if C11 is enabled by default:")
		CHECK_C_SOURCE_COMPILES("
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
# error C11 is not enabled
#endif

int main() { return 0; }" CHECK_C11_ENABLED_DEFAULT)
		IF (CHECK_C11_ENABLED_DEFAULT)
			SET(_SYS_C11_C99_CFLAG "" CACHE INTERNAL "CFLAG required for C11 or C99 mode.")
			MESSAGE(STATUS "Checking if C11 is enabled by default: yes")
		ELSE(CHECK_C11_ENABLED_DEFAULT)
			MESSAGE(STATUS "Checking if C11 is enabled by default: no")
			MESSAGE(STATUS "Checking what CFLAG is required for C11:")
			FOREACH(CHECK_C11_CFLAG "-std=gnu11" "-std=c11" "-c99" "-AC99" "-xc99=all" "-qlanglvl=extc1x" "-qlanglvl=stdc11")
				SET(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
				SET(CMAKE_REQUIRED_DEFINITIONS "${CHECK_C11_CFLAG}")
				CHECK_C_SOURCE_COMPILES("int main() { return 0; }" CFLAG_${CHECK_C11_CFLAG})
				SET(CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")
				IF(CFLAG_${CHECK_C11_CFLAG})
					SET(_SYS_C11_C99_CFLAG "${CHECK_C11_CFLAG}" CACHE INTERNAL "CFLAG required for C11 or C99 mode.")
					BREAK()
				ENDIF(CFLAG_${CHECK_C11_CFLAG})
			ENDFOREACH()

			IF(_SYS_C11_C99_CFLAG)
				MESSAGE(STATUS "Checking what CFLAG is required for C11: ${_SYS_C11_C99_CFLAG}")
			ELSE(_SYS_C11_C99_CFLAG)
				MESSAGE(STATUS "Checking what CFLAG is required for C11: unavailable")
			ENDIF(_SYS_C11_C99_CFLAG)
		ENDIF(CHECK_C11_ENABLED_DEFAULT)

		IF(NOT CHECK_C11_ENABLED_DEFAULT AND NOT _SYS_C11_C99_CFLAG)
			# Could not enable C11. Try C99 instead.
			MESSAGE(STATUS "Checking if C99 is enabled by default:")
			CHECK_C_SOURCE_COMPILES("
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
# error C99 is not enabled
#endif

int main() { return 0; }" CHECK_C99_ENABLED_DEFAULT)
			IF (CHECK_C99_ENABLED_DEFAULT)
				SET(_SYS_C11_C99_CFLAG "" CACHE INTERNAL "CFLAG required for C11 or C99 mode.")
				MESSAGE(STATUS "Checking if C99 is enabled by default: yes")
			ELSE(CHECK_C99_ENABLED_DEFAULT)
				MESSAGE(STATUS "Checking if C99 is enabled by default: no")
				MESSAGE(STATUS "Checking what CFLAG is required for C99:")
				FOREACH(CHECK_C99_CFLAG "-std=gnu99" "-std=c99" "-c99" "-AC99" "-xc99=all" "-qlanglvl=extc99")
					SET(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
					SET(CMAKE_REQUIRED_DEFINITIONS "${CHECK_C99_CFLAG}")
					CHECK_C_SOURCE_COMPILES("int main() { return 0; }" CFLAG_${CHECK_C99_CFLAG})
					SET(CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")
					IF(CFLAG_${CHECK_C99_CFLAG})
						SET(_SYS_C11_C99_CFLAG "${CHECK_C99_CFLAG}" CACHE INTERNAL "CFLAG required for C11 or C99 mode.")
						BREAK()
					ENDIF(CFLAG_${CHECK_C99_CFLAG})
				ENDFOREACH()

				IF(_SYS_C11_C99_CFLAG)
					MESSAGE(STATUS "Checking what CFLAG is required for C99: ${_SYS_C11_C99_CFLAG}")
				ELSE(_SYS_C11_C99_CFLAG)
					SET(${_SYS_C11_C99_CFLAG} "" CACHE INTERNAL "CFLAG required for C11 or C99 mode.")
					MESSAGE(STATUS "Checking what CFLAG is required for C99: unavailable")
				ENDIF(_SYS_C11_C99_CFLAG)
			ENDIF(CHECK_C99_ENABLED_DEFAULT)
		ENDIF(NOT CHECK_C11_ENABLED_DEFAULT AND NOT _SYS_C11_C99_CFLAG)
	ENDIF(NOT DEFINED _SYS_C11_C99_CFLAG AND NOT MSVC)

	SET(${_result} "${_SYS_C11_C99_CFLAG}" PARENT_SCOPE)
ENDFUNCTION(CHECK_C11_C99_COMPILER_FLAG)
