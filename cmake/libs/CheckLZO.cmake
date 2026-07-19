# Check for LZO.
# If LZO isn't found, extlib/lzo/ will be used instead.

IF(ENABLE_LZO)

IF(NOT USE_INTERNAL_LZO)
	# Since we're using dlopen() for liblzo2 on Linux now, we're only using
	# the internal copy of LZO on Windows and macOS.
	IF(WIN32)
		MESSAGE(STATUS "Using the internal copy of LZO on Windows.")
		SET(USE_INTERNAL_LZO ON CACHE BOOL "Use the internal copy of LZO" FORCE)
	ELSEIF(APPLE)
		MESSAGE(STATUS "Using the internal copy of LZO on macOS.")
		SET(USE_INTERNAL_LZO ON CACHE BOOL "Use the internal copy of LZO" FORCE)
	ENDIF()
ELSE()
	IF(WIN32)
		MESSAGE(STATUS "Using the internal copy of LZO on Windows.")
	ELSEIF(APPLE)
		MESSAGE(STATUS "Using the internal copy of LZO on macOS.")
	ELSE()
		MESSAGE(STATUS "Using the internal copy of LZO.")
	ENDIF()
ENDIF(NOT USE_INTERNAL_LZO)

IF(USE_INTERNAL_LZO)
	# Using the internal LZO library. (MiniLZO)
	SET(LZO_FOUND 1)
	SET(HAVE_LZO 1)
	# FIXME: When was it changed from LIBRARY to LIBRARIES?
	IF(WIN32)
		# Using DLLs on Windows.
		SET(USE_INTERNAL_LZO_DLL ON)
		SET(LZO_LIBRARY minilzo CACHE INTERNAL "LZO library" FORCE)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_LZO_DLL OFF)
		SET(LZO_LIBRARY minilzo CACHE INTERNAL "LZO library" FORCE)
	ENDIF()
	SET(LZO_LIBRARIES ${LZO_LIBRARY})
	# FIXME: When was it changed from DIR to DIRS?
	SET(LZO_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extlib/minilzo)
	SET(LZO_INCLUDE_DIRS ${LZO_INCLUDE_DIR})
ELSE(USE_INTERNAL_LZO)
	SET(USE_INTERNAL_LZO_DLL OFF)
ENDIF(USE_INTERNAL_LZO)

ENDIF(ENABLE_LZO)
