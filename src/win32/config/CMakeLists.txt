PROJECT(rp-config-win32 C)
# rp-config stub for Windows.

# Create the manifest file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/rp-config.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/rp-config.exe.manifest" @ONLY)

ADD_EXECUTABLE(rp-config WIN32 rp-config.c rp-config.rc)
SET_TARGET_PROPERTIES(rp-config PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rp-config)
SET_WINDOWS_SUBSYSTEM(rp-config WIN32)
SET_WINDOWS_NO_MANIFEST(rp-config)
# NOTE: rp_show_config_dialog() uses a WinMain-style entry point,
# so use WinMain for rp-config.exe.
SET_WINDOWS_ENTRYPOINT(rp-config WinMain OFF)

TARGET_LINK_LIBRARIES(rp-config PRIVATE rpsecure)
TARGET_LINK_LIBRARIES(rp-config PRIVATE advapi32)
TARGET_INCLUDE_DIRECTORIES(rp-config
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# build/src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# build/src/win32
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(rp-config git_version)
ENDIF(TARGET git_version)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}\"")
ENDIF(MINGW)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS rp-config
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rp-config PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
