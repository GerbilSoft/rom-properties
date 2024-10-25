# Stub executable for Unix and Unix-like thumbnailers
PROJECT(rp-stub LANGUAGES C)

# rp-stub
ADD_EXECUTABLE(${PROJECT_NAME} rp-stub.c rp-stub_secure.c rp-stub_secure.h)
DO_SPLIT_DEBUG(${PROJECT_NAME})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# rp-stub
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# rp-stub
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(${PROJECT_NAME} git_version)
ENDIF(TARGET git_version)

# dll-search.c is in libunixcommon.
# strlcat() is in librptext.
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE unixcommon rptext rpsecure)

IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE i18n)
ENDIF(ENABLE_NLS)

# Link in libdl if it's required for dlopen().
IF(CMAKE_DL_LIBS)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
ENDIF(CMAKE_DL_LIBS)

###########################
# Install the executable. #
###########################

INCLUDE(DirInstallPaths)

# Create symlinks for rp-thumbnail and rp-config.
# Reference: https://stackoverflow.com/questions/34578362/how-can-i-package-a-symlink-with-cpack
IF(ENABLE_NIXOS)
	# NixOS ends up with a double-path issue if CMAKE_INSTALL_PREFIX is specified here.
	SET(SYMLINK_SRC "${DIR_INSTALL_EXE}/${PROJECT_NAME}")
ELSE(ENABLE_NIXOS)
	SET(SYMLINK_SRC "${CMAKE_INSTALL_PREFIX}/${DIR_INSTALL_EXE}/${PROJECT_NAME}")
ENDIF(ENABLE_NIXOS)
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ln -sf "${SYMLINK_SRC}" rp-thumbnail
	WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
	VERBATIM
	)
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ln -sf ${PROJECT_NAME} rp-config
	WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
	VERBATIM
	)

# Install the main target and the symlinks.
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	COMPONENT "plugin"
	)
INSTALL(FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rp-thumbnail"
	DESTINATION "${DIR_INSTALL_LIBEXEC}"
	COMPONENT "plugin"
	)
INSTALL(PROGRAMS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rp-config"
	DESTINATION "${DIR_INSTALL_EXE}"
	COMPONENT "plugin"
	)

# Check if a split debug file should be installed.
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
	IF(DEBUG_FILENAME)
		# TODO: Uncomment this once symlinking is fixed.
		#INSTALL(FILES "${DEBUG_FILENAME}"
		#	DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		#	COMPONENT "debug"
		#	)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
			RENAME "rp-thumbnail.debug"
			COMPONENT "debug"
			)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
			RENAME "rp-config.debug"
			COMPONENT "debug"
			)
	ENDIF(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
