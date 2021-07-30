# Online downloader program for external images
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rp-download LANGUAGES C CXX)

# Check for an xattr function.
INCLUDE(CheckSymbolExists)
IF(APPLE)
	CHECK_SYMBOL_EXISTS(fsetxattr "sys/xattr.h" HAVE_FSETXATTR_MAC)
ELSEIF(NOT WIN32)
	CHECK_SYMBOL_EXISTS(fsetxattr "sys/xattr.h" HAVE_FSETXATTR_LINUX)
	CHECK_SYMBOL_EXISTS(extattr_set_fd "sys/extattr.h" HAVE_EXTATTR_SET_FD)
ENDIF()

IF(NOT WIN32)
	# Check for statx().
	SET(OLD_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
	SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
	CHECK_SYMBOL_EXISTS(statx "sys/stat.h" HAVE_STATX)
	SET(CMAKE_REQUIRED_DEFINITIONS "${OLD_CMAKE_REQUIRED_DEFINITIONS}")
	UNSET(OLD_CMAKE_REQUIRED_DEFINITIONS)
ENDIF(NOT WIN32)

IF(APPLE)
	FIND_LIBRARY(CORESERVICES_LIBRARY CoreServices REQUIRED)
ENDIF(APPLE)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.${PROJECT_NAME}.h")

SET(${PROJECT_NAME}_SRCS
	rp-download.cpp
	IDownloader.cpp
	http-status.c
	)
SET(${PROJECT_NAME}_H
	IDownloader.hpp
	http-status.h
	)

IF(WIN32)
	# Windows: Use WinInet.
	SET(${PROJECT_NAME}_OS_SRCS
		WinInetDownloader.cpp
		SetFileOriginInfo_win32.cpp
		resource.rc
		)
	SET(${PROJECT_NAME}_OS_H
		WinInetDownloader.hpp
		)

	# Create the manifest file.
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.exe.manifest" @ONLY)
ELSE(WIN32)
	# Non-Windows platforms: Use cURL.
	FIND_PACKAGE(CURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
	SET(${PROJECT_NAME}_OS_SRCS
		CurlDownloader.cpp
		SetFileOriginInfo_posix.cpp
		)
	SET(${PROJECT_NAME}_OS_H
		CurlDownloader.hpp
		)
ENDIF()

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(${PROJECT_NAME}_PCH_C stdafx.c)
	SET(${PROJECT_NAME}_PCH_CXX stdafx.cpp)
	SET(${PROJECT_NAME}_PCH_SRC ${${PROJECT_NAME}_PCH_C} ${${PROJECT_NAME}_PCH_CXX})
	SET(${PROJECT_NAME}_PCH_H stdafx.h)
ENDIF(ENABLE_PCH)

#########################
# Build the executable. #
#########################

INCLUDE(SetMSVCDebugPath)

# rp-download: Cache downloader program. (Can be sandboxed.)
ADD_EXECUTABLE(${PROJECT_NAME}
	${${PROJECT_NAME}_PCH_SRC} ${${PROJECT_NAME}_PCH_H}
	${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_OS_SRCS} ${${PROJECT_NAME}_OS_H}
	)
IF(ENABLE_PCH)
	ADD_PRECOMPILED_HEADER(${PROJECT_NAME} ${${PROJECT_NAME}_PCH_H}
		SOURCE_C ${${PROJECT_NAME}_PCH_C}
		SOURCE_CXX ${${PROJECT_NAME}_PCH_CXX}
		)
ENDIF(ENABLE_PCH)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(${PROJECT_NAME})
SET_WINDOWS_SUBSYSTEM(${PROJECT_NAME} CONSOLE)
SET_WINDOWS_NO_MANIFEST(${PROJECT_NAME})
SET_WINDOWS_ENTRYPOINT(${PROJECT_NAME} wmain OFF)

# FIXME: librpbase isn't actually needed; only the headers are.
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpsecure rpbase cachecommon)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# rp-download
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# rp-download
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)
IF(WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE win32common)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE wininet advapi32)
	# Delay-load shell32.dll and ole32.dll to prevent a performance penalty due to gdi32.dll.
	# Reference: https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
	# This is also needed when disabling direct Win32k syscalls,
	# since loading gdi32.dll will crash in that case.
	# NOTE: ole32.dll is indirectly linked through libwin32common. (CoTaskMemFree())
	INCLUDE(../libwin32common/DelayLoadHelper.cmake)
	ADD_DELAYLOAD_FLAGS(${PROJECT_NAME} shell32.dll ole32.dll)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE unixcommon inih)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${CURL_LIBRARIES})
ENDIF(WIN32)

IF(APPLE)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${CORESERVICES_LIBRARY})
ENDIF(APPLE)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(${PROJECT_NAME} git_version)
ENDIF(TARGET git_version)

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
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "${DIR_INSTALL_LIBEXEC}"
	COMPONENT "plugin"
	)
IF(INSTALL_APPARMOR)
	# AppArmor profile (Linux only)
	# Convert the full path to use '.' instead of '/'.
	STRING(REPLACE "/" "." APPARMOR_CONF "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIBEXEC}/${PROJECT_NAME}")
	# Remove the leading slash if present.
	STRING(REGEX REPLACE "^\\." "" APPARMOR_CONF "${APPARMOR_CONF}")

	INSTALL(FILES ${PROJECT_NAME}.apparmor.conf
		RENAME "${APPARMOR_CONF}"
		DESTINATION "${DIR_INSTALL_APPARMOR}"
		COMPONENT "apparmor"
		)
ENDIF(INSTALL_APPARMOR)
IF(INSTALL_DEBUG)
	# Split debug file
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
	IF(DEBUG_FILENAME)
		IF(WIN32)
			SET(PDB_DEST_DIR "${DIR_INSTALL_DLL_DEBUG}")
		ELSE(WIN32)
			SET(PDB_DEST_DIR "lib/debug/${DIR_INSTALL_LIBEXEC}")
		ENDIF(WIN32)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "${PDB_DEST_DIR}"
			COMPONENT "debug"
			)
	ENDIF(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
