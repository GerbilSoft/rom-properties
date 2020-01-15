PROJECT(rp-download)
# Online downloader program for external images.

SET(rp-download_SRCS rp-download.cpp IDownloader.cpp)
SET(rp-download_H IDownloader.hpp)

IF(WIN32)
	# Windows: Use Urlmon.
	# TODO: Switch to WinInet?
	SET(rp-download_OS_SRCS UrlmonDownloader.cpp)
	SET(rp-download_OS_H UrlmonDownloader.hpp)
ELSE(WIN32)
	# Non-Windows platforms: Use cURL.
	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	SET(rp-download_OS_SRCS CurlDownloader.cpp)
	SET(rp-download_OS_H CurlDownloader.hpp)
ENDIF(WIN32)

#########################
# Build the executable. #
#########################

INCLUDE(SetMSVCDebugPath)

# rp-download: Cache downloader program. (Can be sandboxed.)
ADD_EXECUTABLE(rp-download
	${rp-download_SRCS} ${rp-download_H}
	${rp-download_OS_SRCS} ${rp-download_OS_H}
	)
SET_TARGET_PROPERTIES(rp-download PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rp-download)
SET_WINDOWS_SUBSYSTEM(rp-download CONSOLE)
#SET_WINDOWS_NO_MANIFEST(rp-download)
SET_WINDOWS_ENTRYPOINT(rp-download wmain OFF)

TARGET_LINK_LIBRARIES(rp-download PRIVATE rpbase cachecommon)
TARGET_INCLUDE_DIRECTORIES(rp-download
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(rp-download PRIVATE win32common)
	TARGET_LINK_LIBRARIES(rp-download PRIVATE urlmon wininet)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(rp-download PRIVATE ${CURL_LIBRARIES})
ENDIF(WIN32)
