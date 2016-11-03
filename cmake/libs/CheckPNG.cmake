# Check for libpng.
# If libpng isn't found, extlib/libpng/ will be used instead.

IF(NOT USE_INTERNAL_PNG)
	# Check for libpng.
	FIND_PACKAGE(PNG)
	IF(PNG_FOUND)
		# Found system libpng.
		# TODO: APNG check?
		SET(HAVE_PNG 1)
	ELSE()
		# System libpng was not found.
		MESSAGE(STATUS "Using the internal copy of libpng since a system version was not found.")
		SET(USE_INTERNAL_PNG ON CACHE STRING "Use the internal copy of libpng.")
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of libpng.")
ENDIF(NOT USE_INTERNAL_PNG)

IF(USE_INTERNAL_PNG)
	# Using the internal zlib library.
	SET(PNG_FOUND 1)
	SET(HAVE_PNG 1)
	SET(PNG_LIBRARY png_static)
	SET(PNG_DEFINITIONS -DPNG_STATIC)
	# FIXME: When was it changed from DIR to DIRS?
	SET(PNG_INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/extlib/libpng
		${CMAKE_BINARY_DIR}/extlib/libpng
		)
	SET(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
ENDIF(USE_INTERNAL_PNG)
