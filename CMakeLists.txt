PROJECT(rom-properties)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

LIST(APPEND CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	)

# Enable testing.
# TODO: INCLUDE(CTest) for more advanced testing after
# enough tests are added.
ENABLE_TESTING()

# Set default build options.
INCLUDE(cmake/options.cmake)
# Check for platform-specific functionality.
INCLUDE(cmake/platform.cmake NO_POLICY_SCOPE)

# Program information.
SET(DESCRIPTION "ROM Properties Page shell extension")
SET(AUTHOR "David Korth")
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 0)
SET(VERSION_PATCH 0)
SET(VERSION_DEVEL 1)
IF(VERSION_PATCH)
	SET(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
ELSE(VERSION_PATCH)
	SET(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")
ENDIF(VERSION_PATCH)
IF(VERSION_DEVEL)
	SET(VERSION_STRING "${VERSION_STRING}+")
ENDIF(VERSION_DEVEL)
SET(VERSION_STRING_WIN32 "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},${VERSION_DEVEL}")

# Split Debug macro.
# Also sets the image version for Windows builds.
INCLUDE(Win32ImageVersionLinkerFlags)
MACRO(DO_SPLIT_DEBUG _TARGET)
	# Split debug information.
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_TARGET})
	IF(SPLIT_DEBUG)
		INCLUDE(SplitDebugInformation)
		SPLIT_DEBUG_INFORMATION(${_TARGET})
	ENDIF(SPLIT_DEBUG)
	# Set image version.
	# Subprojects can override ${VERSION_MAJOR} and ${VERSION_MINOR}.
	# FIXME: If minor version is e.g. "3", Windows interprets it as "03",
	# so "1.3" will actually be "1.03".
	WIN32_IMAGE_VERSION_LINKER_FLAGS(${VERSION_MAJOR} ${VERSION_MINOR})
ENDMACRO(DO_SPLIT_DEBUG)

# If no build type is set, default to "debug".
# TODO: Default to "release"?
IF(CMAKE_BUILD_TYPE MATCHES ^$)
       SET(CMAKE_BUILD_TYPE "debug")
ENDIF()

# Project subdirectories.
ADD_SUBDIRECTORY(extlib)
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(doc)
# NOTE: scripts currently only has Windows-specific files.
IF(WIN32)
	ADD_SUBDIRECTORY(scripts)
ENDIF(WIN32)

# TODO: Print build summary indicating what plugins will be built.
# (Some other project had something like this...)
# TODO: Fail if no plugins are being built.
