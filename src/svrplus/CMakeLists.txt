PROJECT(svrplus C)

# Create the manifest file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/svrplus.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/svrplus.exe.manifest" @ONLY)

# Sources and headers.
SET(svrplus_SRCS svrplus.c resource.rc)
SET(svrplus_H resource.h)

#########################
# Build the executable. #
#########################

# Use the static CRT.
INCLUDE(ConfigureMSVCRuntime)
CONFIGURE_MSVC_RUNTIME(static MinSizeRel Release RelWithDebInfo)

ADD_EXECUTABLE(svrplus ${svrplus_SRCS} ${svrplus_H})
SET_TARGET_PROPERTIES(svrplus PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(svrplus)
SET_WINDOWS_SUBSYSTEM(svrplus WIN32)
SET_WINDOWS_NO_MANIFEST(svrplus)
SET_WINDOWS_ENTRYPOINT(svrplus wWinMain OFF)

TARGET_LINK_LIBRARIES(svrplus comctl32)
TARGET_INCLUDE_DIRECTORIES(svrplus
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# svrplus
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# svrplus
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_BINARY_DIR}\"")
ENDIF(MINGW)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS svrplus
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME svrplus PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
