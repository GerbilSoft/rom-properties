# Check for zlib.
# If zlib isn't found, extlib/zlib/ will be used instead.

# Enable ZLIB const qualifiers.
SET(ZLIB_DEFINITIONS -DZLIB_CONST)

IF(NOT USE_INTERNAL_ZLIB)
	# Check for ZLIB.
	FIND_PACKAGE(ZLIB)
	IF(ZLIB_FOUND)
		# Found system ZLIB.
		SET(HAVE_ZLIB 1)
	ELSE()
		# System ZLIB was not found.
		MESSAGE(STATUS "Using the internal copy of zlib since a system version was not found.")
		SET(USE_INTERNAL_ZLIB ON CACHE STRING "Use the internal copy of zlib.")
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of zlib.")
ENDIF(NOT USE_INTERNAL_ZLIB)

IF(USE_INTERNAL_ZLIB)
	# Using the internal zlib library.
	SET(ZLIB_FOUND 1)
	SET(HAVE_ZLIB 1)
	# FIXME: When was it changed from LIBRARY to LIBRARIES?
	SET(ZLIB_LIBRARY zlibstatic)
	SET(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
	# FIXME: When was it changed from DIR to DIRS?
	SET(ZLIB_INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/extlib/zlib
		${CMAKE_BINARY_DIR}/extlib/zlib
		)
	SET(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
ENDIF(USE_INTERNAL_ZLIB)
