PROJECT(rom-properties-kde5)
SET(ENV{QT_SELECT} qt5)

# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf5/
# Find KDE5 Extra CMake Modules.
FIND_PACKAGE(ECM ${REQUIRE_KDE5} 0.0.11 NO_MODULE)
IF(ECM_MODULE_PATH AND ECM_KDE_MODULE_DIR)
	# Make sure ECM's CMake files don't create an uninstall rule.
	SET(KDE_SKIP_UNINSTALL_TARGET TRUE)

	# Don't add KDE tests to the CTest build.
	SET(KDE_SKIP_TEST_SETTINGS TRUE)

	# Include KDE5 CMake modules.
	LIST(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
	INCLUDE(KDEInstallDirs)
	INCLUDE(KDECMakeSettings)

	# Qt5 requires "-fpic -fPIC" due to reduced relocations.
	SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fPIC")

	# Find Qt5.
	SET(Qt5_NO_LINK_QTMAIN 1)
	FIND_PACKAGE(Qt5 ${REQUIRE_KDE5} COMPONENTS Core Gui Widgets)
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
		EXEC_PROGRAM(${QTPATHS} ARGS --plugin-dir OUTPUT_VARIABLE PLUGIN_INSTALL_DIR)
		IF(NOT PLUGIN_INSTALL_DIR)
			MESSAGE(FATAL_ERROR "`qtpaths` isn't working correctly.")
		ENDIF(NOT PLUGIN_INSTALL_DIR)
		# FIXME: Mageia has the Qt path set to "/usr/lib64/qt5" instead of "/usr".
		# Reference: https://github.com/GerbilSoft/rom-properties/issues/69
		INCLUDE(ReplaceHardcodedPrefix)
		REPLACE_HARDCODED_PREFIX(PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
		SET(QT_PLUGIN_INSTALL_DIR "${PLUGIN_INSTALL_DIR}")

		# Find KDE5. (TODO: Version?)
		FIND_PACKAGE(KF5 ${REQUIRE_KDE5} COMPONENTS KIO WidgetsAddons FileMetaData)
		IF(NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND)
			# KDE5 not found.
			SET(BUILD_KDE5 OFF CACHE INTERNAL "Build the KDE5 plugin." FORCE)
		ENDIF(NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND)

		# TODO: Can we get the subdirectory from KFileMetaData?
		SET(KFMD_PLUGIN_INSTALL_DIR "${PLUGIN_INSTALL_DIR}/kf5/kfilemetadata")
		SET(KFOV_PLUGIN_INSTALL_DIR "${PLUGIN_INSTALL_DIR}/kf5/overlayicon")
	ELSE()
		# Qt5 not found.
		SET(BUILD_KDE5 OFF CACHE INTERNAL "Build the KDE5 plugin." FORCE)
	ENDIF()
ELSE()
	# KDE5 Extra CMake Modules not found.
	SET(BUILD_KDE5 OFF CACHE INTERNAL "Build the KDE5 plugin." FORCE)
ENDIF()

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde5_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde5_H    "${rom-properties-kde_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde5_UIS  "${rom-properties-kde_UIS}")

# Sources and headers. (KDE5-specific)
SET(rom-properties-kde5_SRCS
	${rom-properties-kde5_SRCS}
	RpExtractorPlugin.cpp
	RpOverlayIconPlugin.cpp
	RomPropertiesDialogPluginFactoryKDE5.cpp
	)
SET(rom-properties-kde5_H
	${rom-properties-kde5_H}
	RpExtractorPlugin.hpp
	RpOverlayIconPlugin.hpp
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
	TARGET_LINK_LIBRARIES(rom-properties-kde5 PRIVATE cachemgr romdata rpbase)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-kde5 PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-kde5 PRIVATE ${ZLIB_LIBRARY})
	TARGET_LINK_LIBRARIES(rom-properties-kde5 PUBLIC KF5::KIOWidgets KF5::WidgetsAddons KF5::FileMetaData)
	TARGET_LINK_LIBRARIES(rom-properties-kde5 PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)

	# Add zlib and png directories for the About tab.
	IF(HAVE_ZLIB)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde5 PRIVATE ${ZLIB_INCLUDE_DIRS})
	ENDIF(HAVE_ZLIB)
	IF(HAVE_PNG)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde5 PRIVATE ${PNG_INCLUDE_DIRS})
	ENDIF(HAVE_PNG)

	# RpExtractorPluginForwarder
	ADD_LIBRARY(kfilemetadata_rom-properties-kde5 MODULE
		RpExtractorPluginForwarder.cpp
		RpExtractorPluginForwarder.hpp
		)
	SET_TARGET_PROPERTIES(kfilemetadata_rom-properties-kde5 PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(kfilemetadata_rom-properties-kde5)
	TARGET_LINK_LIBRARIES(kfilemetadata_rom-properties-kde5 KF5::KIOCore KF5::FileMetaData)
	TARGET_COMPILE_DEFINITIONS(kfilemetadata_rom-properties-kde5
		PRIVATE PLUGIN_INSTALL_DIR=\"${PLUGIN_INSTALL_DIR}\"
		)
	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(kfilemetadata_rom-properties-kde5 ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	# RpOverlayIconPluginForwarder
	# TODO: Use kcoreaddons_add_plugin() for the JSON?
	# https://github.com/owncloud/client/blob/master/shell_integration/dolphin/CMakeLists.txt
	# TODO: Better name.
	ADD_LIBRARY(overlayiconplugin_rom-properties-kde5 MODULE
		RpOverlayIconPluginForwarder.cpp
		RpOverlayIconPluginForwarder.hpp
		)
	SET_TARGET_PROPERTIES(overlayiconplugin_rom-properties-kde5 PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(overlayiconplugin_rom-properties-kde5)
	TARGET_LINK_LIBRARIES(overlayiconplugin_rom-properties-kde5 KF5::KIOCore KF5::KIOWidgets)
	TARGET_COMPILE_DEFINITIONS(overlayiconplugin_rom-properties-kde5
		PRIVATE PLUGIN_INSTALL_DIR=\"${PLUGIN_INSTALL_DIR}\"
		)
	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(overlayiconplugin_rom-properties-kde5 ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	# FIXME: Workaround for gcc-5.4.0 LTO bug.
	# Disabling LTO for the KDE5 build if using gcc-5.4.0 or earlier,
	# and decryption is enabled. (It crashes when compiling the moc
	# file for KeyManagerTab.cpp.)
	# NOTE: We can't add it to the automoc.cpp file because then automoc
	# will try including itself, resulting in a failure.
	IF(GCC_5xx_LTO_ISSUES AND ENABLE_DECRYPTION)
		SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fno-lto")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-lto")
	ENDIF(GCC_5xx_LTO_ISSUES AND ENABLE_DECRYPTION)
ENDIF(BUILD_KDE5)

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE5)
	INSTALL(TARGETS rom-properties-kde5
		LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(TARGETS kfilemetadata_rom-properties-kde5
		LIBRARY DESTINATION "${KFMD_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(TARGETS overlayiconplugin_rom-properties-kde5
		LIBRARY DESTINATION "${KFOV_PLUGIN_INSTALL_DIR}"
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

		GET_TARGET_PROPERTY(DEBUG_FILENAME kfilemetadata_rom-properties-kde5 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KFMD_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME overlayiconplugin_rom-properties-kde5 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KFOV_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE5)
