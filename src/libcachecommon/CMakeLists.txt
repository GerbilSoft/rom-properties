# Common cache management functions used by both Cache Manager and rp-download.
PROJECT(cachecommon LANGUAGES CXX)

SET(${PROJECT_NAME}_SRCS
	CacheKeys.cpp
	CacheDir.cpp
	)
SET(${PROJECT_NAME}_H
	CacheKeys.hpp
	CacheDir.hpp
	)

# Write the config.h file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

######################
# Build the library. #
######################

# NOTE: ${PROJECT_NAME} is the original target.
# ${PROJECT_NAME}-dll is the target with dllimport/dllexport for libromdata.
IF(RP_LIBROMDATA_IS_DLL AND WIN32)
	# On Win32, the dll target needs dllimport/dllexport.
	SET(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-dll)
ELSE()
	# On other platforms, the dll target will be an alias to the regular target.
	# That will be set up later, since the regular target doesn't exist yet.
	SET(TARGETS ${PROJECT_NAME})
ENDIF()

FOREACH(_target ${TARGETS})
	ADD_LIBRARY(${_target} STATIC
		${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
		)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)

	# DLL targets need the appropriate macros.
	TARGET_COMPILE_DEFINITIONS(${_target} PRIVATE RP_BUILDING_LIBROMDATA=1)
	IF(${_target} MATCHES "-dll$")
		SET(_target_suffix "-dll")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ELSE()
		UNSET(_target_suffix)
	ENDIF()

	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		)
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpthreads${_target_suffix})
	IF(WIN32)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${_target_suffix})
	ELSE(WIN32)
		TARGET_LINK_LIBRARIES(${_target} PRIVATE ${_target_suffix})
	ENDIF(WIN32)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

# Unix: Add -fpic/-fPIC in order to use these static libraries in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
