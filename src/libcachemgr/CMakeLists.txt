PROJECT(libcachemgr)
# Cache Manager for downloading and caching external images.
# Downloading is handled using a separate executable, rp-download,
# which can be sandboxed.

SET(libcachecommon_SRCS cache_common.cpp)
SET(libcachecommon_H cache_common.hpp)

SET(libcachemgr_SRCS CacheManager.cpp IDownloader.cpp)
SET(libcachemgr_H CacheManager.hpp IDownloader.hpp)

IF(WIN32)
	# Windows: Use Urlmon.
	# TODO: Switch to WinInet?
	SET(libcachemgr_OS_SRCS UrlmonDownloader.cpp)
	SET(libcachemgr_OS_H UrlmonDownloader.hpp)
ELSE(WIN32)
	# Non-Windows platforms: Use cURL.
	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	SET(libcachemgr_OS_SRCS CurlDownloader.cpp)
	SET(libcachemgr_OS_H CurlDownloader.hpp)
ENDIF(WIN32)

######################
# Build the library. #
######################

INCLUDE(SetMSVCDebugPath)

# libcachecommon: Common library for libcachemgr and rp-download.
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

# libcachemgr: rom-properties.dll cache interface.
ADD_LIBRARY(cachemgr STATIC
	${libcachemgr_SRCS} ${libcachemgr_H}
	${libcachemgr_OS_SRCS} ${libcachemgr_OS_H}
	)
SET_MSVC_DEBUG_PATH(cachemgr)
TARGET_LINK_LIBRARIES(cachemgr PUBLIC rpbase)
TARGET_LINK_LIBRARIES(cachemgr PRIVATE rpthreads cachecommon)
TARGET_INCLUDE_DIRECTORIES(cachemgr
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(cachemgr PROPERTIES EXCLUDE_FROM_ALL TRUE)
IF(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr PRIVATE win32common)
	TARGET_LINK_LIBRARIES(cachemgr PRIVATE urlmon wininet)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr PRIVATE ${CURL_LIBRARIES})
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
