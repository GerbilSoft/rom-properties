PROJECT(rom-properties-kde5)

# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf5/
# Find KDE5 Extra CMake Modules.
FIND_PACKAGE(ECM 0.0.11 NO_MODULE)
IF(ECM_MODULE_PATH AND ECM_KDE_MODULE_DIR)
	# Make sure ECM's CMake files don't create an uninstall rule.
	SET(KDE_SKIP_UNINSTALL_TARGET TRUE)

	# Don't add KDE tests to the CTest build.
	SET(KDE_SKIP_TEST_SETTINGS TRUE)

	# Include KDE5 CMake modules.
	LIST(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
	INCLUDE(KDEInstallDirs)
	INCLUDE(KDECMakeSettings)
	INCLUDE(KDECompilerSettings)

	# Get rid of the explicit C90 setting.
	STRING(REPLACE "-std=iso9899:1990" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

	# Qt5 requires "-fpic -fPIC" due to reduced relocations.
	SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fPIC")

	# Find Qt5.
	SET(Qt5_NO_LINK_QTMAIN 1)
	FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets)
	IF(Qt5Core_FOUND AND Qt5Gui_FOUND AND Qt5Widgets_FOUND)
		# NOTE: QT_PLUGIN_INSTALL_DIR is missing the 'qt5' directory.
		# Use `qtpaths` instead to get the actual path.
		#
		# Ubuntu:
		# - Expected: lib/${DEB_HOST_MULTIARCH}/qt5/plugins
		# - Actual:   lib/${DEB_HOST_MULTIARCH}/plugins
		#
		# Gentoo:
		# - Expected: lib64/qt5/plugins
		# - Actual:   lib64/plugins
		#
		# Arch:
		# - Expected: lib/qt/plugins
		# - Actual:   (FIXME)
		#

		# Find the qtpaths executable.
		FIND_PROGRAM(QTPATHS qtpaths)
		IF(NOT QTPATHS)
			MESSAGE(FATAL_ERROR "qtpaths not found. Install one of these packages:
  - Debian/Ubuntu: qttools5-dev-tools
  - Red Hat/Fedora: qt5-qttools")
		ENDIF(NOT QTPATHS)

		# Get the plugin directory and Qt prefix.
		# Prefix will be removed from the plugin directory if necessary.
		EXEC_PROGRAM(qtpaths ARGS --plugin-dir OUTPUT_VARIABLE PLUGIN_INSTALL_DIR)
		IF(NOT PLUGIN_INSTALL_DIR)
			MESSAGE(FATAL_ERROR "`qtpaths` isn't working correctly.")
		ENDIF(NOT PLUGIN_INSTALL_DIR)
		# FIXME: Mageia has the Qt path set to "/usr/lib64/qt5" instead of "/usr".
		# Reference: https://github.com/GerbilSoft/rom-properties/issues/69
		INCLUDE(ReplaceHardcodedPrefix)
		REPLACE_HARDCODED_PREFIX(PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
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
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde5_UIS "${rom-properties-kde_UIS}")

# Sources and headers. (KDE5-specific)
SET(rom-properties-kde5_SRCS
	${rom-properties-kde5_SRCS}
	RomPropertiesDialogPluginFactoryKDE5.cpp
	)

IF(COMMAND QT5_WRAP_UI)
	# Wrap UI files.
	QT5_WRAP_UI(rom-properties-kde5_UIS_H ${rom-properties-kde5_UIS})
	ADD_CUSTOM_TARGET(kde5_generate_ui_h DEPENDS ${rom-properties-kde5_UIS_H})
ENDIF(COMMAND QT5_WRAP_UI)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE5)
	# Reference: https://community.kde.org/Frameworks/Porting_Notes
	ADD_LIBRARY(rom-properties-kde5 MODULE
		${rom-properties-kde5_SRCS}
		${rom-properties-kde5_H}
		${rom-properties-kde5_UIS_H}
		)
	SET_TARGET_PROPERTIES(rom-properties-kde5 PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(rom-properties-kde5)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde5
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kde5
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kde5
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(rom-properties-kde5 cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-kde5 i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-kde5 ${ZLIB_LIBRARY})
	TARGET_LINK_LIBRARIES(rom-properties-kde5 KF5::KIOWidgets)
	TARGET_LINK_LIBRARIES(rom-properties-kde5 Qt5::Widgets Qt5::Gui Qt5::Core)

	# Add zlib and png directories for the About tab.
	IF(HAVE_ZLIB)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde5 PRIVATE ${ZLIB_INCLUDE_DIRS})
	ENDIF(HAVE_ZLIB)
	IF(HAVE_PNG)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde5 PRIVATE ${PNG_INCLUDE_DIRS})
	ENDIF(HAVE_PNG)
ENDIF(BUILD_KDE5)

# Define -DQT_NO_DEBUG in release builds.
SET(CMAKE_C_FLAGS_RELEASE   "-DQT_NO_DEBUG ${CMAKE_C_FLAGS_RELEASE}")
SET(CMAKE_CXX_FLAGS_RELEASE "-DQT_NO_DEBUG ${CMAKE_CXX_FLAGS_RELEASE}")

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE5)
	INSTALL(TARGETS rom-properties-kde5
		LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom-properties-kde5.desktop"
		DESTINATION "${SERVICES_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca5?

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-kde5 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE5)
