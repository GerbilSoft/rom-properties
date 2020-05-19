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
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_H "${rom-properties-kde_H}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_UIS "${rom-properties-kde_UIS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_RCC_SRCS "${rom-properties-kde_RCC_SRCS}")

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/../config.kde.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.kde.h")

# Sources and headers. (KDE4-specific)
IF(QT4_FOUND)
	QT4_ADD_RESOURCES(
		rom-properties-kde4_RCC_O ${rom-properties-kde4_RCC_SRCS}
		OPTIONS -no-compress
		)
ENDIF(QT4_FOUND)
IF(HAVE_QtDBus_NOTIFY)
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4-notify_SRCS "${rom-properties-kde-notify_SRCS}")
	STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4-notify_H    "${rom-properties-kde-notify_H}")
ENDIF(HAVE_QtDBus_NOTIFY)

SET(rom-properties-kde4_SRCS
	${rom-properties-kde4_SRCS}
	${rom-properties-kde4-notify_SRCS}
	${rom-properties-kde4_RCC_O}
	RomPropertiesDialogPluginFactoryKDE4.cpp
	)
SET(rom-properties-kde4_H
	${rom-properties-kde4_H}
	${rom-properties-kde4-notify_H}
	)

IF(COMMAND QT4_WRAP_UI)
	# Wrap UI files.
	QT4_WRAP_UI(rom-properties-kde4_UIS_H ${rom-properties-kde4_UIS})
	ADD_CUSTOM_TARGET(kde4_generate_ui_h DEPENDS ${rom-properties-kde4_UIS_H})
ENDIF(COMMAND QT4_WRAP_UI)

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(rom-properties-kde4_PCH_CXX ../stdafx.cpp)
	SET(rom-properties-kde4_PCH_SRC ${rom-properties-kde4_PCH_CXX})
	SET(rom-properties-kde4_PCH_H ../stdafx.h)
ENDIF(ENABLE_PCH)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE4)
	KDE4_ADD_PLUGIN(rom-properties-kde4
		${rom-properties-kde4_PCH_SRC} ${rom-properties-kde4_PCH_H}
		${rom-properties-kde4_SRCS}
		${rom-properties-kde4_H}
		${rom-properties-kde4_UIS_H}
		)
	IF(ENABLE_PCH)
		ADD_PRECOMPILED_HEADER(rom-properties-kde4 ${rom-properties-kde4_PCH_H}
			SOURCE_CXX ${rom-properties-kde4_PCH_CXX}
			)
	ENDIF(ENABLE_PCH)
	DO_SPLIT_DEBUG(rom-properties-kde4)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# kde4
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# kde4
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# kde
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>	# src
		)
	TARGET_LINK_LIBRARIES(rom-properties-kde4 PRIVATE romdata rpfile rpbase unixcommon)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(rom-properties-kde4 PRIVATE i18n)
	ENDIF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties-kde4 PRIVATE ${ZLIB_LIBRARY})
	TARGET_LINK_LIBRARIES(rom-properties-kde4 PUBLIC ${KDE4_KFILE_LIBRARY} ${KDE4_KDEUI_LIBRARY})
	TARGET_LINK_LIBRARIES(rom-properties-kde4 PUBLIC Qt4::QtGui Qt4::QtCore)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PUBLIC ${KDE4_INCLUDE_DIR})
	IF(HAVE_QtDBus_NOTIFY)
		TARGET_LINK_LIBRARIES(rom-properties-kde4 PUBLIC Qt4::QtDBus)
	ENDIF(HAVE_QtDBus_NOTIFY)

	# Add zlib and png directories for the About tab.
	IF(HAVE_ZLIB)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PRIVATE ${ZLIB_INCLUDE_DIRS})
	ENDIF(HAVE_ZLIB)
	IF(HAVE_PNG)
		TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PRIVATE ${PNG_INCLUDE_DIRS})
	ENDIF(HAVE_PNG)
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
