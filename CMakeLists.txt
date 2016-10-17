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
SET(PACKAGE_NAME "rom-properties")
SET(AUTHOR "David Korth")
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 7)
SET(VERSION_PATCH 99)
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

# Git version information.
FIND_PROGRAM(POSIX_SH sh)
IF(POSIX_SH)
	# sh is available.
	# Run the git version script.
	IF(WIN32)
		SET(ENV{SHELLOPTS} igncr)
	ENDIF(WIN32)
	ADD_CUSTOM_TARGET(git_version ALL
		${POSIX_SH} "${CMAKE_CURRENT_SOURCE_DIR}/git_version.sh"
		-s "${CMAKE_CURRENT_SOURCE_DIR}"
		-o "${CMAKE_CURRENT_BINARY_DIR}/git_version.h"
		VERBATIM
		)
ELSE(POSIX_SH)
	# sh isn't available.
	# Create a blank git_version.h.
	FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/git_version.h"
		"/* dummy file; POSIX sh is not available */\n")
ENDIF(POSIX_SH)

# Make sure the file is deleted on `make clean`.
SET_PROPERTY(DIRECTORY APPEND
	PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/git_version.h")

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

# CPack settings.
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${DESCRIPTION}")
SET(CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
SET(CPACK_PACKAGE_VENDOR "${AUTHOR}")
SET(CPACK_PACKAGE_CONTACT "David Korth <gerbilsoft@gerbilsoft.com>")
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
SET(CPACK_PACKAGE_VERSION ${VERSION_STRING})

# TODO: DESCRIPTION and WELCOME files.
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/doc/DESCRIPTION.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
#SET(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/doc/WELCOME.txt")

# CPack: Source package settings.
# NOTE: Double-escape is required because the unescaped
# string # is written to CPackSourceConfig.cmake, which
# is then unescaped.
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES
	"build.*/"
	"build.*\\\\.sh"
	"\\\\.git/"
	"\\\\.gitignore"
	"*\\\\.kate-swp"
	)

IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.|x)86\$")
	SET(CPACK_PACKAGE_SYSTEM_PROCESSOR "i386")
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64\$")
	SET(CPACK_PACKAGE_SYSTEM_PROCESSOR "amd64")
ELSE()
	SET(CPACK_PACKAGE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
ENDIF()

IF(UNIX AND NOT APPLE)
	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_PACKAGE_SYSTEM_PROCESSOR}")
	SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}")
ELSEIF(APPLE)
	# TODO: Support for Mac OS X.
ELSEIF(WIN32)
	IF(MSVC AND CMAKE_CL_64)
		SET(WIN32_PACKAGE_SUFFIX "win64")
	ELSEIF(NOT MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		SET(WIN32_PACKAGE_SUFFIX "win64")
	ELSE()
		SET(WIN32_PACKAGE_SUFFIX "win32")
	ENDIF()

	SET(CPACK_GENERATOR "ZIP")
	SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${WIN32_PACKAGE_SUFFIX}")
	SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
ENDIF()

# Components.
SET(CPACK_COMPONENTS_ALL dll debug doc xdg-mime)
SET(CPACK_COMPONENT_DLL_DISPLAY_NAME "DLLs")
SET(CPACK_COMPONENT_DEBUG_DISPLAY_NAME "Debugging Symbols")

# Initialize CPack.
INCLUDE(CPack)
