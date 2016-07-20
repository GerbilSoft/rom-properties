# Win32 image version linker flags.
# Determines the appropriate flags to use with MinGW and MSVC.
# The TARGET_VERSION field causes executables to be named
# program.exe-1.0 when built with MinGW, so we can't use it.
#
# The appropriate flags are automatically appended to
# CMAKE_EXE_LINKER_FLAGS_(DEBUG|RELEASE) and
# CMAKE_SHARED_LINKER_FLAGS_(DEBUG|RELEASE).

MACRO(WIN32_IMAGE_VERSION_LINKER_FLAGS _major_version _minor_version)
IF(WIN32)
	IF(MSVC)
		SET(_remove_flags "/version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR>")
		SET(_linker_flags "-version:${_major_version}.${_minor_version}")
	ELSEIF(MINGW)
		# NOTE: Also ${CMAKE_GNULD_IMAGE_VERSION}.
		SET(_remove_flags "-Wl,--major-image-version,<TARGET_VERSION_MAJOR>,--minor-image-version,<TARGET_VERSION_MINOR>")
		SET(_linker_flags "-Wl,--major-image-version,${_major_version},--minor-image-version,${_minor_version}")
	ELSE()
		MESSAGE(FATAL_ERROR "Unknown Windows compiler, please fix WIN32_IMAGE_VERSION_LINKER_FLAGS.")
	ENDIF()

	# Replace the version flags.
	STRING(REPLACE "${_remove_flags}" "${_linker_flags}" CMAKE_C_CREATE_SHARED_MODULE "${CMAKE_C_CREATE_SHARED_MODULE}")
	STRING(REPLACE "${_remove_flags}" "${_linker_flags}" CMAKE_C_CREATE_SHARED_LIBRARY "${CMAKE_C_CREATE_SHARED_LIBRARY}")
	STRING(REPLACE "${_remove_flags}" "${_linker_flags}" CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE}")
	STRING(REPLACE "${_remove_flags}" "${_linker_flags}" CMAKE_CXX_CREATE_SHARED_MODULE "${CMAKE_CXX_CREATE_SHARED_MODULE}")
	STRING(REPLACE "${_remove_flags}" "${_linker_flags}" CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_CXX_CREATE_SHARED_LIBRARY}")
	STRING(REPLACE "${_remove_flags}" "${_linker_flags}" CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")

	# Unset temporary variables.
	UNSET(_remove_flags)
	UNSET(_linker_flags)
ENDIF(WIN32)
ENDMACRO(WIN32_IMAGE_VERSION_LINKER_FLAGS)
