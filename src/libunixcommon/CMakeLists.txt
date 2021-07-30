# Unix and Unix-like systems common code
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
# FIXME: CMake-3.15.5 on travis-ci can't find Objective-C...
PROJECT(unixcommon LANGUAGES C CXX)

# Qt5 requires "-fpic -fPIC" due to reduced relocations.
# This is also needed in order to link the static library
# into a shared library.
SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fpic -fPIC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fPIC")

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

# Find the qtpaths executable.
IF(BUILD_KF5)
	SET(ENV{QT_SELECT} qt5)
	FIND_PROGRAM(QTPATHS qtpaths)
	IF(NOT QTPATHS)
		MESSAGE(FATAL_ERROR "qtpaths not found. Install one of these packages:
  - Debian/Ubuntu: qttools5-dev-tools
  - Red Hat/Fedora: qt5-qttools")
	ENDIF(NOT QTPATHS)

	# Get the plugin directory and Qt prefix.
	# Prefix will be removed from the plugin directory if necessary.
	EXEC_PROGRAM(${QTPATHS} ARGS --plugin-dir OUTPUT_VARIABLE KF5_PLUGIN_INSTALL_DIR)
	IF(NOT KF5_PLUGIN_INSTALL_DIR)
		MESSAGE(FATAL_ERROR "`qtpaths` isn't working correctly.")
	ENDIF(NOT KF5_PLUGIN_INSTALL_DIR)
	EXEC_PROGRAM(${QTPATHS} ARGS --install-prefix OUTPUT_VARIABLE QT5_INSTALL_PREFIX)
	INCLUDE(ReplaceHardcodedPrefix)
	REPLACE_HARDCODED_PREFIX(KF5_PLUGIN_INSTALL_DIR "${QT5_INSTALL_PREFIX}")
	SET(ENV{QT_SELECT} ${QT_SELECT_OLD})
	UNSET(QT_SELECT_OLD)

	SET(KF5_PRPD_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/propertiesdialog")
	SET(KF5_KFMD_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/kfilemetadata")
	SET(KF5_KOVI_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/overlayicon")
ENDIF(BUILD_KF5)

# GTK+ UI frontends.
# NOTE: Only checking for the main plugin, not symlinked plugins.
IF(BUILD_GTK2)
	# Thunar (XFCE)
	FIND_PACKAGE(ThunarX2)
	IF(NOT ThunarX2_FOUND)
		SET(ThunarX2_EXTENSIONS_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/thunarx-2")
	ENDIF(NOT ThunarX2_FOUND)
ENDIF(BUILD_GTK2)
IF(BUILD_GTK3)
	# Nautilus (GNOME)
	FIND_PACKAGE(LibNautilusExtension 3.0.0)
	IF(NOT LibNautilusExtension_FOUND)
		SET(LibNautilusExtension_EXTENSION_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/nautilus/extensions-3.0")
	ENDIF(NOT LibNautilusExtension_FOUND)
ENDIF(BUILD_GTK3)

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

ADD_LIBRARY(${PROJECT_NAME} STATIC
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(${PROJECT_NAME})
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
IF(APPLE)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC "-framework Foundation")
ENDIF(APPLE)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
	)
