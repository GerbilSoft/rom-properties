PROJECT(rom-properties-kde)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

IF(BUILD_KDE4)
	# Find Qt4.
	FIND_PACKAGE(Qt4 4.6.0 COMPONENTS QtCore QtGui)
	IF(QT4_FOUND)
		# Find KDE4. (TODO: Version?)
		FIND_PACKAGE(KDE4)
		IF(NOT KDE4_FOUND)
			# KDE4 not found.
			SET(BUILD_KDE4 OFF CACHE "" INTERNAL FORCE)
		ENDIF(NOT KDE4_FOUND)
	ELSE(QT4_FOUND)
		# Qt4 not found.
		SET(BUILD_KDE4 OFF CACHE "" INTERNAL FORCE)
	ENDIF(QT4_FOUND)
ENDIF(BUILD_KDE4)

IF(BUILD_KDE5)
	# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf5/
	# Find KDE5 Extra CMake Modules.
	FIND_PACKAGE(ECM 0.0.11 NO_MODULE)
	IF(ECM_MODULE_PATH AND ECM_KDE_MODULE_DIR)
		LIST(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
		INCLUDE(KDEInstallDirs)
		INCLUDE(KDECMakeSettings)
		INCLUDE(KDECompilerSettings)
		# Find Qt5.
		FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets)
		IF(Qt5Core_FOUND AND Qt5Gui_FOUND AND Qt5Widgets_FOUND)
			# Find KDE5. (TODO: Version?)
			FIND_PACKAGE(KF5 COMPONENTS KIO)
			IF(NOT KF5KIO_FOUND)
				# KDE5 not found.
				SET(BUILD_KDE5 OFF CACHE "" INTERNAL FORCE)
			ENDIF(NOT KF5KIO_FOUND)
		ELSE()
			# Qt5 not found.
			SET(BUILD_KDE5 OFF CACHE "" INTERNAL FORCE)
		ENDIF()
	ELSE()
		# KDE5 Extra CMake Modules not found.
		SET(BUILD_KDE5 OFF CACHE "" INTERNAL FORCE)
	ENDIF()
ENDIF(BUILD_KDE5)

# Include the current source and binary directories.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# Include the previous source and binary directories.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

# TODO: Check for iconv on non-Windows systems.
# NOTE: KDE4_ADD_PLUGIN() uses automoc.

# Sources and headers.
SET(rom-properties-kde_SRCS
	RomPropertiesDialogPlugin.cpp
	RomDataView.cpp
	)
SET(rom-properties-kde_H
	RomPropertiesDialogPlugin.hpp
	RomDataView.hpp
	)
IF(BUILD_KDE4)
	SET(rom-properties-kde4_SRCS
		RomPropertiesDialogPluginFactoryKDE4.cpp
		)
ENDIF(BUILD_KDE4)
IF(BUILD_KDE5)
	SET(rom-properties-kde5_SRCS
		RomPropertiesDialogPluginFactoryKDE5.cpp
		)
ENDIF(BUILD_KDE5)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE4)
	KDE4_ADD_PLUGIN(rom-properties-kde4
		${rom-properties-kde_SRCS}
		${rom-properties-kde4_SRCS}
		${rom-properties-kde_H}
		${rom-properties-kde4_UIS_H}
		)
	DO_SPLIT_DEBUG(rom-properties-kde4)
	TARGET_LINK_LIBRARIES(rom-properties-kde4
		romdata16
		${KDE4_KFILE_LIBRARY}
		Qt4::QtGui Qt4::QtCore
		)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PUBLIC ${KDE4_INCLUDE_DIR})
ENDIF(BUILD_KDE4)

IF(BUILD_KDE5)
	# Reference: https://community.kde.org/Frameworks/Porting_Notes
	ADD_LIBRARY(rom-properties-kde5 SHARED
		${rom-properties-kde_SRCS}
		${rom-properties-kde5_SRCS}
		${rom-properties-kde_H}
		${rom-properties-kde5_UIS_H}
		)
	SET_TARGET_PROPERTIES(rom-properties-kde5 PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-kde5)
	TARGET_LINK_LIBRARIES(rom-properties-kde5
		romdata16
		KF5::KIOWidgets
		Qt5::Widgets Qt5::Gui Qt5::Core
		)
ENDIF(BUILD_KDE5)

# Qt options:
# - Fast QString concatenation. (Qt 4.6+, plus 4.8-specific version)
# - Disable implicit QString ASCII casts.
ADD_DEFINITIONS(-DQT_USE_FAST_CONCATENATION
	-DQT_USE_FAST_OPERATOR_PLUS
	-DQT_USE_QSTRINGBUILDER
	-DQT_NO_CAST_FROM_ASCII
	-DQT_NO_CAST_TO_ASCII
	)

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE4)
	# FIXME: Determine the correct plugin path.
	# NOTE: Debian multiarch still uses /usr/lib/kde4.
	# Fedora 64-bit uses /usr/lib64/kde4. (/usr/lib is 32-bit)
	SET(KDE4_PLUGIN_DIRECTORY "lib/kde4")
	INSTALL(TARGETS rom-properties-kde4
		LIBRARY DESTINATION ${KDE4_PLUGIN_DIRECTORY}
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom-properties-kde4.desktop"
		DESTINATION "share/kde4/services"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca4.

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-kde4 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES ${DEBUG_FILENAME}
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KDE4_PLUGIN_DIRECTORY}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE4)

IF(BUILD_KDE5)
	# FIXME: Determine the correct plugin path.
	# On Kubuntu 16.04, it's /usr/lib/x86_64-linux-gnu/qt5/plugins/kf5/
	SET(KDE5_PLUGIN_DIRECTORY "lib/x86_64-linux-gnu/qt5/plugins/kf5")
	INSTALL(TARGETS rom-properties-kde5
		LIBRARY DESTINATION ${KDE5_PLUGIN_DIRECTORY}
		COMPONENT "plugin"
		)
	# Services are installed in /usr/share/kservices5/
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom-properties-kde5.desktop"
		DESTINATION "share/kservices5"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca5?

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-kde5 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES ${DEBUG_FILENAME}
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KDE5_PLUGIN_DIRECTORY}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE5)
