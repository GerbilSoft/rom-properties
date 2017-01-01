# Check what flags are needed for hidden visibility.
# Usage: CHECK_HIDDEN_VISIBILITY()
#
# NOTE: Do NOT use this for extlibs, since many extlibs, including
# zlib, don't work properly if symbols aren't visible by default.

MACRO(CHECK_HIDDEN_VISIBILITY)
	# Check for visibility symbols.
	IF(NOT CMAKE_VERSION VERSION_LESS 3.3.0)
		# CMake 3.3: Use CMake predefined variables.
		# NOTE: CMake 3.0-3.2 do not apply these settings
		# to static libraries, so we have to fall back to the
		# "deprecated" ADD_COMPILER_EXPORT_FLAGS().
		CMAKE_POLICY(SET CMP0063 NEW)
		SET(CMAKE_C_VISIBILITY_PRESET "hidden")
		SET(CMAKE_CXX_VISIBILITY_PRESET "hidden")
		SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
	ELSE()
		# CMake 2.x, 3.0-3.2: Use ADD_COMPILER_EXPORT_FLAGS().
		# NOTE: 3.0+ will show a deprecation warning.
		INCLUDE(GenerateExportHeader)
		ADD_COMPILER_EXPORT_FLAGS(RP_COMPILER_EXPORT_FLAGS)
		SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${RP_COMPILER_EXPORT_FLAGS}")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RP_COMPILER_EXPORT_FLAGS}")
		UNSET(RP_COMPILER_EXPORT_FLAGS)
	ENDIF()
ENDMACRO()
