PROJECT(rp-stub)

# Find the framework libraries in order to determine plugin paths.
FIND_PACKAGE(LibNautilusExtension 3.0.0)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.rp-stub.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.rp-stub.h")

# rp-stub
ADD_EXECUTABLE(rp-stub rp-stub.c)
DO_SPLIT_DEBUG(rp-stub)
TARGET_INCLUDE_DIRECTORIES(rp-stub
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
	)

# Link in libdl if it's required for dlopen().
IF(CMAKE_DL_LIBS)
	TARGET_LINK_LIBRARIES(rp-stub ${CMAKE_DL_LIBS})
ENDIF(CMAKE_DL_LIBS)

###########################
# Install the executable. #
###########################

INCLUDE(DirInstallPaths)
INSTALL(TARGETS rp-stub
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	COMPONENT "plugin"
	)

# Create symlinks for rp-thumbnail and rp-config.
# Reference: https://cmake.org/pipermail/cmake/2010-April/036504.html
SET(RP_STUB_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_EXE}/rp-stub")
SET(RP_THUMBNAIL_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_LIBEXEC}/rp-thumbnail")
SET(RP_CONFIG_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_EXE}/rp-config")
INSTALL(CODE "
	EXECUTE_PROCESS(COMMAND ln -sf \"${RP_STUB_FULL_PATH}\" \"${RP_THUMBNAIL_FULL_PATH}\")
	EXECUTE_PROCESS(COMMAND ln -sf \"${RP_STUB_FULL_PATH}\" \"${RP_CONFIG_FULL_PATH}\")
	")

# Check if a split debug file should be installed.
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rp-stub PDB)
	IF(DEBUG_FILENAME)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "lib/debug/${DIR_INSTALL_EXE}"
			COMPONENT "debug"
			)
	ENDIF(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
