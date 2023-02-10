# Unix and Unix-like systems common code
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
# FIXME: CMake-3.15.5 on travis-ci can't find Objective-C...
PROJECT(unixcommon LANGUAGES C CXX)

IF(BUILD_KDE4)
	# Find the framework libraries in order to determine plugin paths.
	# FIXME: Remove prefix from KDE4's path.
	# NOTE: Need to skip the uninstall rule due to conflicts
	# with the actual KDE4 frontend.
	SET(QT_SELECT_OLD $ENV{QT_SELECT})
	SET(ENV{QT_SELECT} qt4)
	SET(_kde4_uninstall_rule_created TRUE)
	FIND_PACKAGE(KDE4)
	SET(KDE4_PLUGIN_INSTALL_DIR "${PLUGIN_INSTALL_DIR}")
	UNSET(PLUGIN_INSTALL_DIR)
	UNSET(PLUGIN_INSTALL_DIR CACHE)
	# Get rid of the explicit C90 setting. (set by KDE4)
	STRING(REPLACE "-std=iso9899:1990" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
ENDIF(BUILD_KDE4)

IF(BUILD_KF5)
	# Find the qtpaths5 executable for Qt5.
	SET(QT_SELECT_OLD "$ENV{QT_SELECT}")
	SET(ENV{QT_SELECT} qt5)
	FIND_PROGRAM(QTPATHS5 qtpaths5 qtpaths)
	IF(NOT QTPATHS5)
		MESSAGE(FATAL_ERROR "qtpaths5 not found. Install one of these packages:
  - Debian/Ubuntu: qttools5-dev-tools
  - Red Hat/Fedora: qt5-qttools")
	ENDIF(NOT QTPATHS5)

	# Get the plugin directory and Qt prefix.
	# Prefix will be removed from the plugin directory if necessary.
	EXEC_PROGRAM(${QTPATHS5} ARGS --plugin-dir OUTPUT_VARIABLE KF5_PLUGIN_INSTALL_DIR)
	IF(NOT KF5_PLUGIN_INSTALL_DIR)
		MESSAGE(FATAL_ERROR "`qtpaths5` is not working correctly.")
	ENDIF(NOT KF5_PLUGIN_INSTALL_DIR)
	EXEC_PROGRAM(${QTPATHS5} ARGS --install-prefix OUTPUT_VARIABLE QT5_INSTALL_PREFIX)
	INCLUDE(ReplaceHardcodedPrefix)
	REPLACE_HARDCODED_PREFIX(KF5_PLUGIN_INSTALL_DIR "${QT5_INSTALL_PREFIX}")
	SET(ENV{QT_SELECT} "${QT_SELECT_OLD}")
	UNSET(QT_SELECT_OLD)

	SET(KF5_PRPD_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/propertiesdialog")
ENDIF(BUILD_KF5)

IF(BUILD_KF6)
	# Find the qtpaths6 executable for Qt6.
	SET(QT_SELECT_OLD "$ENV{QT_SELECT}")
	SET(ENV{QT_SELECT} qt6)
	FIND_PROGRAM(QTPATHS6 qtpaths6 qtpaths)
	IF(NOT QTPATHS6)
		MESSAGE(FATAL_ERROR "qtpaths6 not found. Install one of these packages:
  - Debian/Ubuntu: qttools6-dev-tools
  - Red Hat/Fedora: qt6-qttools")
	ENDIF(NOT QTPATHS6)

	# Get the plugin directory and Qt prefix.
	# Prefix will be removed from the plugin directory if necessary.
	EXEC_PROGRAM(${QTPATHS6} ARGS --plugin-dir OUTPUT_VARIABLE KF6_PLUGIN_INSTALL_DIR)
	IF(NOT KF6_PLUGIN_INSTALL_DIR)
		MESSAGE(FATAL_ERROR "`qtpaths6` is not working correctly.")
	ENDIF(NOT KF6_PLUGIN_INSTALL_DIR)
	EXEC_PROGRAM(${QTPATHS6} ARGS --install-prefix OUTPUT_VARIABLE QT6_INSTALL_PREFIX)
	INCLUDE(ReplaceHardcodedPrefix)
	REPLACE_HARDCODED_PREFIX(KF6_PLUGIN_INSTALL_DIR "${QT6_INSTALL_PREFIX}")
	SET(ENV{QT_SELECT} "${QT_SELECT_OLD}")
	UNSET(QT_SELECT_OLD)

	SET(KF6_PRPD_PLUGIN_INSTALL_DIR "${KF6_PLUGIN_INSTALL_DIR}/kf6/propertiesdialog")
ENDIF(BUILD_KF6)

# GTK+ UI frontends.
# NOTE: Only checking for the main plugin, not symlinked plugins.
IF(BUILD_GTK2)
	# Thunar (XFCE)
	FIND_PACKAGE(ThunarX2)
	IF(NOT ThunarX2_FOUND)
		SET(ThunarX2_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/thunarx-2")
	ENDIF(NOT ThunarX2_FOUND)
ENDIF(BUILD_GTK2)
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
SET(OLD_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
CHECK_SYMBOL_EXISTS(statx "sys/stat.h" HAVE_STATX)
SET(CMAKE_REQUIRED_DEFINITIONS "${OLD_CMAKE_REQUIRED_DEFINITIONS}")
UNSET(OLD_CMAKE_REQUIRED_DEFINITIONS)

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
