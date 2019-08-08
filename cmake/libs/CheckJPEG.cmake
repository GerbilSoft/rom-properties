# Check for libjpeg.
# If libjpeg isn't found, extlib/libjpeg-turbo/ will be used instead.
IF(ENABLE_JPEG)

IF(WIN32)
	MESSAGE(STATUS "Using gdiplus for JPEG decoding.")
	SET(JPEG_FOUND 1)
	SET(HAVE_JPEG 1)
	SET(USE_INTERNAL_JPEG_DLL OFF)
	SET(JPEG_LIBRARY gdiplus CACHE INTERNAL "JPEG library" FORCE)
ELSE(WIN32)
	IF(JPEG_LIBRARY MATCHES "^jpeg$" OR JPEG_LIBRARY MATCHES "^jpeg-static$")
		# Internal libjpeg was previously in use.
		UNSET(JPEG_FOUND)
		UNSET(HAVE_JPEG)
		UNSET(JPEG_LIBRARY CACHE)
	ENDIF()

	# Check for libjpeg.
	FIND_PACKAGE(JPEG)
	IF(JPEG_FOUND)
		# Found system libjpeg.
		SET(HAVE_JPEG 1)
	ENDIF()
ENDIF(WIN32)

ELSE(ENABLE_JPEG)
	# Disable JPEG.
	UNSET(JPEG_FOUND)
	UNSET(HAVE_JPEG)
	UNSET(JPEG_LIBRARY)
	UNSET(JPEG_INCLUDE_DIR)
	UNSET(JPEG_INCLUDE_DIRS)
ENDIF(ENABLE_JPEG)
