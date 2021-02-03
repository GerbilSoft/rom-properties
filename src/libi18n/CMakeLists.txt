# Internationalization support code.
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(libi18n LANGUAGES C)

# NOTE: This subdirectory is always built, even if NLS is disabled,
# since config.libi18n.h is included by i18n.h.
IF(ENABLE_NLS)
	# Find gettext.
	INCLUDE(CheckGettext)
	CHECK_GETTEXT()
ENDIF(ENABLE_NLS)

# Write the config.h file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libi18n.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libi18n.h")

######################
# Build the library. #
######################

IF(ENABLE_NLS)
	ADD_LIBRARY(i18n STATIC i18n.c i18n.h)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(i18n)
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(i18n PROPERTIES EXCLUDE_FROM_ALL TRUE)

	TARGET_LINK_LIBRARIES(i18n PUBLIC ${Intl_LIBRARIES})
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
