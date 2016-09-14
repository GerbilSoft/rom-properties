PROJECT(rom-properties-kde5)

# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf5/
# Find KDE5 Extra CMake Modules.
FIND_PACKAGE(ECM 0.0.11 NO_MODULE)
IF(ECM_MODULE_PATH AND ECM_KDE_MODULE_DIR)
	LIST(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
	INCLUDE(KDEInstallDirs)
	INCLUDE(KDECMakeSettings)
	INCLUDE(KDECompilerSettings)

	# Find Qt5.
	SET(Qt5_NO_LINK_QTMAIN 1)
	FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets)
	IF(Qt5Core_FOUND AND Qt5Gui_FOUND AND Qt5Widgets_FOUND)
		# FIXME: Some systems some have the correct value for PLUGIN_INSTALL_DIR.
		#
		# Ubuntu:
		# - Expected: lib/${DEB_HOST_MULTIARCH}/qt5/plugins
		# - Actual:   lib/${DEB_HOST_MULTIARCH}/plugins
		#
		# Gentoo:
		# - Expected: lib64/qt5/plugins
		# - Actual:   lib64/plugins
		#
		# Fix this by using "${LIB_INSTALL_DIR}/qt5/plugins".
		#
		SET(PLUGIN_INSTALL_DIR "${LIB_INSTALL_DIR}/qt5/plugins")
		SET(QT_PLUGIN_INSTALL_DIR "${PLUGIN_INSTALL_DIR}")

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

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde5_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde5_H "${rom-properties-kde_H}")

# Sources and headers. (KDE5-specific)
SET(rom-properties-kde5_SRCS
	${rom-properties-kde5_SRCS}
	RomPropertiesDialogPluginFactoryKDE5.cpp
	)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE5)
	# Reference: https://community.kde.org/Frameworks/Porting_Notes
	ADD_LIBRARY(rom-properties-kde5 MODULE
		${rom-properties-kde5_SRCS}
		${rom-properties-kde5_UIS_H}
		)
	SET_TARGET_PROPERTIES(rom-properties-kde5 PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-kde5)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde5
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>
		)
	TARGET_LINK_LIBRARIES(rom-properties-kde5
		romdata16
		cachemgr16
		KF5::KIOWidgets
		Qt5::Widgets Qt5::Gui Qt5::Core
		)
ENDIF(BUILD_KDE5)

# Define -DQT_NO_DEBUG in release builds.
SET(CMAKE_C_FLAGS_RELEASE   "-DQT_NO_DEBUG ${CMAKE_C_FLAGS_RELEASE}")
SET(CMAKE_CXX_FLAGS_RELEASE "-DQT_NO_DEBUG ${CMAKE_CXX_FLAGS_RELEASE}")

# Qt options:
# - Fast QString concatenation. (Qt 4.6+, plus 4.8-specific version)
# - Disable implicit QString ASCII casts.
# - Use strict iterators.
ADD_DEFINITIONS(-DQT_USE_FAST_CONCATENATION
	-DQT_USE_FAST_OPERATOR_PLUS
	-DQT_USE_QSTRINGBUILDER
	-DQT_NO_CAST_FROM_ASCII
	-DQT_NO_CAST_TO_ASCII
	-DQT_STRICT_ITERATORS
	)

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE5)
	INSTALL(TARGETS rom-properties-kde5
		LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
		COMPONENT "plugin"
		)
	# Services are installed in /usr/share/kservices5/
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom-properties-kde5.desktop"
		DESTINATION ${SERVICES_INSTALL_DIR}
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca5?

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-kde5 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES ${DEBUG_FILENAME}
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE5)
