# KDE Frameworks 5.x UI frontend
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-kf5 LANGUAGES CXX)

# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf5/
# Find KF5 Extra CMake Modules.
SET(ENV{QT_SELECT} qt5)
FIND_PACKAGE(ECM ${REQUIRE_KF5} 0.0.11 NO_MODULE)
IF(ECM_MODULE_PATH AND ECM_KDE_MODULE_DIR)
	# Make sure ECM's CMake files don't create an uninstall rule.
	SET(KDE_SKIP_UNINSTALL_TARGET TRUE)

	# Don't add KDE tests to the CTest build.
	SET(KDE_SKIP_TEST_SETTINGS TRUE)

	# Include KF5 CMake modules.
	LIST(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
	INCLUDE(KDEInstallDirs)
	INCLUDE(KDECMakeSettings)

	# Qt5 requires "-fpic -fPIC" due to reduced relocations.
	SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fPIC")

	# Find Qt5.
	SET(Qt5_NO_LINK_QTMAIN 1)
	FIND_PACKAGE(Qt5 ${REQUIRE_KF5} COMPONENTS Core Gui Widgets DBus)
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
		EXEC_PROGRAM(${QTPATHS} ARGS --plugin-dir OUTPUT_VARIABLE KF5_PLUGIN_INSTALL_DIR)
		IF(NOT KF5_PLUGIN_INSTALL_DIR)
			MESSAGE(FATAL_ERROR "`qtpaths` isn't working correctly.")
		ENDIF(NOT KF5_PLUGIN_INSTALL_DIR)
		# FIXME: Mageia has the Qt path set to "/usr/lib64/qt5" instead of "/usr".
		# Reference: https://github.com/GerbilSoft/rom-properties/issues/69
		INCLUDE(ReplaceHardcodedPrefix)
		REPLACE_HARDCODED_PREFIX(KF5_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
		SET(QT_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}")

		# Find KF5. (TODO: Version?)
		FIND_PACKAGE(KF5 ${REQUIRE_KF5} COMPONENTS KIO WidgetsAddons FileMetaData)
		IF(NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND)
			# KF5 not found.
			SET(BUILD_KF5 OFF CACHE INTERNAL "Build the KDE Frameworks 5 plugin." FORCE)
		ENDIF(NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND)

		SET(KF5_PRPD_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/propertiesdialog")
		SET(KF5_KFMD_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/kfilemetadata")
		SET(KF5_KOVI_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/overlayicon")

		IF(ENABLE_ACHIEVEMENTS AND Qt5DBus_FOUND)
			# QtDBus is used for notifications.
			# TODO: Make notifications optional.
			SET(HAVE_QtDBus_NOTIFY 1)
		ENDIF(ENABLE_ACHIEVEMENTS AND Qt5DBus_FOUND)
	ELSE()
		# Qt5 not found.
		SET(BUILD_KF5 OFF CACHE INTERNAL "Build the KDE Frameworks 5 plugin." FORCE)
	ENDIF()
ELSE()
	# KF5 Extra CMake Modules not found.
	SET(BUILD_KF5 OFF CACHE INTERNAL "Build the KDE Frameworks 5 plugin." FORCE)
ENDIF()

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.kf5.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.kf5.h")

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_H    "${rom-properties-kde_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_UIS  "${rom-properties-kde_UIS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_RCC_SRCS "${rom-properties-kde_RCC_SRCS}")

STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kio5_SRCS "${rom-properties-kio_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kio5_H    "${rom-properties-kio_H}")

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/../config.kde.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.kde.h")

# Sources and headers. (KF5-specific)
IF(Qt5Core_FOUND)
	QT5_ADD_RESOURCES(
		${PROJECT_NAME}_RCC_O ${${PROJECT_NAME}_RCC_SRCS}
		OPTIONS -no-compress
		)
ENDIF(Qt5Core_FOUND)
IF(HAVE_QtDBus_NOTIFY)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-kde-notify_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-kde-notify_H}")

	# QtDBus wrappers
	QT5_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_SRCS
		"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.Notifications.xml"
		)
ENDIF(HAVE_QtDBus_NOTIFY)

SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS}
	${rom-properties-kio5_SRCS}
	${${PROJECT_NAME}-notify_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_SRCS}
	${${PROJECT_NAME}_RCC_O}
	RomPropertiesDialogPluginFactoryKF5.cpp
	ExtractorPlugin.cpp
	OverlayIconPlugin.cpp
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${rom-properties-kio5_H}
	${${PROJECT_NAME}-notify_H}
	ExtractorPlugin.hpp
	OverlayIconPlugin.hpp
	)

IF(COMMAND QT5_WRAP_UI)
	# Wrap UI files.
	QT5_WRAP_UI(${PROJECT_NAME}_UIS_H ${${PROJECT_NAME}_UIS})
	ADD_CUSTOM_TARGET(kf5_generate_ui_h DEPENDS ${${PROJECT_NAME}_UIS_H})
ENDIF(COMMAND QT5_WRAP_UI)

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(${PROJECT_NAME}_PCH_CXX ../stdafx.cpp)
	SET(${PROJECT_NAME}_PCH_SRC ${rom-properties-kde4_PCH_CXX})
	SET(${PROJECT_NAME}_PCH_H ../stdafx.h)
ENDIF(ENABLE_PCH)

#####################
# Build the plugin. #
#####################

IF(BUILD_KF5)
	# Reference: https://community.kde.org/Frameworks/Porting_Notes
	ADD_LIBRARY(${PROJECT_NAME} MODULE
		${${PROJECT_NAME}_PCH_SRC} ${${PROJECT_NAME}_PCH_H}
		${${PROJECT_NAME}_SRCS}
		${${PROJECT_NAME}_H}
		${${PROJECT_NAME}_UIS_H}
		)
	IF(ENABLE_PCH)
		ADD_PRECOMPILED_HEADER(${PROJECT_NAME} ${${PROJECT_NAME}_PCH_H}
			SOURCE_CXX ${${PROJECT_NAME}_PCH_CXX}
			)
	ENDIF(ENABLE_PCH)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf5
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf5
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARY})
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC KF5::KIOCore KF5::KIOWidgets KF5::WidgetsAddons KF5::FileMetaData)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)
	IF(HAVE_QtDBus_NOTIFY)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt5::DBus)
	ENDIF(HAVE_QtDBus_NOTIFY)

	# Add zlib and png directories for the About tab.
	IF(HAVE_ZLIB)
		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
	ENDIF(HAVE_ZLIB)
	IF(HAVE_PNG)
		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${PNG_INCLUDE_DIRS})
	ENDIF(HAVE_PNG)

	# ExtractorPluginForwarder
	ADD_LIBRARY(kfilemetadata_${PROJECT_NAME} MODULE
		ExtractorPluginForwarder.cpp
		ExtractorPluginForwarder.hpp
		)
	SET_TARGET_PROPERTIES(kfilemetadata_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(kfilemetadata_${PROJECT_NAME})
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE KF5::KIOCore KF5::FileMetaData)

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	# OverlayIconPluginForwarder
	# TODO: Use kcoreaddons_add_plugin() for the JSON?
	# https://github.com/owncloud/client/blob/master/shell_integration/dolphin/CMakeLists.txt
	# TODO: Better name.
	ADD_LIBRARY(overlayiconplugin_${PROJECT_NAME} MODULE
		OverlayIconPluginForwarder.cpp
		OverlayIconPluginForwarder.hpp
		)
	SET_TARGET_PROPERTIES(overlayiconplugin_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(overlayiconplugin_${PROJECT_NAME})
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE KF5::KIOCore KF5::KIOWidgets)

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	# FIXME: Workaround for gcc-5.4.0 LTO bug.
	# Disabling LTO for the kf5 build if using gcc-5.4.0 or earlier,
	# and decryption is enabled. (It crashes when compiling the moc
	# file for KeyManagerTab.cpp.)
	# NOTE: We can't add it to the automoc.cpp file because then automoc
	# will try including itself, resulting in a failure.
	IF(GCC_5xx_LTO_ISSUES AND ENABLE_DECRYPTION)
		SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fno-lto")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-lto")
	ENDIF(GCC_5xx_LTO_ISSUES AND ENABLE_DECRYPTION)
ENDIF(BUILD_KF5)

#######################
# Install the plugin. #
#######################

IF(BUILD_KF5)
	# Process MIME types for the .desktop and .json files.
	# FIXME: Need to add a dependency to ensure these files are regenerated if the XDG files are changed.
	INCLUDE(ParseMimeTypes)
	PARSE_MIME_TYPES(MIMETYPES_THUMBONLY
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		)
	PARSE_MIME_TYPES(MIMETYPES_ALL
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types"
		)
	PARSE_MIME_TYPES_JSON(MIMETYPES_ALL_JSON_ARRAY "            "
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types"
		)
	CONFIGURE_FILE(${PROJECT_NAME}.KPropertiesDialog.desktop.in ${PROJECT_NAME}.KPropertiesDialog.desktop @ONLY)
	CONFIGURE_FILE(${PROJECT_NAME}.ThumbCreator.desktop.in      ${PROJECT_NAME}.ThumbCreator.desktop @ONLY)
	CONFIGURE_FILE(${PROJECT_NAME}.json.in                      ${PROJECT_NAME}.json    @ONLY)

	# FIXME: Installing the main plugin into ${KF5_PRPD_PLUGIN_INSTALL_DIR}
	# breaks thumbnails. We might need to make a KPropertiesDialog forwarder.
	INSTALL(TARGETS ${PROJECT_NAME}
		LIBRARY DESTINATION "${KF5_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(TARGETS kfilemetadata_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF5_KFMD_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(TARGETS overlayiconplugin_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF5_KOVI_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.KPropertiesDialog.desktop"
		      "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.ThumbCreator.desktop"
		DESTINATION "${SERVICES_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca5?

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KF5_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME kfilemetadata_${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KF5_KFMD_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME overlayiconplugin_${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KF5_KOVI_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KF5)
