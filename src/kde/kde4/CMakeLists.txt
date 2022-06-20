# KDE 4.x UI frontend
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-kde4 LANGUAGES CXX)

# Find Qt4.
SET(ENV{QT_SELECT} qt4)
SET(QT4_NO_LINK_QTMAIN 1)
FIND_PACKAGE(Qt4 4.6.0 ${REQUIRE_KDE4} COMPONENTS QtCore QtGui QtDBus)
IF(QT4_FOUND)
	# Make sure KDE4's CMake files don't create an uninstall rule.
	SET(_kde4_uninstall_rule_created TRUE)

	# Find KDE4.
	# - 4.7.0: KMessageWidget
	FIND_PACKAGE(KDE4 4.7.0 ${REQUIRE_KDE4})
	IF(NOT KDE4_FOUND)
		# KDE4 not found.
		SET(BUILD_KDE4 OFF CACHE INTERNAL "Build the KDE4 plugin." FORCE)
	ENDIF(NOT KDE4_FOUND)

	# Get rid of the explicit C90 setting.
	STRING(REPLACE "-std=iso9899:1990" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

	IF(ENABLE_ACHIEVEMENTS AND QT_QTDBUS_LIBRARY)
		# QtDBus is used for notifications.
		# TODO: Make notifications optional.
		SET(HAVE_QtDBus_NOTIFY 1)
	ENDIF(ENABLE_ACHIEVEMENTS AND QT_QTDBUS_LIBRARY)
ELSE(QT4_FOUND)
	# Qt4 not found.
	SET(BUILD_KDE4 OFF CACHE INTERNAL "Build the KDE4 plugin." FORCE)
ENDIF(QT4_FOUND)

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_H "${rom-properties-kde_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_UIS "${rom-properties-kde_UIS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}_RCC_SRCS "${rom-properties-kde_RCC_SRCS}")

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/../config.kde.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.kde.h")

# Sources and headers. (KDE4-specific)
IF(QT4_FOUND)
	QT4_ADD_RESOURCES(
		${PROJECT_NAME}_RCC_O ${${PROJECT_NAME}_RCC_SRCS}
		OPTIONS -no-compress
		)
ENDIF(QT4_FOUND)
IF(HAVE_QtDBus_NOTIFY)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-kde-notify_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-kde-notify_H}")

	# QtDBus wrappers
	QT4_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_SRCS
		"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.Notifications.xml"
		)
ENDIF(HAVE_QtDBus_NOTIFY)

SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS}
	${${PROJECT_NAME}-notify_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_SRCS}
	${${PROJECT_NAME}_RCC_O}
	RomPropertiesDialogPluginFactoryKDE4.cpp
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${${PROJECT_NAME}-notify_H}
	)

IF(COMMAND QT4_WRAP_UI)
	# Wrap UI files.
	QT4_WRAP_UI(${PROJECT_NAME}_UIS_H ${${PROJECT_NAME}_UIS})
	ADD_CUSTOM_TARGET(kde4_generate_ui_h DEPENDS ${${PROJECT_NAME}_UIS_H})
ENDIF(COMMAND QT4_WRAP_UI)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE4)
	KDE4_ADD_PLUGIN(${PROJECT_NAME}
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
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${KDE4_KFILE_LIBRARY} ${KDE4_KDEUI_LIBRARY})
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt4::QtGui Qt4::QtCore)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PUBLIC ${KDE4_INCLUDE_DIR})
	IF(HAVE_QtDBus_NOTIFY)
		TARGET_LINK_LIBRARIES(rom-properties-kde4 PUBLIC Qt4::QtDBus)
	ENDIF(HAVE_QtDBus_NOTIFY)
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
	CONFIGURE_FILE(rom-properties-kde4.KPropertiesDialog.desktop.in rom-properties-kde4.KPropertiesDialog.desktop @ONLY)
	CONFIGURE_FILE(rom-properties-kde4.ThumbCreator.desktop.in      rom-properties-kde4.ThumbCreator.desktop @ONLY)

	# FIXME: KDE4's installation directories have the prefix
	# hard-coded. This should be removed.
	INSTALL(TARGETS rom-properties-kde4
		LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/rom-properties-kde4.KPropertiesDialog.desktop"
		      "${CMAKE_CURRENT_BINARY_DIR}/rom-properties-kde4.ThumbCreator.desktop"
		DESTINATION "${SERVICES_INSTALL_DIR}"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca4.

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-kde4 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "lib/debug/${PLUGIN_INSTALL_DIR}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE4)
