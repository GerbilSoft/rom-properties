# Check for LZ4.
# If LZ4 isn't found, extlib/lz4/ will be used instead.

IF(ENABLE_LZ4)

IF(NOT USE_INTERNAL_LZ4)
	# Since we're using dlopen() for liblz4 on Linux now, we're only using
	# the internal copy of LZ4 on Windows and macOS.
	IF(WIN32)
		MESSAGE(STATUS "Using the internal copy of LZ4 on Windows.")
		SET(USE_INTERNAL_LZ4 ON CACHE BOOL "Use the internal copy of LZ4" FORCE)
	ELSEIF(APPLE)
		MESSAGE(STATUS "Using the internal copy of LZ4 on macOS.")
		SET(USE_INTERNAL_LZ4 ON CACHE BOOL "Use the internal copy of LZ4" FORCE)
	ENDIF()
ELSE()
	IF(WIN32)
		MESSAGE(STATUS "Using the internal copy of LZ4 on Windows.")
	ELSEIF(APPLE)
		MESSAGE(STATUS "Using the internal copy of LZ4 on macOS.")
	ELSE()
		MESSAGE(STATUS "Using the internal copy of LZ4.")
	ENDIF()
ENDIF(NOT USE_INTERNAL_LZ4)

IF(USE_INTERNAL_LZ4)
	# Using the internal LZ4 library.
	SET(LZ4_FOUND 1)
	SET(HAVE_LZ4 1)
	# FIXME: When was it changed from LIBRARY to LIBRARIES?
	IF(WIN32)
		# Using DLLs on Windows.
		SET(USE_INTERNAL_LZ4_DLL ON)
		SET(LZ4_LIBRARY lz4_shared CACHE INTERNAL "LZ4 library" FORCE)
	ELSE()
		# Using static linking on other systems.
		SET(USE_INTERNAL_LZ4_DLL OFF)
		SET(LZ4_LIBRARY lz4_static CACHE INTERNAL "LZ4 library" FORCE)
	ENDIF()
	SET(LZ4_LIBRARIES ${LZ4_LIBRARY})
	# FIXME: When was it changed from DIR to DIRS?
	SET(LZ4_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extlib/lz4)
	SET(LZ4_INCLUDE_DIRS ${LZ4_INCLUDE_DIR})
ELSE(USE_INTERNAL_LZ4)
	SET(USE_INTERNAL_LZ4_DLL OFF)
ENDIF(USE_INTERNAL_LZ4)

ENDIF(ENABLE_LZ4)
