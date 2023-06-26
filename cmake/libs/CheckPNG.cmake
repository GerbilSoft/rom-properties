# Check for libpng.
# If libpng isn't found, extlib/libpng/ will be used instead.

IF(NOT USE_INTERNAL_PNG)
	IF(PNG_LIBRARY MATCHES "^png_shared$" OR PNG_LIBRARY MATCHES "^png_static$")
		# Internal libpng was previously in use.
		UNSET(PNG_FOUND)
		UNSET(HAVE_PNG)
		UNSET(PNG_LIBRARY CACHE)
	ENDIF()

	# Check for libpng.
	FIND_PACKAGE(PNG)
	IF(PNG_FOUND)
		# Found system libpng.
		# TODO: APNG check?
		SET(HAVE_PNG 1)
	ELSE()
		# System libpng was not found.
		MESSAGE(STATUS "Using the internal copy of libpng since a system version was not found.")
		SET(USE_INTERNAL_PNG ON CACHE BOOL "Use the internal copy of libpng" FORCE)
	ENDIF()
ELSE()
	MESSAGE(STATUS "Using the internal copy of libpng.")
ENDIF(NOT USE_INTERNAL_PNG)

IF(USE_INTERNAL_PNG)
	# Using the internal PNG library.
	SET(PNG_FOUND 1)
	SET(HAVE_PNG 1)
	IF(WIN32 OR APPLE)
		# Using DLLs on Windows and Mac OS X.
		SET(USE_INTERNAL_PNG_DLL ON)
		SET(PNG_LIBRARY png_shared CACHE INTERNAL "PNG library" FORCE)
		UNSET(PNG_DEFINITIONS)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_PNG_DLL OFF)
		SET(PNG_LIBRARY png_static CACHE INTERNAL "PNG library" FORCE)
		SET(PNG_DEFINITIONS -DPNG_STATIC)
	ENDIF()
	# FIXME: When was it changed from DIR to DIRS?
	SET(PNG_INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/extlib/libpng
		${CMAKE_BINARY_DIR}/extlib/libpng
		)
	SET(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
ELSE(USE_INTERNAL_PNG)
	SET(USE_INTERNAL_PNG_DLL OFF)
ENDIF(USE_INTERNAL_PNG)
