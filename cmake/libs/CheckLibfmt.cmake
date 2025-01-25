# Check for libfmt.
# If libfmt isn't found, extlib/libfmt/ will be used instead.

IF(NOT USE_INTERNAL_FMT)
	IF(Fmt_LIBRARY MATCHES "^fmt$" OR Fmt_LIBRARY MATCHES "^fmt")
		# Internal libfmt was previously in use.
		UNSET(Fmt_FOUND)
		UNSET(HAVE_Fmt)
		UNSET(Fmt_LIBRARY CACHE)
		UNSET(Fmt_LIBRARIES CACHE)
	ENDIF()

	# Check for libfmt.
	# - libfmt-6.2.0: Introduced the 'L' format specifier.
	#   - Previously, 'n' was available.
	#   - The 'L' specifier matches C++20 std::format().
	# - libfmt-7.0.x: Has fixes for the 'L' format specifier; removed 'n'.
	#   - 'n' can be re-enabled with FMT_DEPRECATED_N_SPECIFIER.
	# - libfmt-7.1.0: Has more fixes for the 'L' format specifier.
	# - libfmt-8.0.0: Removes the 'n' format specifier entirely.
	#
	# Since we use the 'L' specifier, we will set a minimum of libfmt-7.1.0.
	# Ubuntu 20.04 has libfmt-6.1.2; Ubuntu 22.04 has libfmt-8.1.1.
	# Also, Ubuntu 20.04's libfmt is a static library. It was changed
	# to a shared library sometime between 20.04 and 22.04.
	FIND_PACKAGE(Fmt 7.1.0)
	IF(Fmt_FOUND)
		# Found system libfmt.
		SET(HAVE_Fmt 1)
		SET(Fmt_LIBRARY "fmt::fmt")
	ELSE()
		# System libfmt was not found.
		MESSAGE(STATUS "Using the internal copy of libfmt since a system version was not found.")
		SET(USE_INTERNAL_FMT ON CACHE BOOL "Use the internal copy of libfmt" FORCE)
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of libfmt.")
ENDIF(NOT USE_INTERNAL_FMT)

IF(USE_INTERNAL_FMT)
	# Using the internal libfmt library.
	SET(Fmt_FOUND 1)
	SET(HAVE_Fmt 1)
	SET(Fmt_VERSION 11.1.2 CACHE INTERNAL "libfmt version" FORCE)
	# FIXME: When was it changed from LIBRARY to LIBRARIES?
	IF(WIN32 OR APPLE)
		# Using DLLs on Windows and Mac OS X.
		SET(USE_INTERNAL_FMT_DLL ON)
		SET(Fmt_LIBRARY fmt::fmt CACHE INTERNAL "libfmt library" FORCE)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_FMT_DLL OFF)
		SET(Fmt_LIBRARY fmt::fmt-header-only CACHE INTERNAL "libfmt library" FORCE)
	ENDIF()
	SET(Fmt_LIBRARIES ${Fmt_LIBRARY} CACHE INTERNAL "libfmt libraries" FORCE)
	# FIXME: When was it changed from DIR to DIRS?
	SET(Fmt_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extlib/libfmt)
	SET(Fmt_INCLUDE_DIRS ${Fmt_INCLUDE_DIR})
ELSE(USE_INTERNAL_FMT)
	SET(USE_INTERNAL_FMT_DLL OFF)
ENDIF(USE_INTERNAL_FMT)
