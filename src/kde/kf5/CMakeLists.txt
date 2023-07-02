# KDE Frameworks 5.x UI frontend
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
		# Use `qtpaths5` instead to get the actual path.
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

		# Find the qtpaths5 executable.
		FIND_PROGRAM(QTPATHS5 NAMES qtpaths5 qtpaths)
		IF(NOT QTPATHS5)
			MESSAGE(FATAL_ERROR "qtpaths5 not found. Install one of these packages:
  - Debian/Ubuntu: qttools5-dev-tools
  - Red Hat/Fedora: qt5-qttools")
		ENDIF(NOT QTPATHS5)

		# Get the plugin directory and Qt prefix.
		# Prefix will be removed from the plugin directory if necessary.
		EXEC_PROGRAM(${QTPATHS5} ARGS --plugin-dir OUTPUT_VARIABLE KF5_PLUGIN_INSTALL_DIR)
		IF(NOT KF5_PLUGIN_INSTALL_DIR)
			MESSAGE(FATAL_ERROR "`qtpaths5` isn't working correctly.")
		ENDIF(NOT KF5_PLUGIN_INSTALL_DIR)
		# FIXME: Mageia has the Qt path set to "/usr/lib64/qt5" instead of "/usr".
		# Reference: https://github.com/GerbilSoft/rom-properties/issues/69
		INCLUDE(ReplaceHardcodedPrefix)
		REPLACE_HARDCODED_PREFIX(KF5_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
		SET(QT_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}")

		# Find KF5. (TODO: Version?)
		FIND_PACKAGE(KF5 ${REQUIRE_KF5} COMPONENTS CoreAddons KIO WidgetsAddons FileMetaData)
		IF(NOT KF5CoreAddons_FOUND OR NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND)
			# KF5 not found.
			SET(BUILD_KF5 OFF CACHE INTERNAL "Build the KDE Frameworks 5 plugin." FORCE)
		ENDIF(NOT KF5CoreAddons_FOUND OR NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND)

		# CoreAddons: If 5.89 or later, use JSON installation instead of .desktop files.
		IF(TARGET KF5::CoreAddons AND KF5CoreAddons_VERSION VERSION_GREATER 5.88.79)
			SET(HAVE_JSON_PLUGIN_LOADER 1)
		ENDIF(TARGET KF5::CoreAddons AND KF5CoreAddons_VERSION VERSION_GREATER 5.88.79)

		# KIOGui library is needed if we have KIO/ThumbnailCreator.
		IF(TARGET KF5::KIOGui AND KF5KIO_VERSION VERSION_GREATER 5.99.79)
			# FIXME: CheckIncludeFileCXX requires compiling, which is difficult
			# due to requiring a ton of Qt and KDE libraries.
			# Instead, only check the KF5 version number. (5.100+)
			SET(HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H 1)
		ENDIF(TARGET KF5::KIOGui AND KF5KIO_VERSION VERSION_GREATER 5.99.79)

		SET(KF5_PRPD_PLUGIN_INSTALL_DIR  "${KF5_PLUGIN_INSTALL_DIR}/kf5/propertiesdialog")
		SET(KF5_KFMD_PLUGIN_INSTALL_DIR  "${KF5_PLUGIN_INSTALL_DIR}/kf5/kfilemetadata")
		SET(KF5_KOVI_PLUGIN_INSTALL_DIR  "${KF5_PLUGIN_INSTALL_DIR}/kf5/overlayicon")
		SET(KF5_THUMB_PLUGIN_INSTALL_DIR "${KF5_PLUGIN_INSTALL_DIR}/kf5/thumbcreator")

		IF(Qt5DBus_FOUND)
			SET(HAVE_QtDBus 1)
			IF(ENABLE_ACHIEVEMENTS)
				# QtDBus is used for notifications.
				# TODO: Make notifications optional.
				SET(HAVE_QtDBus_NOTIFY 1)
			ENDIF(ENABLE_ACHIEVEMENTS)
		ENDIF(Qt5DBus_FOUND)
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

STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_SRCS "${rom-properties-kde_XATTR_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_H    "${rom-properties-kde_XATTR_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_UIS  "${rom-properties-kde_XATTR_UIS}")

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
IF(HAVE_QtDBus)
	QT5_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_SRCS
		"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.NetworkManager.xml"
		)
	IF(HAVE_QtDBus_NOTIFY)
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-kde-notify_SRCS}")
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-kde-notify_H}")
		QT5_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_SRCS_2
			"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.Notifications.xml"
			)
	ENDIF(HAVE_QtDBus_NOTIFY)
ENDIF(HAVE_QtDBus)

# Main plugin
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS}
	${rom-properties-kio5_SRCS}
	${${PROJECT_NAME}-notify_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_SRCS_2}
	${${PROJECT_NAME}_RCC_O}
	PluginFactoryKF5.cpp
	../ExtractorPlugin.cpp
	../OverlayIconPlugin.cpp
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${rom-properties-kio5_H}
	${${PROJECT_NAME}-notify_H}
	../ExtractorPlugin.hpp
	../OverlayIconPlugin.hpp
	)

# XAttrView plugin
SET(${PROJECT_NAME}_XATTR_SRCS
	${${PROJECT_NAME}_XATTR_SRCS}
	XAttrViewPluginFactoryKF5.cpp
	)

IF(COMMAND QT5_WRAP_UI)
	# Wrap UI files.
	QT5_WRAP_UI(${PROJECT_NAME}_UIS_H ${${PROJECT_NAME}_UIS})
	QT5_WRAP_UI(${PROJECT_NAME}_XATTR_UIS_H ${${PROJECT_NAME}_XATTR_UIS})
	ADD_CUSTOM_TARGET(kf5_generate_ui_h DEPENDS ${${PROJECT_NAME}_UIS_H} ${${PROJECT_NAME}_XATTR_UIS_H})
ENDIF(COMMAND QT5_WRAP_UI)

#####################
# Build the plugin. #
#####################

IF(BUILD_KF5)
	### Main UI frontend plugin ###
	# Reference: https://community.kde.org/Frameworks/Porting_Notes
	ADD_LIBRARY(${PROJECT_NAME} MODULE
		${${PROJECT_NAME}_SRCS}
		${${PROJECT_NAME}_H}
		${${PROJECT_NAME}_UIS_H}
		)
	IF(ENABLE_PCH)
		TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} PRIVATE
			"${RP_CONFIG_LIBC_H}" "${RP_CXX11_COMPAT_H}"
			../stdafx.h
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
	IF(HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE KF5::KIOGui)
	ENDIF(HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE KF5::CoreAddons KF5::KIOCore KF5::KIOWidgets KF5::WidgetsAddons KF5::FileMetaData)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)
	IF(HAVE_QtDBus)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt5::DBus)
	ENDIF(HAVE_QtDBus)

	### XAttrView plugin ###
	SET(XATTRVIEW_NAME xattrview-kf5)

	ADD_LIBRARY(${XATTRVIEW_NAME} MODULE
		${${PROJECT_NAME}_XATTR_SRCS}
		${${PROJECT_NAME}_XATTR_H}
		${${PROJECT_NAME}_XATTR_UIS_H}
		)
	# TODO: Enable PCH?
	#IF(ENABLE_PCH)
	#	TARGET_PRECOMPILE_HEADERS(${XATTRVIEW_NAME} PRIVATE
	#		"${RP_CONFIG_LIBC_H}" "${RP_CXX11_COMPAT_H}"
	#		../stdafx.h
	#		)
	#ENDIF(ENABLE_PCH)
	SET_TARGET_PROPERTIES(${XATTRVIEW_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(${XATTRVIEW_NAME})
	TARGET_INCLUDE_DIRECTORIES(${XATTRVIEW_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf5
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf5
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PRIVATE KF5::KIOCore KF5::KIOWidgets)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)

	### Plugin Forwarders ###

	# ExtractorPluginForwarder
	ADD_LIBRARY(kfilemetadata_${PROJECT_NAME} MODULE
		ExtractorPluginForwarder.cpp
		ExtractorPluginForwarder.hpp
		)
	SET_TARGET_PROPERTIES(kfilemetadata_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(kfilemetadata_${PROJECT_NAME})
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE KF5::KIOCore KF5::FileMetaData)
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE Qt5::Core)

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
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE Qt5::Core)

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)
ENDIF(BUILD_KF5)

#######################
# Install the plugin. #
#######################

IF(BUILD_KF5)
	# Process MIME types for the .desktop and .json files.
	# FIXME: Need to add a dependency to ensure these files are regenerated if the XDG files are changed.
	INCLUDE(ParseMimeTypes)
	PARSE_MIME_TYPES(MIMETYPES_THUMBONLY
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types")
	PARSE_MIME_TYPES(MIMETYPES_ALL
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types")
	PARSE_MIME_TYPES_JSON(MIMETYPES_ALL_JSON_ARRAY "            "
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types")
	IF(NOT HAVE_JSON_PLUGIN_LOADER)
		CONFIGURE_FILE(${PROJECT_NAME}.KPropertiesDialog.desktop.in ${PROJECT_NAME}.KPropertiesDialog.desktop @ONLY)
	ENDIF(NOT HAVE_JSON_PLUGIN_LOADER)
	IF(NOT HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
		CONFIGURE_FILE(${PROJECT_NAME}.ThumbCreator.desktop.in      ${PROJECT_NAME}.ThumbCreator.desktop @ONLY)
	ENDIF(NOT HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
	CONFIGURE_FILE(${PROJECT_NAME}.json.in ${PROJECT_NAME}.json @ONLY)
	CONFIGURE_FILE(ExtractorPluginForwarder.json.in ExtractorPluginForwarder.json @ONLY)
	CONFIGURE_FILE(OverlayIconPluginForwarder.json.in OverlayIconPluginForwarder.json @ONLY)

	IF(HAVE_JSON_PLUGIN_LOADER)
		SET(${PROJECT_NAME}_PLUGIN_DIR "${KF5_PRPD_PLUGIN_INSTALL_DIR}")
	ELSE(HAVE_JSON_PLUGIN_LOADER)
		SET(${PROJECT_NAME}_PLUGIN_DIR "${KF5_PLUGIN_INSTALL_DIR}")
	ENDIF(HAVE_JSON_PLUGIN_LOADER)
	INSTALL(TARGETS ${PROJECT_NAME} ${XATTRVIEW_NAME}
		LIBRARY DESTINATION "${${PROJECT_NAME}_PLUGIN_DIR}"
		COMPONENT "plugin")

	IF(HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
		# Main plugin is installed in the propertiesdialog subdirectory.
		# Create a symlink for ThumbnailCreator.
		ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ln -sf "${${PROJECT_NAME}_PLUGIN_DIR}/${PROJECT_NAME}.so" "${PROJECT_NAME}.symlink.ThumbnailCreator.so"
			WORKING_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
			VERBATIM)
		INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.ThumbnailCreator.so"
			DESTINATION "${KF5_THUMB_PLUGIN_INSTALL_DIR}"
			RENAME "${PROJECT_NAME}.so"
			COMPONENT "plugin")
	ELSE(HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
		# NOTE: If using JSON plugin loading, we need to create a symlink for
		# the old ThumbCreator interface.
		IF(HAVE_JSON_PLUGIN_LOADER)
			ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
				COMMAND ln -sf "${${PROJECT_NAME}_PLUGIN_DIR}/${PROJECT_NAME}.so" "${PROJECT_NAME}.symlink.ForThumbCreator.so"
				WORKING_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
				VERBATIM)
			INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.ForThumbCreator.so"
				DESTINATION "${KF5_PLUGIN_INSTALL_DIR}"
				RENAME "${PROJECT_NAME}.so"
				COMPONENT "plugin")
		ENDIF(HAVE_JSON_PLUGIN_LOADER)
		INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.ThumbCreator.desktop"
			DESTINATION "${SERVICES_INSTALL_DIR}"
			COMPONENT "plugin")
	ENDIF(HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)

	INSTALL(TARGETS kfilemetadata_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF5_KFMD_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	INSTALL(TARGETS overlayiconplugin_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF5_KOVI_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	IF(NOT HAVE_JSON_PLUGIN_LOADER)
		INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.KPropertiesDialog.desktop"
		              "${XATTRVIEW_NAME}.desktop"
			DESTINATION "${SERVICES_INSTALL_DIR}"
			COMPONENT "plugin")
	ENDIF(NOT HAVE_JSON_PLUGIN_LOADER)
	# FIXME: Run kbuildsycoca5?

	# Service menus
	INSTALL(FILES ../servicemenus/rp-convert-to-png.desktop
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/kio/servicemenus"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		COMPONENT "plugin")

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF5_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug")
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME ${XATTRVIEW_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF5_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug")
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME kfilemetadata_${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF5_KFMD_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug")
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME overlayiconplugin_${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF5_KOVI_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug")
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KF5)
