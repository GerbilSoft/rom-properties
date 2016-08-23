PROJECT(libcachemgr)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

# Cache Manager for downloading and caching external images.
# TODO: Make network access optional?

SET(libcachemgr_SRCS
	)
SET(libcachemgr_H
	)

IF(NOT WIN32)
	# Non-Windows platforms: Use cURL.
	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	SET(libcachemgr_OS_SRCS
		CurlDownloader.cpp
		)
	SET(libcachemgr_OS_H
		CurlDownloader.hpp
		)
ENDIF(NOT WIN32)

IF(WIN32)
	# Windows: Use Urlmon.
	# TODO: Switch to WinInet?
	SET(libcachemgr_OS_SRCS
		UrlmonDownloader.cpp
		)
	SET(libcachemgr_OS_H
		UrlmonDownloader.hpp
		)
ENDIF(WIN32)

######################
# Build the library. #
######################

# UTF-8 version.
ADD_LIBRARY(cachemgr8 STATIC
	${libcachemgr_SRCS} ${libcachemgr_H}
	${libcachemgr_OS_SRCS} ${libcachemgr_OS_H}
	)
TARGET_INCLUDE_DIRECTORIES(cachemgr8
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_LINK_LIBRARIES(cachemgr8 romdata8)
SET_TARGET_PROPERTIES(cachemgr8 PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
        )
IF(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr8 urlmon)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr8 ${CURL_LIBRARIES})
ENDIF(WIN32)

# UTF-16 version.
ADD_LIBRARY(cachemgr16 STATIC
	${libcachemgr_SRCS} ${libcachemgr_H}
	${libcachemgr_OS_SRCS} ${libcachemgr_OS_H}
	)
TARGET_INCLUDE_DIRECTORIES(cachemgr16
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
TARGET_LINK_LIBRARIES(cachemgr16 romdata16)
SET_TARGET_PROPERTIES(cachemgr16 PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
        )
IF(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr16 urlmon)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(cachemgr16 ${CURL_LIBRARIES})
ENDIF(WIN32)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
