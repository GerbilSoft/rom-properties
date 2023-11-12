# Windows DLL registration frontend
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(svrplus LANGUAGES C)

# Create the manifest file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.exe.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.exe.manifest" @ONLY)

# Sources and headers.
SET(${PROJECT_NAME}_SRCS svrplus.c)
SET(${PROJECT_NAME}_H resource.h gdipmini.h)

SET(${PROJECT_NAME}_RC
	resource.rc
	res-en_US.rc
	res-de_DE.rc
	res-fr_FR.rc
	)

#########################
# Build the executable. #
#########################

# Use the static CRT.
INCLUDE(ConfigureMSVCRuntime)
CONFIGURE_MSVC_RUNTIME(static MinSizeRel Release RelWithDebInfo)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H} ${${PROJECT_NAME}_RC})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(${PROJECT_NAME})
SET_WINDOWS_SUBSYSTEM(${PROJECT_NAME} WIN32)
SET_WINDOWS_NO_MANIFEST(${PROJECT_NAME})
SET_WINDOWS_ENTRYPOINT(${PROJECT_NAME} wWinMain OFF)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# svrplus
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# svrplus
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE win32darkmode)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpsecure)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE comctl32)

IF(MSVC)
	# Delay-load gdiplus.dll and uxtheme.dll.
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG	"${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DELAYLOAD:gdiplus.dll /DELAYLOAD:uxtheme.dll")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE	"${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DELAYLOAD:gdiplus.dll /DELAYLOAD:uxtheme.dll")
ENDIF(MSVC)

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
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
