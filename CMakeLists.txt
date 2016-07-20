PROJECT(rom-properties)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

# TODO: Options.
SET(BUILD_KDE4 ON)

LIST(APPEND CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros"
	)

# Check for platform-specific functionality.
INCLUDE(cmake/platform.cmake)

# Project subdirectories.
ADD_SUBDIRECTORY(src)
