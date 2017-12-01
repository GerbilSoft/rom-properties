PROJECT(libcachemgr)

# Cache Manager for downloading and caching external images.
# TODO: Make network access optional?

SET(libcachemgr_SRCS
	IDownloader.cpp
	CacheManager.cpp
	)
SET(libcachemgr_H
	IDownloader.hpp
	CacheManager.hpp
	)

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

ADD_LIBRARY(cachemgr STATIC
	${libcachemgr_SRCS} ${libcachemgr_H}
	${libcachemgr_OS_SRCS} ${libcachemgr_OS_H}
	${libcachemgr_THREAD_SRCS} ${libcachemgr_THREAD_H}
	)
TARGET_LINK_LIBRARIES(cachemgr rpbase)

TARGET_INCLUDE_DIRECTORIES(cachemgr
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(cachemgr PROPERTIES EXCLUDE_FROM_ALL TRUE)
IF(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr win32common)
	TARGET_LINK_LIBRARIES(cachemgr urlmon wininet)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr ${CURL_LIBRARIES})
ENDIF(WIN32)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
