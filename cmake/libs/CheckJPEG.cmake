# Check for libjpeg.
# If libjpeg isn't found, extlib/libjpeg-turbo/ will be used instead.
IF(ENABLE_JPEG)

IF(NOT USE_INTERNAL_JPEG)
	# Check for libjpeg.
	FIND_PACKAGE(JPEG)
	IF(JPEG_FOUND)
		# Found system libjpeg.
		SET(HAVE_JPEG 1)
	ELSE()
		# System libjpeg was not found.
		IF(NOT WIN32)
			MESSAGE(FATAL_ERROR "System libjpeg was not found.\nCannot use the internal libjpeg-turbo on non-Windows platforms.")
		ENDIF(NOT WIN32)
		MESSAGE(STATUS "Using the internal copy of libjpeg-turbo since a system version was not found.")
		SET(USE_INTERNAL_JPEG ON CACHE STRING "Use the internal copy of libjpeg.")
	ENDIF()
ELSE()
	IF(NOT WIN32)
		MESSAGE(FATAL_ERROR "Cannot use internal libjpeg-turbo on non-Windows platforms.")
	ENDIF(NOT WIN32)
	MESSAGE(STATUS "Using the internal copy of libjpeg.")
ENDIF(NOT USE_INTERNAL_JPEG)

IF(USE_INTERNAL_JPEG)
	# Using the internal JPEG library.
	SET(JPEG_FOUND 1)
	SET(HAVE_JPEG 1)
	IF(WIN32 OR APPLE)
		# Using DLLs on Windows and Mac OS X.
		# NOTE: libjpeg-turbo 1.5.1's CMakeLists only builds
		# a DLL version of turbojpeg, not libjpeg.
		SET(USE_INTERNAL_JPEG_DLL ON)
		SET(JPEG_LIBRARY jpeg)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_JPEG_DLL OFF)
		SET(JPEG_LIBRARY jpeg-static)
	ENDIF()
	SET(JPEG_INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/extlib/libjpeg-turbo
		${CMAKE_BINARY_DIR}/extlib/libjpeg-turbo
		)
	SET(JPEG_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})
ELSE(USE_INTERNAL_JPEG)
	SET(USE_INTERNAL_JPEG_DLL OFF)
ENDIF(USE_INTERNAL_JPEG)

ELSE(ENABLE_JPEG)
	# Disable JPEG.
	UNSET(JPEG_FOUND)
	UNSET(HAVE_JPEG)
	UNSET(USE_INTERNAL_JPEG)
	UNSET(USE_INTERNAL_JPEG_DLL)
	UNSET(JPEG_LIBRARY)
	UNSET(JPEG_INCLUDE_DIR)
	UNSET(JPEG_INCLUDE_DIRS)
ENDIF(ENABLE_JPEG)
