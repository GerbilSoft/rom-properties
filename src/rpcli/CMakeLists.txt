# Command line interface
PROJECT(rpcli LANGUAGES C CXX)

# Sources and headers
SET(${PROJECT_NAME}_SRCS
	rpcli.cpp
	device.cpp
	rpcli_secure.c
	)
SET(${PROJECT_NAME}_H
	device.hpp
	rpcli_secure.h
	)

# Sixel (TODO: Check forlibsixel)
# EXPERIMENTAL: sixel output isn't fully implemented yet.
IF(ENABLE_SIXEL AND NOT WIN32)
	SET(${PROJECT_NAME}_SIXEL_SRCS rp_sixel.cpp)
	SET(${PROJECT_NAME}_SIXEL_H    rp_sixel.hpp)
ENDIF(ENABLE_SIXEL AND NOT WIN32)

# Check for system security functionality.
IF(WIN32)
	SET(${PROJECT_NAME}_RC resource.rc)

	# Create the manifest file.
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.exe.manifest" @ONLY)
ENDIF(WIN32)

IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_CRYPTO_SRCS verifykeys.cpp)
	SET(${PROJECT_NAME}_CRYPTO_H verifykeys.hpp)
ENDIF(ENABLE_DECRYPTION)

IF(MSVC)
	# Delay-load sources.
	SET(${PROJECT_NAME}-DELAYLOAD_SRC ../libwin32common/DelayLoadHelper.c)
	SET(${PROJECT_NAME}-DELAYLOAD_H   ../libwin32common/DelayLoadHelper.h)
ENDIF(MSVC)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.${PROJECT_NAME}.h")

#########################
# Build the executable. #
#########################

# Delay-load extlibs.
INCLUDE(../libwin32common/DelayLoadHelper.cmake)
SET_DELAYLOAD_EXTLIB_FLAGS()

IF(RP_LIBROMDATA_IS_DLL)
	# Delay-load libromdata in order to allow usage of
	# architecture-specific subdirectories.
	# TODO: MinGW version.
	IF(MSVC)
		GET_TARGET_PROPERTY(ROMDATA_DEBUG_POSTFIX   romdata DEBUG_POSTFIX)
		GET_TARGET_PROPERTY(ROMDATA_RELEASE_POSTFIX romdata RELEASE_POSTFIX)
		# FIXME: This doesn't seem to work...
		# TODO: Global variable with the libromdata SOVERSION?
		#SET_PROPERTY(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG   " /DELAYLOAD:romdata${ROMDATA_DEBUG_POSTFIX}")
		#SET_PROPERTY(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE " /DELAYLOAD:romdata${ROMDATA_RELEASE_POSTFIX}")
		SET(CMAKE_EXE_LINKER_FLAGS_DEBUG   "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DELAYLOAD:romdata${ROMDATA_DEBUG_POSTFIX}.dll")
		SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DELAYLOAD:romdata${ROMDATA_RELEASE_POSTFIX}.dll")
	ENDIF(MSVC)
ENDIF(RP_LIBROMDATA_IS_DLL)

ADD_EXECUTABLE(${PROJECT_NAME}
	${${PROJECT_NAME}-DELAYLOAD_SRC} ${${PROJECT_NAME}-DELAYLOAD_H}
	${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_SIXEL_SRCS} ${${PROJECT_NAME}_SIXEL_H}
	${${PROJECT_NAME}_RC}
	${${PROJECT_NAME}_CRYPTO_SRCS} ${${PROJECT_NAME}_CRYPTO_H}
	)
IF(ENABLE_PCH)
	TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} PRIVATE
		"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
		stdafx.h
		)
ENDIF(ENABLE_PCH)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(${PROJECT_NAME})
SET_WINDOWS_SUBSYSTEM(${PROJECT_NAME} CONSOLE)
SET_WINDOWS_NO_MANIFEST(${PROJECT_NAME})
# FIXME: setargv isn't necessarily sorted...
# TODO: Implement our own setargv function?
SET_WINDOWS_ENTRYPOINT(${PROJECT_NAME} wmain ON)

IF(RP_LIBROMDATA_IS_DLL)
	# libromdata is being built as a DLL.
	# Need to enable dllimports.
	# FIXME: This *should* be inherited by linking to romdata...
	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE RP_BUILDING_FOR_DLL=1)
ENDIF(RP_LIBROMDATA_IS_DLL)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# rpcli
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# rpcli
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpsecure romdata)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE gsvt)
IF(ENABLE_SIXEL AND NOT WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE sixel)
ENDIF(ENABLE_SIXEL AND NOT WIN32)

IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)

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

# Delay-load shell32.dll and ole32.dll to prevent a performance penalty due to gdi32.dll.
# Reference: https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
# This is also needed when disabling direct Win32k syscalls,
# since loading gdi32.dll will crash in that case.
# NOTE: ole32.dll is indirectly linked through libwin32common. (CoTaskMemFree())
# FIXME: gdiplus.dll isn't linked in, though I'm pretty sure it's used...
ADD_DELAYLOAD_FLAGS(${PROJECT_NAME} shell32.dll ole32.dll)
IF(MSVC)
	# Extended delay-load functionality.
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE delayimp)
ENDIF(MSVC)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
# FIXME: Not working properly. (cannot write files to the user's home directory)
IF(0)
IF(INSTALL_APPARMOR)
	# AppArmor profile (Linux only)
	# Convert the full path to use '.' instead of '/'.
	STRING(REPLACE "/" "." APPARMOR_CONF "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_EXE}/${PROJECT_NAME}")
	# Remove the leading slash if present.
	STRING(REGEX REPLACE "^\\." "" APPARMOR_CONF "${APPARMOR_CONF}")

	INSTALL(FILES ${PROJECT_NAME}.apparmor.conf
		RENAME "${APPARMOR_CONF}"
		DESTINATION "${DIR_INSTALL_APPARMOR}"
		COMPONENT "apparmor"
		)
	INSTALL(FILES ${PROJECT_NAME}.local.apparmor.conf
		RENAME "${APPARMOR_CONF}"
		DESTINATION "${DIR_INSTALL_APPARMOR}/local"
		COMPONENT "apparmor"
		)
ENDIF(INSTALL_APPARMOR)
ENDIF(0)
IF(INSTALL_DEBUG)
	# Split debug file
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
