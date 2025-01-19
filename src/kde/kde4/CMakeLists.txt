# KDE 4.x UI frontend
PROJECT(rom-properties-kde4 LANGUAGES CXX)

# Enable the "new" automoc policy for KDE4.
# Otherwise, CMake complains because the old policy doesn't run
# automoc on generated sources, e.g. from uic or rcc.
# NOTE: Only on Qt4; CMake doesn't show this for Qt5/Qt6.
IF(POLICY CMP0071)
	CMAKE_POLICY(SET CMP0071 NEW)
ENDIF(POLICY CMP0071)

# Find Qt4 and KDE4.
INCLUDE(RP_FindQt4andKDE4)
FIND_QT4_AND_KDE4()

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_H "${rom-properties-kde_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_UIS "${rom-properties-kde_UIS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_RCC_SRCS "${rom-properties-kde_RCC_SRCS}")

STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_RomThumbCreator_SRCS "${rom-properties-kde_RomThumbCreator_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_RomThumbCreator_H    "${rom-properties-kde_RomThumbCreator_H}")

STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_SRCS "${rom-properties-kde_XATTR_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_H    "${rom-properties-kde_XATTR_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_XATTR_UIS  "${rom-properties-kde_XATTR_UIS}")

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/../config.kde.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.kde.h")

# Sources and headers. (KDE4-specific)
IF(QT4_FOUND)
	QT4_ADD_RESOURCES(
		${PROJECT_NAME}_RCC_O ${${PROJECT_NAME}_RCC_SRCS}
		OPTIONS -no-compress
		)
ENDIF(QT4_FOUND)
IF(HAVE_QtDBus)
	# FIXME: Broken on Qt4.
	IF(0)
		QT4_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_NM_SRCS
			"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.NetworkManager.xml"
			)
		# NOTE: Setting CMAKE_POLICY(SET CMP0071 NEW) causes the generated D-Bus .moc
		# to be included twice. Work around this by setting SKIP_AUTOMOC.
		SET_PROPERTY(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/networkmanagerinterface.h" PROPERTY SKIP_AUTOMOC ON)
	ENDIF(0)
	IF(HAVE_QtDBus_NOTIFY)
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-kde-notify_SRCS}")
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-kde-notify_H}")
		QT4_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_NOTIFY_SRCS
			"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.Notifications.xml"
			)
		# NOTE: Setting CMAKE_POLICY(SET CMP0071 NEW) causes the generated D-Bus .moc
		# to be included twice. Work around this by setting SKIP_AUTOMOC.
		SET_PROPERTY(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/notificationsinterface.h" PROPERTY SKIP_AUTOMOC ON)
	ENDIF(HAVE_QtDBus_NOTIFY)
ENDIF(HAVE_QtDBus)

# Main plugin
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}_RomThumbCreator_SRCS}
	${${PROJECT_NAME}-notify_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_NM_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_NOTIFY_SRCS}
	${${PROJECT_NAME}_RCC_O}
	PluginFactoryKDE4.cpp
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_RomThumbCreator_H}
	${${PROJECT_NAME}-notify_H}
	)

# XAttrView plugin
SET(${PROJECT_NAME}_XATTR_SRCS
	${${PROJECT_NAME}_XATTR_SRCS}
	XAttrViewPluginFactoryKDE4.cpp
	)

IF(COMMAND QT4_WRAP_UI)
	# Wrap UI files.
	QT4_WRAP_UI(${PROJECT_NAME}_UIS_H ${${PROJECT_NAME}_UIS})
	QT4_WRAP_UI(${PROJECT_NAME}_XATTR_UIS_H ${${PROJECT_NAME}_XATTR_UIS})
	ADD_CUSTOM_TARGET(kde4_generate_ui_h DEPENDS ${${PROJECT_NAME}_UIS_H} ${${PROJECT_NAME}_XATTR_UIS_H})
ENDIF(COMMAND QT4_WRAP_UI)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE4)
	### Main UI frontend plugin ###
	KDE4_ADD_PLUGIN(${PROJECT_NAME}
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
	DO_SPLIT_DEBUG(${PROJECT_NAME})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kde4
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kde4
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${KDE4_KFILE_LIBRARY} ${KDE4_KDEUI_LIBRARY} ${KDE4_KDECORE_LIBRARY})
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt4::QtGui Qt4::QtCore)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PUBLIC ${KDE4_INCLUDE_DIR} ${KDE4_INCLUDE_DIR}/KDE)
	IF(HAVE_QtDBus)
		TARGET_LINK_LIBRARIES(rom-properties-kde4 PUBLIC Qt4::QtDBus)
	ENDIF(HAVE_QtDBus)

	### XAttrView plugin ###
	SET(XATTRVIEW_NAME xattrview-kde4)

	KDE4_ADD_PLUGIN(${XATTRVIEW_NAME}
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
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kde4
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kde4
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PRIVATE romdata)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PUBLIC ${KDE4_KFILE_LIBRARY} ${KDE4_KDEUI_LIBRARY})
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PUBLIC Qt4::QtGui Qt4::QtCore)
ENDIF(BUILD_KDE4)

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE4)
	# Process MIME types for the .desktop files.
	INCLUDE(ParseMimeTypes)
	PARSE_MIME_TYPES(MIMETYPES_THUMBONLY
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		)
	PARSE_MIME_TYPES(MIMETYPES_ALL
		"${CMAKE_SOURCE_DIR}/xdg/mime.thumbnail.types"
		"${CMAKE_SOURCE_DIR}/xdg/mime.no-thumbnail.types"
		)
	CONFIGURE_FILE(${PROJECT_NAME}.KPropertiesDialog.desktop.in ${PROJECT_NAME}.KPropertiesDialog.desktop @ONLY)
	CONFIGURE_FILE(${PROJECT_NAME}.ThumbCreator.desktop.in      ${PROJECT_NAME}.ThumbCreator.desktop @ONLY)

	# FIXME: KDE4's installation directories have the prefix
	# hard-coded. This should be removed.
	INSTALL(TARGETS ${PROJECT_NAME} ${XATTRVIEW_NAME}
		LIBRARY DESTINATION "${KDE4_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.KPropertiesDialog.desktop"
		      "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.ThumbCreator.desktop"
		      "${XATTRVIEW_NAME}.desktop"
		DESTINATION "${KDE4_SERVICES_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca4?

	# Service menus
	INSTALL(FILES ../servicemenus/rp-convert-to-png.desktop
		DESTINATION "${KDE4_SERVICES_INSTALL_DIR}/ServiceMenus"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		COMPONENT "plugin")

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KDE4_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)

		GET_TARGET_PROPERTY(DEBUG_FILENAME ${XATTRVIEW_NAME} PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${KDE4_PLUGIN_INSTALL_DIR}"
				COMPONENT "debug")
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE4)
