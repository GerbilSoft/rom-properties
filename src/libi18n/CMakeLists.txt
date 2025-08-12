# Internationalization support code.
PROJECT(i18n LANGUAGES CXX)

# NOTE: This subdirectory is always built, even if NLS is disabled,
# since config.libi18n.h is included by i18n.hpp.
IF(ENABLE_NLS)
	# Find gettext.
	INCLUDE(CheckGettext)
	CHECK_GETTEXT()
ENDIF(ENABLE_NLS)

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

IF(NOT ENABLE_NLS)
	# NLS is disabled. Don't build anything.
	UNSET(TARGETS)
ENDIF(NOT ENABLE_NLS)

FOREACH(_target ${TARGETS})
	ADD_LIBRARY(${_target} STATIC i18n.cpp i18n.hpp)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds; enable PIC.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		POSITION_INDEPENDENT_CODE TRUE
		)

	# DLL targets need the appropriate macros.
	IF(${_target} MATCHES "-dll$")
		SET(_target_suffix "-dll")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ELSE()
		UNSET(_target_suffix)
	ENDIF()

	IF(Intl_LIBRARY)
		TARGET_LINK_LIBRARIES(${_target} PUBLIC ${Intl_LIBRARY})
	ENDIF(Intl_LIBRARY)
	TARGET_LINK_LIBRARIES(${_target} PRIVATE rpthreads${_target_suffix})
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
			${Intl_INCLUDE_DIR}
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		)
ENDFOREACH(_target)
UNSET(_target_suffix)

IF(TARGET ${PROJECT_NAME} AND NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()
