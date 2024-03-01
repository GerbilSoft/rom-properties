# KDE Frameworks 5.x UI frontend
PROJECT(rom-properties-kf5 LANGUAGES CXX)

# Find Qt5 and KF5.
INCLUDE(RP_FindQt5andKF5)
FIND_QT5_AND_KF5()

# FIXME: On Ubuntu 16.04, ${SERVICES_INSTALL_DIR} is incorrectly
# set to the KDE4 directory. Not sure if it's because kdelibs5
# for KDE4 is also installed...
IF(SERVICES_INSTALL_DIR MATCHES "kde4")
	SET(SERVICES_INSTALL_DIR "share/kservices5")
ENDIF()

# Disable anything deprecated as of Qt 5.15.
ADD_DEFINITIONS(-DQT_DISABLE_DEPRECATED_UP_TO=0x050F00)

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
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${rom-properties-kio5_H}
	${${PROJECT_NAME}-notify_H}
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
			"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
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
	#		"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
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

	### Additional Plugins ###

	# ExtractorPlugin
	ADD_LIBRARY(kfilemetadata_${PROJECT_NAME} MODULE
		../plugins/ExtractorPlugin.cpp
		../RpQUrl.cpp

		../plugins/ExtractorPluginKF5.hpp
		../RpQtNS.hpp
		../RpQUrl.hpp
		)
	SET_TARGET_PROPERTIES(kfilemetadata_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(kfilemetadata_${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(kfilemetadata_${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf5
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf5
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE KF5::KIOCore KF5::FileMetaData)
	# NOTE: Technically, only Qt5::Core is needed, but stdafx.h usage
	# requires other libraries for include paths.
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Core)

	# OverlayIconPlugin
	# TODO: Use kcoreaddons_add_plugin() for the JSON?
	# https://github.com/owncloud/client/blob/master/shell_integration/dolphin/CMakeLists.txt
	# TODO: Better name.
	ADD_LIBRARY(overlayiconplugin_${PROJECT_NAME} MODULE
		../plugins/OverlayIconPlugin.cpp
		../RpQUrl.cpp

		../plugins/OverlayIconPluginKF5.hpp
		../RpQtNS.hpp
		../RpQUrl.hpp
		)
	SET_TARGET_PROPERTIES(overlayiconplugin_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(overlayiconplugin_${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(overlayiconplugin_${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf5
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf5
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE KF5::KIOCore KF5::KIOWidgets)
	# NOTE: Technically, only Qt5::Core is needed, but stdafx.h usage
	# requires other libraries for include paths.
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Core)
ENDIF(BUILD_KF5)

#######################
# Install the plugin. #
#######################

IF(BUILD_KF5)
	# Process MIME types for the .desktop and .json files.
	# FIXME: Need to add a dependency to ensure these files are regenerated if the XDG files are changed.
	# TODO: Better versioning for ExtractorPlugin?
	INCLUDE(ParseMimeTypes)
	PARSE_MIME_TYPES(MIMETYPES_THUMBONLY
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types")
	PARSE_MIME_TYPES(MIMETYPES_ALL
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types")
	PARSE_MIME_TYPES_JSON(MIMETYPES_ALL_JSON_ARRAY "            "
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types")
	PARSE_MIME_TYPES_JSON_VERSIONED(MIMETYPES_ALL_JSON_VERSIONED_ARRAY "        "
		"${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types")
	IF(NOT HAVE_JSON_PLUGIN_LOADER)
		CONFIGURE_FILE(${PROJECT_NAME}.KPropertiesDialog.desktop.in ${PROJECT_NAME}.KPropertiesDialog.desktop @ONLY)
	ENDIF(NOT HAVE_JSON_PLUGIN_LOADER)
	IF(NOT HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
		CONFIGURE_FILE(${PROJECT_NAME}.ThumbCreator.desktop.in      ${PROJECT_NAME}.ThumbCreator.desktop @ONLY)
	ENDIF(NOT HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H)
	CONFIGURE_FILE(${PROJECT_NAME}.json.in ${PROJECT_NAME}.json @ONLY)
	CONFIGURE_FILE(ExtractorPlugin.json.in ExtractorPlugin.json @ONLY)
	CONFIGURE_FILE(OverlayIconPlugin.json.in OverlayIconPlugin.json @ONLY)

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

	IF(HAVE_KF5_DEPRECATED_SERVICE_MENU_DIR)
		# Service menus (deprecated install location)
		# KF5 <5.85 uses ${SERVICES_INSTALL_DIR}.
		INSTALL(FILES ../servicemenus/rp-convert-to-png.desktop
			DESTINATION "${SERVICES_INSTALL_DIR}"
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			COMPONENT "plugin")
	ENDIF(HAVE_KF5_DEPRECATED_SERVICE_MENU_DIR)

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
