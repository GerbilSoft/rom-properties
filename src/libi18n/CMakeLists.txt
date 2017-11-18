# Internationalization support code.
PROJECT(libi18n C)

# NOTE: This subdirectory is always built, even if NLS is disabled,
# since config.libi18n.h is included by i18n.h.

IF(ENABLE_NLS)
	# Find gettext.
	# Intl == runtime library
	# Gettext == compile-time tools
	FIND_PACKAGE(Intl REQUIRED)
	FIND_PACKAGE(Gettext REQUIRED)
	SET(HAVE_GETTEXT 1)
ENDIF(ENABLE_NLS)

# Write the config.h file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libi18n.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libi18n.h")

######################
# Build the library. #
######################

IF(ENABLE_NLS)
	ADD_LIBRARY(i18n STATIC i18n.c i18n.h)
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(i18n PROPERTIES EXCLUDE_FROM_ALL TRUE)

	TARGET_LINK_LIBRARIES(i18n ${Intl_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(i18n
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
			${Intl_INCLUDE_DIR}
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		)
ENDIF(ENABLE_NLS)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
