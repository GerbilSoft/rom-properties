# Unix and Unix-like systems common code
# FIXME: CMake-3.15.5 on travis-ci can't find Objective-C...
PROJECT(unixcommon LANGUAGES C CXX)

IF(BUILD_KDE4)
	INCLUDE(RP_FindQt4andKDE4)
	FIND_QT4_AND_KDE4()
ENDIF(BUILD_KDE4)
IF(BUILD_KF5)
	INCLUDE(RP_FindQt5andKF5)
	FIND_QT5_AND_KF5()
ENDIF(BUILD_KF5)
IF(BUILD_KF6)
	INCLUDE(RP_FindQt6andKF6)
	FIND_QT6_AND_KF6()
ENDIF(BUILD_KF6)

# GTK+ UI frontends.
# NOTE: Only checking for the main plugin, not symlinked plugins.
IF(BUILD_XFCE)
	# Thunar (XFCE)
	FIND_PACKAGE(ThunarX2)
	IF(NOT ThunarX2_FOUND)
		SET(ThunarX2_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/thunarx-2")
	ENDIF(NOT ThunarX2_FOUND)
ENDIF(BUILD_XFCE)
IF(BUILD_GTK3 OR BUILD_GTK4)
	# Nautilus (GNOME)
	FIND_PACKAGE(LibNautilusExtension)	# v3
	FIND_PACKAGE(LibNautilusExtension4)	# v4
	IF(LibNautilusExtension_FOUND AND LibNautilusExtnsion4_FOUND)
		# Found both versions. (...not normally possible?)
	ELSEIF(LibNautilusExtension_FOUND)
		# Found v3 but not v4. Use defaults for v4.
		SET(LibNautilusExtension3_EXTENSION_DIR "${LibNautilusExtension_EXTENSION_DIR}")
		STRING(REPLACE "extensions-3.0" "extensions-4" LibNautilusExtension4_EXTENSION_DIR "${LibNautilusExtension_EXTENSION_DIR}")
	ELSEIF(LibNautilusExtension4_FOUND)
		# Found v4 but not v3. Use defaults for v3.
		STRING(REPLACE "extensions-4" "extensions-3.0" LibNautilusExtension3_EXTENSION_DIR "${LibNautilusExtension4_EXTENSION_DIR}")
	ELSE()
		# Neither v3 nor v4 were found.
		MESSAGE(STATUS "LibNautilusExtension not found. Using the default extensions directory.")
		SET(LibNautilusExtension3_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-3.0")
		SET(LibNautilusExtension4_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-4")
	ENDIF()
ENDIF(BUILD_GTK3 OR BUILD_GTK4)

# Check for C library functions.
INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(getpwuid_r "pwd.h" HAVE_GETPWUID_R)
IF(NOT HAVE_GETPWUID_R)
	CHECK_SYMBOL_EXISTS(getpwuid "pwd.h" HAVE_GETPWUID)
ENDIF(NOT HAVE_GETPWUID_R)
IF(NOT HAVE_GETPWUID_R AND NOT HAVE_GETPWUID)
	MESSAGE(FATAL_ERROR "Neither getpwuid_r() nor getpwuid() were found.")
ENDIF(NOT HAVE_GETPWUID_R AND NOT HAVE_GETPWUID)

# Check for statx().
SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
CHECK_SYMBOL_EXISTS(statx "sys/stat.h" HAVE_STATX)
UNSET(CMAKE_REQUIRED_DEFINITIONS)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

# Sources and headers.
IF(APPLE)
	SET(${PROJECT_NAME}_SRCS userdirs_mac.mm)
	SET(${PROJECT_NAME}_H userdirs.hpp)
ELSE(APPLE)
	SET(${PROJECT_NAME}_SRCS userdirs.cpp dll-search.c)
	SET(${PROJECT_NAME}_H userdirs.hpp dll-search.h)
ENDIF(APPLE)

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
		${${PROJECT_NAME}_SRCS}
		${${PROJECT_NAME}_H}
		)
	INCLUDE(SetMSVCDebugPath)
	SET_MSVC_DEBUG_PATH(${_target})
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)

	# DLL targets need the appropriate macros.
	IF(${_target} MATCHES "-dll$")
		TARGET_COMPILE_DEFINITIONS(${_target}
			PUBLIC		RP_BUILDING_FOR_DLL=1
			PRIVATE		RP_BUILDING_LIBROMDATA=1
			)
	ENDIF()

	IF(APPLE)
		TARGET_LINK_LIBRARIES(${_target} PUBLIC "-framework Foundation")
	ENDIF(APPLE)

	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
		)
ENDFOREACH(_target)

IF(NOT TARGET ${PROJECT_NAME}-dll)
	# On other platforms, the dll target will be an alias to the regular target.
	ADD_LIBRARY(${PROJECT_NAME}-dll ALIAS ${PROJECT_NAME})
ENDIF()

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
# Qt5 requires "-fpic -fPIC" due to reduced relocations.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
