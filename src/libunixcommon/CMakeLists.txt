# Unix common code.
PROJECT(libunixcommon)

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
ENDIF(BUILD_KF5)

# XFCE and GNOME.
IF(BUILD_XFCE)
	FIND_PACKAGE(ThunarX2)
ENDIF(BUILD_XFCE)
IF(BUILD_XFCE3)
	FIND_PACKAGE(ThunarX3)
ENDIF(BUILD_XFCE3)
IF(BUILD_GNOME)
	FIND_PACKAGE(LibNautilusExtension 3.0.0)
ENDIF(BUILD_GNOME)
IF(BUILD_MATE)
	FIND_PACKAGE(LibCajaExtension 1.18.0)
ENDIF(BUILD_MATE)
IF(BUILD_CINNAMON)
	FIND_PACKAGE(LibNemoExtension)
ENDIF(BUILD_CINNAMON)

INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(getpwuid_r "pwd.h" HAVE_GETPWUID_R)
IF(NOT HAVE_GETPWUID_R)
	CHECK_SYMBOL_EXISTS(getpwuid   "pwd.h" HAVE_GETPWUID)
ENDIF(NOT HAVE_GETPWUID_R)
IF(NOT HAVE_GETPWUID_R AND NOT HAVE_GETPWUID)
	MESSAGE(FATAL_ERROR "Neither getpwuid_r() nor getpwuid() were found.")
ENDIF(NOT HAVE_GETPWUID_R AND NOT HAVE_GETPWUID)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libunixcommon.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libunixcommon.h")

# Sources and headers.
IF(APPLE)
	SET(libunixcommon_SRCS userdirs_mac.mm)
	SET(libunixcommon_H userdirs.hpp)
ELSE(APPLE)
	SET(libunixcommon_SRCS userdirs.cpp dll-search.c)
	SET(libunixcommon_H userdirs.hpp dll-search.h)
ENDIF(APPLE)

######################
# Build the library. #
######################

ADD_LIBRARY(unixcommon STATIC
	${libunixcommon_SRCS}
	${libunixcommon_H}
	)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(unixcommon)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(unixcommon PROPERTIES EXCLUDE_FROM_ALL TRUE)
IF(APPLE)
	TARGET_LINK_LIBRARIES(unixcommon PUBLIC "-framework Foundation")
ENDIF(APPLE)

TARGET_INCLUDE_DIRECTORIES(unixcommon
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
	)
