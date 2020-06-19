# Common cache management functions used by both Cache Manager and rp-download.
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(libcachecommon LANGUAGES CXX)

IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

SET(libcachecommon_SRCS
	CacheKeys.cpp
	CacheDir.cpp
	)
SET(libcachecommon_H
	CacheKeys.hpp
	CacheDir.hpp
	)

######################
# Build the library. #
######################

INCLUDE(SetMSVCDebugPath)

ADD_LIBRARY(cachecommon STATIC
	${libcachecommon_SRCS} ${libcachecommon_H}
	)
SET_MSVC_DEBUG_PATH(cachecommon)
TARGET_INCLUDE_DIRECTORIES(cachecommon
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(cachecommon PROPERTIES EXCLUDE_FROM_ALL TRUE)
TARGET_LINK_LIBRARIES(cachecommon PRIVATE rpthreads)
IF(WIN32)
	TARGET_LINK_LIBRARIES(cachecommon PRIVATE win32common)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(cachecommon PRIVATE unixcommon)
ENDIF(WIN32)

# Unix: Add -fpic/-fPIC in order to use these static libraries in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
