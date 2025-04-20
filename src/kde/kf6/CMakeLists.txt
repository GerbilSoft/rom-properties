# KDE Frameworks 6.x UI frontend
PROJECT(rom-properties-kf6 LANGUAGES CXX)

# Find Qt6 and KF6.
INCLUDE(RP_FindQt6andKF6)
FIND_QT6_AND_KF6()

# Disable deprecated Qt and KDE functionality in debug builds.
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_DISABLE_DEPRECATED_BEFORE=0x060600 -DQT_DEPRECATED_WARNINGS_SINCE=0x060700")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x060000 -DKF_DEPRECATED_WARNINGS_SINCE=0x060000")

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.kf6.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.kf6.h")

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_H    "${rom-properties-kde_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_UIS  "${rom-properties-kde_UIS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_RCC_SRCS "${rom-properties-kde_RCC_SRCS}")

STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_SRCS "${rom-properties-kde_XATTR_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_H    "${rom-properties-kde_XATTR_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_UIS  "${rom-properties-kde_XATTR_UIS}")

STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kio6_SRCS "${rom-properties-kio_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kio6_H    "${rom-properties-kio_H}")

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/../config.kde.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.kde.h")

# Sources and headers. (KF6-specific)
IF(Qt6Core_FOUND)
	QT6_ADD_RESOURCES(
		${PROJECT_NAME}_RCC_O ${${PROJECT_NAME}_RCC_SRCS}
		OPTIONS -no-compress
		)
ENDIF(Qt6Core_FOUND)
IF(HAVE_QtDBus)
	IF(HAVE_QtDBus_NOTIFY)
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-kde-notify_SRCS}")
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-kde-notify_H}")
		QT6_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_NOTIFY_SRCS
			"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.Notifications.xml"
			)
	ENDIF(HAVE_QtDBus_NOTIFY)
	IF(ENABLE_NETWORKING)
		QT6_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_NM_SRCS
			"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.NetworkManager.xml"
			)
	ENDIF(ENABLE_NETWORKING)
ENDIF(HAVE_QtDBus)

# Main plugin
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS}
	${rom-properties-kio6_SRCS}
	${${PROJECT_NAME}-notify_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_NM_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_NOTIFY_SRCS}
	${${PROJECT_NAME}_RCC_O}
	PluginFactoryKF6.cpp
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${rom-properties-kio6_H}
	${${PROJECT_NAME}-notify_H}
	)

# XAttrView plugin
SET(${PROJECT_NAME}_XATTR_SRCS
	${${PROJECT_NAME}_XATTR_SRCS}
	XAttrViewPluginFactoryKF6.cpp
	)

IF(COMMAND QT6_WRAP_UI)
	# Wrap UI files.
	QT6_WRAP_UI(${PROJECT_NAME}_UIS_H ${${PROJECT_NAME}_UIS})
	QT6_WRAP_UI(${PROJECT_NAME}_XATTR_UIS_H ${${PROJECT_NAME}_XATTR_UIS})
	ADD_CUSTOM_TARGET(kf6_generate_ui_h DEPENDS ${${PROJECT_NAME}_UIS_H} ${PROJECT_NAME}_XATTR_UIS_H)
ENDIF(COMMAND QT6_WRAP_UI)

#####################
# Build the plugin. #
#####################

IF(BUILD_KF6)
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
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf6
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf6
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC KF6::KIOWidgets KF6::KIOGui KF6::KIOCore KF6::WidgetsAddons KF6::FileMetaData KF6::Crash)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt6::Network Qt6::Widgets Qt6::Gui Qt6::Core)
	IF(HAVE_QtDBus)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt6::DBus)
	ENDIF(HAVE_QtDBus)

	# PugiXML headers are needed for AboutTab.
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${pugixml_INCLUDE_DIR})

	# libfmt
	IF(Fmt_FOUND)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${Fmt_LIBRARY})
	ENDIF(Fmt_FOUND)

	### ThumbnailCreator plugin ###
	SET(RomThumbnailCreator_NAME RomThumbnailCreator-kf6)
	ADD_LIBRARY(${RomThumbnailCreator_NAME} MODULE
		ThumbnailCreatorPluginFactoryKF6.cpp
		../plugins/RomThumbnailCreator.cpp
		../plugins/RomThumbCreator_p.cpp
		../ProxyForUrl.cpp
		../RpQImageBackend.cpp
		../RpQUrl.cpp
		../RpQt.cpp

		../plugins/RomThumbnailCreator.hpp
		../plugins/RomThumbCreator_p.hpp
		../ProxyForUrl.hpp
		../RpQImageBackend.hpp
		../RpQUrl.hpp
		../RpQt.hpp

		# NetworkManager D-Bus interface
		${${PROJECT_NAME}_DBUS_IFACE_NM_SRCS}
		)
	SET_TARGET_PROPERTIES(${RomThumbnailCreator_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(${RomThumbnailCreator_NAME})
	TARGET_INCLUDE_DIRECTORIES(${RomThumbnailCreator_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf6
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf6
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_COMPILE_DEFINITIONS(${RomThumbnailCreator_NAME} PRIVATE RP_NO_INCLUDE_LIBFMT_IN_STDAFX_H)
	TARGET_LINK_LIBRARIES(${RomThumbnailCreator_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${RomThumbnailCreator_NAME} PRIVATE KF6::KIOWidgets KF6::KIOCore)
	TARGET_LINK_LIBRARIES(${RomThumbnailCreator_NAME} PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core)
	IF(HAVE_QtDBus)
		TARGET_LINK_LIBRARIES(${RomThumbnailCreator_NAME} PUBLIC Qt6::DBus)
	ENDIF(HAVE_QtDBus)

	### XAttrView plugin ###
	SET(XATTRVIEW_NAME xattrview-kf6)

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
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf6
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf6
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PRIVATE KF6::KIOWidgets KF6::KIOCore)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core)

	# libfmt
	IF(Fmt_FOUND)
		TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PRIVATE ${Fmt_LIBRARY})
	ENDIF(Fmt_FOUND)

	### Additional Plugins ###

	# ExtractorPlugin
	ADD_LIBRARY(kfilemetadata_${PROJECT_NAME} MODULE
		../plugins/ExtractorPlugin.cpp
		../RpQUrl.cpp

		../plugins/ExtractorPluginKF6.hpp
		../RpQtNS.hpp
		../RpQUrl.hpp
		)
	SET_TARGET_PROPERTIES(kfilemetadata_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(kfilemetadata_${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(kfilemetadata_${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf6
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf6
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_COMPILE_DEFINITIONS(kfilemetadata_${PROJECT_NAME} PRIVATE RP_NO_INCLUDE_LIBFMT_IN_STDAFX_H)
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE KF6::KIOCore KF6::FileMetaData)
	# NOTE: Technically, only Qt6::Core is needed, but stdafx.h usage
	# requires other libraries for include paths.
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core)

	# libfmt
	IF(Fmt_FOUND)
		TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE ${Fmt_LIBRARY})
	ENDIF(Fmt_FOUND)

	# OverlayIconPlugin
	# TODO: Use kcoreaddons_add_plugin() for the JSON?
	# https://github.com/owncloud/client/blob/master/shell_integration/dolphin/CMakeLists.txt
	# TODO: Better name.
	ADD_LIBRARY(overlayiconplugin_${PROJECT_NAME} MODULE
		../plugins/OverlayIconPlugin.cpp
		../RpQUrl.cpp

		../plugins/OverlayIconPluginKF6.hpp
		../RpQtNS.hpp
		../RpQUrl.hpp
		)
	SET_TARGET_PROPERTIES(overlayiconplugin_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(overlayiconplugin_${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(overlayiconplugin_${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kf6
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kf6
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE KF6::KIOCore KF6::FileMetaData)
	# NOTE: Technically, only Qt6::Core is needed, but stdafx.h usage
	# requires other libraries for include paths.
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core)

	# libfmt
	IF(Fmt_FOUND)
		TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE ${Fmt_LIBRARY})
	ENDIF(Fmt_FOUND)
ENDIF(BUILD_KF6)

#######################
# Install the plugin. #
#######################

IF(BUILD_KF6)
	# Process MIME types for the .json files.
	# FIXME: Need to add a dependency to ensure these files are regenerated if the XDG files are changed.
	# TODO: Better versioning for ExtractorPlugin?
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
	PARSE_MIME_TYPES_JSON_VERSIONED(MIMETYPES_ALL_JSON_VERSIONED_ARRAY "        "
		"${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types")

	CONFIGURE_FILE(${PROJECT_NAME}.json.in ${PROJECT_NAME}.json @ONLY)
	CONFIGURE_FILE(RomThumbnailCreator.json.in RomThumbnailCreator.json @ONLY)
	CONFIGURE_FILE(ExtractorPlugin.json.in ExtractorPlugin.json @ONLY)
	CONFIGURE_FILE(OverlayIconPlugin.json.in OverlayIconPlugin.json @ONLY)

	# Install plugins in the appropriate subdirectories.
	INSTALL(TARGETS ${PROJECT_NAME} ${XATTRVIEW_NAME}
		LIBRARY DESTINATION "${KF6_PRPD_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	INSTALL(TARGETS ${RomThumbnailCreator_NAME}
		LIBRARY DESTINATION "${KF6_THUMB_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	INSTALL(TARGETS kfilemetadata_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF6_KFMD_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	INSTALL(TARGETS overlayiconplugin_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF6_KOVI_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	# FIXME: Run kbuildsycoca6?

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF6_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME ${XATTRVIEW_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF6_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug")
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME kfilemetadata_${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF6_KFMD_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME overlayiconplugin_${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KF6_KOVI_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KF6)
