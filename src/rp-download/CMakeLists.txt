PROJECT(rp-download)
# Online downloader program for external images.

IF(APPLE)
	CHECK_SYMBOL_EXISTS(fsetxattr "sys/xattr.h" HAVE_FSETXATTR_MAC)
ELSEIF(NOT WIN32)
	CHECK_SYMBOL_EXISTS(fsetxattr "sys/xattr.h" HAVE_FSETXATTR_LINUX)
	CHECK_SYMBOL_EXISTS(extattr_set_fd "sys/extattr.h" HAVE_EXTATTR_SET_FD)
ENDIF()

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.rp-download.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.rp-download.h")

IF(WIN32)
	# Create the manifest file.
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/rp-download.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/rp-download.exe.manifest" @ONLY)
ENDIF(WIN32)

SET(rp-download_SRCS rp-download.cpp IDownloader.cpp)
SET(rp-download_H os-secure.h IDownloader.hpp)

IF(WIN32)
	# Windows: Use WinInet.
	SET(rp-download_OS_SRCS
		os-secure_win32.c
		WinInetDownloader.cpp
		SetFileOriginInfo_win32.cpp
		resource.rc
		)
	SET(rp-download_OS_H
		WinInetDownloader.hpp
		)
ELSE(WIN32)
	# Non-Windows platforms: Use cURL.
	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	SET(rp-download_OS_SRCS
		os-secure_posix.c
		CurlDownloader.cpp
		SetFileOriginInfo_posix.cpp
		)
	SET(rp-download_OS_H
		CurlDownloader.hpp
		)
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
SET_WINDOWS_NO_MANIFEST(rp-download)
SET_WINDOWS_ENTRYPOINT(rp-download wmain OFF)

TARGET_LINK_LIBRARIES(rp-download PRIVATE rpbase cachecommon)
TARGET_INCLUDE_DIRECTORIES(rp-download
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# rp-download
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# rp-download
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(rp-download PRIVATE win32common)
	TARGET_LINK_LIBRARIES(rp-download PRIVATE wininet advapi32)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(rp-download PRIVATE ${CURL_LIBRARIES})
ENDIF(WIN32)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}\"")
ENDIF(MINGW)

###########################
# Install the executable. #
###########################

INCLUDE(DirInstallPaths)
INSTALL(TARGETS rp-download
	RUNTIME DESTINATION "${DIR_INSTALL_LIBEXEC}"
	COMPONENT "plugin"
	)

# Check if a split debug file should be installed.
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rp-download PDB)
	IF(DEBUG_FILENAME)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "lib/debug/${DIR_INSTALL_LIBEXEC}"
			COMPONENT "debug"
			)
	ENDIF(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
