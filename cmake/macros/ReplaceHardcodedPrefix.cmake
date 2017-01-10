# Replace hard-coded prefixes.
# Parameters:
# - _var: Variable containing the path with the prefix to replcae.
# - _prefix: Prefix to replace.

MACRO(REPLACE_HARDCODED_PREFIX _var _prefix)
	IF(NOT "${_prefix}" STREQUAL "${CMAKE_INSTALL_PREFIX}")
		STRING(LENGTH "${_prefix}" prefix_LEN)
		IF(${_var})
			STRING(SUBSTRING "${${_var}}" ${prefix_LEN} -1 prefix_SUB)
			SET(${_var} "${CMAKE_INSTALL_PREFIX}${prefix_SUB}")
		ENDIF()
	ENDIF()
ENDMACRO()
