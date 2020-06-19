# Command line interface
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(rpcli LANGUAGES C CXX)

IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

# Sources and headers.
SET(rpcli_SRCS
	rpcli.cpp
	properties.cpp
	device.cpp
	rpcli_secure.c
	)
SET(rpcli_H
	properties.hpp
	device.hpp
	rpcli_secure.h
	)

# Check for system security functionality.
IF(WIN32)
	SET(rpcli_RC resource.rc)

	# Create the manifest file.
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/rpcli.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/rpcli.exe.manifest" @ONLY)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(rpcli_CRYPTO_SRCS verifykeys.cpp)
	SET(rpcli_CRYPTO_H verifykeys.hpp)
ENDIF(ENABLE_DECRYPTION)

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(rpcli_PCH_C stdafx.c)
	SET(rpcli_PCH_CXX stdafx.cpp)
	SET(rpcli_PCH_SRC ${rpcli_PCH_C} ${rpcli_PCH_CXX})
	SET(rpcli_PCH_H stdafx.h)
ENDIF(ENABLE_PCH)

IF(MSVC)
	# Delay-load sources.
	SET(rpcli-DELAYLOAD_SRC ../libwin32common/DelayLoadHelper.c)
	SET(rpcli-DELAYLOAD_H ../libwin32common/DelayLoadHelper.h)
ENDIF(MSVC)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.rpcli.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.rpcli.h")

#########################
# Build the executable. #
#########################

# Delay-load extlibs.
INCLUDE(../libwin32common/DelayLoadHelper.cmake)
SET_DELAYLOAD_EXTLIB_FLAGS()

ADD_EXECUTABLE(rpcli
	${rpcli_PCH_SRC} ${rpcli_PCH_H}
	${rpcli-DELAYLOAD_SRC} ${rpcli-DELAYLOAD_H}
	${rpcli_SRCS} ${rpcli_H}
	${rpcli_CRYPTO_SRCS} ${rpcli_CRYPTO_H}
	${rpcli_RC}
	)
IF(ENABLE_PCH)
	ADD_PRECOMPILED_HEADER(rpcli ${rpcli_PCH_H}
		SOURCE_C ${rpcli_PCH_C}
		SOURCE_CXX ${rpcli_PCH_CXX}
		)
ENDIF(ENABLE_PCH)
SET_TARGET_PROPERTIES(rpcli PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rpcli)
SET_WINDOWS_SUBSYSTEM(rpcli CONSOLE)
SET_WINDOWS_NO_MANIFEST(rpcli)
# FIXME: setargv isn't necessarily sorted...
# TODO: Implement our own setargv function?
SET_WINDOWS_ENTRYPOINT(rpcli wmain ON)
# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rpcli
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# rpcli
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# rpcli
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(rpcli PRIVATE rpsecure romdata rpfile rpbase)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rpcli PRIVATE i18n)
ENDIF(ENABLE_NLS)
IF(WIN32)
	TARGET_LINK_LIBRARIES(rpcli PRIVATE wmain)
ENDIF(WIN32)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(rpcli git_version)
ENDIF(TARGET git_version)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}\"")
ENDIF(MINGW)

# Delay-load shell32.dll and ole32.dll to prevent a performance penalty due to gdi32.dll.
# Reference: https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
# This is also needed when disabling direct Win32k syscalls,
# since loading gdi32.dll will crash in that case.
# NOTE: ole32.dll is indirectly linked through libwin32common. (CoTaskMemFree())
# FIXME: gdiplus.dll isn't linked in, though I'm pretty sure it's used...
ADD_DELAYLOAD_FLAGS(rpcli shell32.dll ole32.dll)
IF(MSVC)
	# Extended delay-load functionality.
	TARGET_LINK_LIBRARIES(rpcli PRIVATE delayimp)
ENDIF(MSVC)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS rpcli
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_APPARMOR)
	# AppArmor profile (Linux only)
	# Convert the full path to use '.' instead of '/'.
	STRING(REPLACE "/" "." APPARMOR_CONF "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_EXE}/rpcli")
	# Remove the leading slash if present.
	STRING(REGEX REPLACE "^\\." "" APPARMOR_CONF "${APPARMOR_CONF}")

	INSTALL(FILES rpcli.apparmor.conf
		RENAME "${APPARMOR_CONF}"
		DESTINATION "${DIR_INSTALL_APPARMOR}"
		COMPONENT "apparmor"
		)
ENDIF(INSTALL_APPARMOR)
IF(INSTALL_DEBUG)
	# Split debug file
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rpcli PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
