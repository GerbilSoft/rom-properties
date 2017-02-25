# Check for 64-bit time support.
# Known cases:
# - Windows, MSVC: Supported as of MSVC 2005.
# - Windows, MinGW: Supported if -D__MINGW_USE_VC2005_COMPAT is defined.
# - 32-bit Linux: Supported in an upcoming glibc release.
# - 64-bit Linux: No macros are required.
# - 32-bit Mac OS X: Not supported.
# - 64-bit Mac OS X: No macros are required.

# Partially based on:
# https://github.com/Benjamin-Dobell/Heimdall/blob/master/cmake/LargeFiles.cmake

# This sets the following variables:
# - TIME64_FOUND: Set to 1 if 64-bit time_t is supported.
# - TIME64_FOUND_MINGW: Set to 1 if MinGW-w64's -D__MINGW_USE_VC2005_COMPAT is in use.
# - TIME64_FOUND_TIME_BITS: Set to 1 if glibc's -D_TIME_BITS=64 is in use.
# - TIME64_DEFINITIONS: Preprocessor macros required for large file support, if any.

FUNCTION(CHECK_64BIT_TIME_SUPPORT)
	IF(NOT DEFINED TIME64_FOUND)
		# NOTE: ${CMAKE_MODULE_PATH} has two directories, macros/ and libs/,
		# so we have to configure this manually.
		SET(TIME64_SOURCE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

		# Check for 64-bit time_t.
		MESSAGE(STATUS "Checking if time_t is 64-bit")
		IF(MSVC)
			IF(MSVC_VERSION LESS 1310)
				MESSAGE(STATUS "Checking if time_t is 64-bit - no")
				MESSAGE(WARNING "MSVC 2005 (8.0) or later is required for 64-bit time_t.")
			ELSE()
				MESSAGE(STATUS "Checking if time_t is 64-bit - yes")
				SET(TMP_TIME64_FOUND 1)
			ENDIF()
		ELSEIF(MINGW)
			# MinGW should support 64-bit time_t if -D__MINGW_USE_VC2005_COMPAT is specified.
			SET(TMP_TIME64_DEFINITIONS -D__MINGW_USE_VC2005_COMPAT)
			TRY_COMPILE(TMP_TIME64_FOUND "${CMAKE_BINARY_DIR}"
				"${TIME64_SOURCE_PATH}/64BitTimeSupport.c"
				COMPILE_DEFINITIONS ${TMP_TIME64_DEFINITIONS})
			IF(TMP_TIME64_FOUND)
				MESSAGE(STATUS "Checking if time_t is 64-bit - yes, using -D__MINGW_USE_VC2005_COMPAT")
				SET(TMP_TIME64_FOUND_MINGW 1)
			ELSE(TMP_TIME64_FOUND)
				MESSAGE(STATUS "Checking if time_t is 64-bit - no")
				MESSAGE(WARNING "MinGW-w64 is required for 64-bit time_t.")
				UNSET(TMP_TIME64_DEFINITIONS)
			ENDIF(TMP_TIME64_FOUND)
		ELSE()
			# Check if the OS supports 64-bit time_t out of the box.
			TRY_COMPILE(TMP_TIME64_FOUND "${CMAKE_BINARY_DIR}"
				"${TIME64_SOURCE_PATH}/64BitTimeSupport.c")
			IF(TMP_TIME64_FOUND)
				# Supported out of the box.
				MESSAGE(STATUS "Checking if time_t is 64-bit - yes")
			ELSE()
				# Try adding 64-bit time_t macros.
				# Reference: https://sourceware.org/glibc/wiki/Y2038ProofnessDesign?rev=115
				SET(TMP_TIME64_DEFINITIONS -D_TIME_BITS=64)
				TRY_COMPILE(TMP_TIME64_FOUND "${CMAKE_BINARY_DIR}"
					"${TIME64_SOURCE_PATH}/64BitTimeSupport.c"
					COMPILE_DEFINITIONS ${TMP_TIME64_DEFINITIONS})
				IF(TMP_TIME64_FOUND)
					# TIME64 macros work.
					MESSAGE(STATUS "Checking if time_t is 64-bit - yes, using -D_TIME_BITS=64")
					SET(TMP_TIME64_FOUND_TIME_BITS 1)
				ELSE()
					# TIME64 macros failed.
					MESSAGE(STATUS "Checking if time_t is 64-bit - no")
					UNSET(TMP_TIME64_DEFINITIONS)
				ENDIF()
			ENDIF()
		ENDIF()

		SET(TIME64_FOUND ${TMP_TIME64_FOUND} CACHE INTERNAL "Is Large File Support available?")
		SET(TIME64_FOUND_MINGW ${TMP_TIME64_FOUND_TIME_BITS} CACHE INTERNAL "64-bit time_t is available using -D__MINGW_USE_VC2005_COMPAT")
		SET(TIME64_FOUND_TIME_BITS ${TMP_TIME64_FOUND_TIME_BITS} CACHE INTERNAL "64-bit time_t is available using -D_TIME_BITS=64")
		SET(TIME64_DEFINITIONS "${TMP_TIME64_DEFINITIONS}" CACHE INTERNAL "Definitions required for 64-bit time_t")
	ENDIF()
ENDFUNCTION(CHECK_64BIT_TIME_SUPPORT)
