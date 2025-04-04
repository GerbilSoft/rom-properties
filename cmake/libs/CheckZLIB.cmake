# Check for zlib.
# If zlib isn't found, extlib/zlib/ will be used instead.

# Enable ZLIB const qualifiers.
SET(ZLIB_DEFINITIONS -DZLIB_CONST)

IF(NOT USE_INTERNAL_ZLIB)
	IF(ZLIB_LIBRARY MATCHES "^zlib$" OR ZLIB_LIBRARY MATCHES "^zlibstatic$")
		# Internal zlib was previously in use.
		UNSET(ZLIB_FOUND)
		UNSET(HAVE_ZLIB)
		UNSET(ZLIB_LIBRARY CACHE)
		UNSET(ZLIB_LIBRARIES CACHE)
	ENDIF()

	# Check for ZLIB.
	FIND_PACKAGE(ZLIB)
	IF(ZLIB_FOUND)
		# Found system ZLIB.
		SET(HAVE_ZLIB 1)
	ELSE()
		# System ZLIB was not found.
		MESSAGE(STATUS "Using the internal copy of zlib-ng since a system version was not found.")
		SET(USE_INTERNAL_ZLIB ON CACHE BOOL "Use the internal copy of zlib-ng" FORCE)
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of zlib-ng.")
ENDIF(NOT USE_INTERNAL_ZLIB)

IF(USE_INTERNAL_ZLIB)
	# Using the internal zlib library.
	SET(ZLIB_FOUND 1)
	SET(HAVE_ZLIB 1)
	# FIXME: When was it changed from LIBRARY to LIBRARIES?
	IF(WIN32)
		# Using DLLs on Windows.
		SET(USE_INTERNAL_ZLIB_DLL ON)
		SET(ZLIB_LIBRARY zlib CACHE INTERNAL "ZLIB library" FORCE)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_ZLIB_DLL OFF)
		SET(ZLIB_LIBRARY zlibstatic CACHE INTERNAL "ZLIB library" FORCE)
	ENDIF()
	SET(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
	# FIXME: When was it changed from DIR to DIRS?
	SET(ZLIB_INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/extlib/zlib-ng
		${CMAKE_BINARY_DIR}/extlib/zlib-ng
		)
	SET(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
ELSE(USE_INTERNAL_ZLIB)
	SET(USE_INTERNAL_ZLIB_DLL OFF)
ENDIF(USE_INTERNAL_ZLIB)
