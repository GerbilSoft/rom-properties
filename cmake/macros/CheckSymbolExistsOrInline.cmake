# Check if a symbol is available either as a regular function or an inline function.
# CHECK_SYMBOL_EXISTS_OR_INLINE(_function _header _sample_code _variable)
#
#  _function    - function name to check
#  _header      - header file
#  _sample_code - sample code to compile
#  _variable    - variable to store the result

# Based on CHECK_SYMBOL_EXISTS(). (CheckSymbolExists.cmake)

INCLUDE(CheckSymbolExists)
INCLUDE(CheckCSourceCompiles)
MACRO(CHECK_SYMBOL_EXISTS_OR_INLINE _function _header _sample_code _variable)
	# MinGW defines some reentrant functions as inline functions
	# that are actually wrappers around MSVC "secure" functions.
	# Check for the function as a regular function first, then check
	# if it's available in the specified header.
	IF(NOT DEFINED ${_variable})

	CHECK_SYMBOL_EXISTS(${_function} ${_header} ${_variable})
	IF(NOT ${_variable})
		# Function does not exist normally.
		# Check if it's an inline function in the specified header.
		MESSAGE(STATUS "Looking for ${_function} as an inline function")
		CHECK_C_SOURCE_COMPILES(
			"#define _POSIX_SOURCE
			#define _POSIX_C_SOURCE 1
			#include <${_header}>
			int main(void) { ${_sample_code} }"
			${_variable}_INLINE)
		IF(${_variable}_INLINE)
			MESSAGE(STATUS "Looking for ${_variable} as an inline function - found")
			SET(${_variable} 1 CACHE INTERNAL "Have function ${_variable} (inline)")
		ELSE(${_variable}_INLINE)
			MESSAGE(STATUS "Looking for ${_variable} as an inline function - not found")
			# ${_variable} is already cached by CHECK_SYMBOL_EXISTS().
		ENDIF(${_variable}_INLINE)
	ENDIF(NOT ${_variable})

	ENDIF(NOT DEFINED ${_variable})
ENDMACRO(CHECK_SYMBOL_EXISTS_OR_INLINE _function _header _sample_code _variable)
