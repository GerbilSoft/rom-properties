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

SET(rp-download_SRCS
	rp-download.cpp
	IDownloader.cpp
	http-status.c
	)
SET(rp-download_H
	os-secure.h
	IDownloader.hpp
	http-status.h
	)

IF(WIN32)
	# Windows: Use WinInet.
	SET(rp-download_OS_SRCS
		WinInetDownloader.cpp
		SetFileOriginInfo_win32.cpp
		resource.rc
		)
	SET(rp-download_OS_H
		WinInetDownloader.hpp
		)
	SET(rp-download-OS_SECURE_SRCS os-secure_win32.c)
ELSE(WIN32)
	# Non-Windows platforms: Use cURL.
	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	SET(rp-download_OS_SRCS
		CurlDownloader.cpp
		SetFileOriginInfo_posix.cpp
		)
	SET(rp-download_OS_H
		CurlDownloader.hpp
		)

	# Check for system security functionality.
	CHECK_SYMBOL_EXISTS(pledge "unistd.h" HAVE_PLEDGE)	# OpenBSD
	IF(HAVE_PLEDGE)
		# pledge() has a different syntax in OpenBSD 6.3 and later:
		# - 5.9-6.2:  pledge(const char *promises, const char *paths[])
		# -     6.3+: pledge(const char *promises, const char *execpromises)
		CHECK_C_SOURCE_COMPILES("
#include <unistd.h>
int main(void)
{
	pledge(\"\", \"\");
}" HAVE_PLEDGE_EXECPROMISES)
		SET(rp-download-OS_SECURE_SRCS os-secure_openbsd.c)
	ELSE()
		# TODO: Add support for other systems.
		SET(rp-download-OS_SECURE_SRCS os-secure_dummy.c)
	ENDIF()
ENDIF(WIN32)

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(rp-download_PCH_C stdafx.c)
	SET(rp-download_PCH_CXX stdafx.cpp)
	SET(rp-download_PCH_SRC ${rp-download_PCH_C} ${rp-download_PCH_CXX})
	SET(rp-download_PCH_H stdafx.h)
ENDIF(ENABLE_PCH)

#########################
# Build the executable. #
#########################

INCLUDE(SetMSVCDebugPath)

# rp-download: Cache downloader program. (Can be sandboxed.)
ADD_EXECUTABLE(rp-download
	${rp-download_PCH_SRC} ${rp-download_PCH_H}
	${rp-download_SRCS} ${rp-download_H}
	${rp-download_OS_SRCS} ${rp-download_OS_H}
	${rp-download-OS_SECURE_SRCS}
	)
IF(ENABLE_PCH)
	ADD_PRECOMPILED_HEADER(rp-download ${rp-download_PCH_H}
		SOURCE_C ${rp-download_PCH_C}
		SOURCE_CXX ${rp-download_PCH_CXX}
		)
ENDIF(ENABLE_PCH)
SET_TARGET_PROPERTIES(rp-download PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rp-download)
SET_WINDOWS_SUBSYSTEM(rp-download CONSOLE)
SET_WINDOWS_NO_MANIFEST(rp-download)
SET_WINDOWS_ENTRYPOINT(rp-download wmain OFF)

# FIXME: librpbase isn't actually needed; only the headers are.
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
	# Delay-load shell32.dll to prevent a performance penalty due to gdi32.dll.
	# Reference: https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
	# This is also needed when disabling direct Win32k syscalls,
	# since loading gdi32.dll will crash in that case.
	INCLUDE(../libwin32common/DelayLoadHelper.cmake)
	ADD_DELAYLOAD_FLAGS(rp-download shell32.dll)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(rp-download PRIVATE unixcommon inih)
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
IF(INSTALL_APPARMOR)
	# AppArmor profile (Linux only)
	# Convert the full path to use '.' instead of '/'.
	STRING(REPLACE "/" "." APPARMOR_CONF "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIBEXEC}/rp-download")
	# Remove the leading slash if present.
	STRING(REGEX REPLACE "^\\." "" APPARMOR_CONF "${APPARMOR_CONF}")

	INSTALL(FILES rp-download.apparmor.conf
		RENAME "${APPARMOR_CONF}"
		DESTINATION "${DIR_INSTALL_APPARMOR}"
		COMPONENT "apparmor"
		)
ENDIF(INSTALL_APPARMOR)
IF(INSTALL_DEBUG)
	# Split debug file
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rp-download PDB)
	IF(DEBUG_FILENAME)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "lib/debug/${DIR_INSTALL_LIBEXEC}"
			COMPONENT "debug"
			)
	ENDIF(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
