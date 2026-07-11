# Check for minizip-ng. (NOTE: minizip-ng *only*, not original minizip)
# If minizip-ng isn't found, extlib/minizip-ng/ will be used instead.

IF(NOT USE_INTERNAL_MinizipNG)
	# Check for minizip-ng.
	FIND_PACKAGE(MinizipNG)
	IF(MinizipNG_FOUND)
		# Found system minizip.
		SET(HAVE_MinizipNG 1)
	ELSE()
		# System minizip was not found.
		MESSAGE(STATUS "Using the internal copy of minizip-ng since a system version was not found.")
		SET(USE_INTERNAL_MINIZIP ON CACHE BOOL "Use the internal copy of minizip-ng" FORCE)
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of minizip-ng.")
ENDIF(NOT USE_INTERNAL_MinizipNG)

IF(USE_INTERNAL_MinizipNG)
	# Using the internal minizip library.
	SET(MinizipNG_FOUND 1)
	SET(HAVE_MinizipNG 1)
	IF(WIN32)
		# Using DLLs on Windows.
		SET(USE_INTERNAL_MINIZIP_DLL ON)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_MINIZIP_DLL OFF)
	ENDIF()
	SET(MinizipNG_LIBRARY minizip-ng CACHE INTERNAL "minizip library" FORCE)
	SET(MinizipNG_LIBRARIES ${MinizipNG_LIBRARY})
	# FIXME: When was it changed from DIR to DIRS?
	SET(MINIZIP_INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/extlib/minizip-ng
		${CMAKE_BINARY_DIR}/extlib/minizip-ng
		)
	SET(MINIZIP_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
ELSE(USE_INTERNAL_MinizipNG)
	SET(USE_INTERNAL_MinizipNG_DLL OFF)
ENDIF(USE_INTERNAL_MinizipNG)
