# Stub executable for Unix and Unix-like thumbnailers
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rp-stub LANGUAGES C)

# rp-stub
ADD_EXECUTABLE(rp-stub rp-stub.c rp-stub_secure.c rp-stub_secure.h)
DO_SPLIT_DEBUG(rp-stub)
TARGET_INCLUDE_DIRECTORIES(rp-stub
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# rp-stub
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# rp-stub
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(rp-stub git_version)
ENDIF(TARGET git_version)

# dll-search.c is in libunixcommon.
TARGET_LINK_LIBRARIES(rp-stub PRIVATE unixcommon rpsecure)

IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rp-stub PRIVATE i18n)
ENDIF(ENABLE_NLS)

# Link in libdl if it's required for dlopen().
IF(CMAKE_DL_LIBS)
	TARGET_LINK_LIBRARIES(rp-stub PRIVATE ${CMAKE_DL_LIBS})
ENDIF(CMAKE_DL_LIBS)

###########################
# Install the executable. #
###########################

INCLUDE(DirInstallPaths)
INSTALL(TARGETS rp-stub
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	COMPONENT "plugin"
	)

# FIXME: Create symlinks for rp-thumbnail and rp-config.
# CMake doesn't really like this, and in the Debian package build,
# it results in errors due to attempting to create a symlink in
# the real file system instead of the fakeroot.
INSTALL(PROGRAMS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rp-stub"
	DESTINATION "${DIR_INSTALL_LIBEXEC}"
	RENAME "rp-thumbnail"
	COMPONENT "plugin"
	)
INSTALL(PROGRAMS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rp-stub"
	DESTINATION "${DIR_INSTALL_EXE}"
	RENAME "rp-config"
	COMPONENT "plugin"
	)

# Check if a split debug file should be installed.
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rp-stub PDB)
	IF(DEBUG_FILENAME)
		# TODO: Uncomment this once symlinking is fixed.
		#INSTALL(FILES "${DEBUG_FILENAME}"
		#	DESTINATION "lib/debug/${DIR_INSTALL_EXE}"
		#	COMPONENT "debug"
		#	)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "lib/debug/${DIR_INSTALL_EXE}"
			RENAME "rp-thumbnail.debug"
			COMPONENT "debug"
			)
		INSTALL(FILES "${DEBUG_FILENAME}"
			DESTINATION "lib/debug/${DIR_INSTALL_EXE}"
			RENAME "rp-config.debug"
			COMPONENT "debug"
			)
	ENDIF(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
