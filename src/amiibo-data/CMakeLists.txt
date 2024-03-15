# amiibo database data and compiler.
PROJECT(amiibo-data LANGUAGES CXX)

SET(amiiboc_SRCS amiiboc.cpp)

#########################
# Build the executable. #
#########################

INCLUDE(SetMSVCDebugPath)

IF(CMAKE_CROSSCOMPILING)
	# Cross-compiling.
	# Use executables from a native build.
	IF(NOT IMPORT_EXECUTABLES)
		MESSAGE(FATAL_ERROR "IMPORT_EXECUTABLES is not defined.
Set IMPORT_EXECUTABLES to the path to a CMake build for the host system's build environment.")
	ENDIF(NOT IMPORT_EXECUTABLES)
	INCLUDE("${IMPORT_EXECUTABLES}/src/amiibo-data/ImportExecutables.cmake")
ELSE(CMAKE_CROSSCOMPILING)
	# Not cross-compiling.
	ADD_EXECUTABLE(amiiboc ${amiiboc_SRCS})
	DO_SPLIT_DEBUG(amiiboc)
	SET_WINDOWS_SUBSYSTEM(amiiboc CONSOLE)
	#SET_WINDOWS_NO_MANIFEST(amiiboc)
	SET_WINDOWS_ENTRYPOINT(amiiboc wmain OFF)

	TARGET_LINK_LIBRARIES(amiiboc PRIVATE rpsecure rpbyteswap)
	TARGET_INCLUDE_DIRECTORIES(amiiboc
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>          # amiibo-data
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>          # amiibo-data
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>       # src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>       # src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>                  # build
		)

	# Make sure git_version.h is created before compiling this target.
	IF(TARGET git_version)
		ADD_DEPENDENCIES(amiiboc git_version)
	ENDIF(TARGET git_version)

	# CMake-3.7.2 doesn't add include paths to windres.
	IF(MINGW)
		SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
		SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
		SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}\"")
	ENDIF(MINGW)

        # Export these executables for cross-compiling.
        # Reference: http://www.cmake.org/Wiki/CMake_Cross_Compiling
        EXPORT(TARGETS amiiboc
                FILE "${CMAKE_CURRENT_BINARY_DIR}/ImportExecutables.cmake"
                )
ENDIF(CMAKE_CROSSCOMPILING)

# Compile amiibo-data.txt into amiibo-data.bin.
SET(AMIIBO_DATA_BIN "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/amiibo-data.bin")
SET(AMIIBO_DATA_TXT "${CMAKE_CURRENT_SOURCE_DIR}/amiibo-data.txt")
ADD_CUSTOM_COMMAND(
	OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/amiibo-data.bin"
	COMMAND "$<TARGET_FILE:amiiboc>" "${AMIIBO_DATA_TXT}" "${AMIIBO_DATA_BIN}"
	DEPENDS amiiboc "${AMIIBO_DATA_TXT}"
	VERBATIM
	)
ADD_CUSTOM_TARGET(amiibo-data ALL
	DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/amiibo-data.bin"
	)

INCLUDE(DirInstallPaths)
INSTALL(FILES "${AMIIBO_DATA_BIN}"
	DESTINATION "${DIR_INSTALL_SHARE}"
	COMPONENT "data"
	)
