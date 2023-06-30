# KDE Frameworks 6.x UI frontend
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-kf6 LANGUAGES CXX)

# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf6/
# Find KF6 Extra CMake Modules.
SET(ENV{QT_SELECT} qt6)
FIND_PACKAGE(ECM ${REQUIRE_KF6} 0.0.11 NO_MODULE)
IF(ECM_MODULE_PATH AND ECM_KDE_MODULE_DIR)
	# Make sure ECM's CMake files don't create an uninstall rule.
	SET(KDE_SKIP_UNINSTALL_TARGET TRUE)

	# Don't add KDE tests to the CTest build.
	SET(KDE_SKIP_TEST_SETTINGS TRUE)

	# Include KF6 CMake modules.
	LIST(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
	INCLUDE(KDEInstallDirs)
	INCLUDE(KDECMakeSettings)

	# Qt6 requires "-fpic -fPIC" due to reduced relocations.
	SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fPIC")

	# Find Qt6.
	SET(Qt6_NO_LINK_QTMAIN 1)
	FIND_PACKAGE(Qt6 ${REQUIRE_KF6} COMPONENTS Core Gui Widgets DBus)
	IF(Qt6Core_FOUND AND Qt6Gui_FOUND AND Qt6Widgets_FOUND)
		# NOTE: QT_PLUGIN_INSTALL_DIR is missing the 'qt6' directory.
		# Use `qtpaths6` instead to get the actual path.
		#
		# Ubuntu:
		# - Expected: lib/${DEB_HOST_MULTIARCH}/qt6/plugins
		# - Actual:   lib/${DEB_HOST_MULTIARCH}/plugins
		#
		# Gentoo:
		# - Expected: lib64/qt6/plugins
		# - Actual:   lib64/plugins
		#
		# Arch:
		# - Expected: lib/qt/plugins
		# - Actual:   (FIXME)
		#

		# Find the qtpaths6 executable.
		FIND_PROGRAM(QTPATHS6 NAMES qtpaths6 qtpaths)
		IF(NOT QTPATHS6)
			MESSAGE(FATAL_ERROR "qtpaths6 not found. Install one of these packages:
  - Debian/Ubuntu: qttools6-dev-tools
  - Red Hat/Fedora: qt6-qttools")
		ENDIF(NOT QTPATHS6)

		# Get the plugin directory and Qt prefix.
		# Prefix will be removed from the plugin directory if necessary.
		EXEC_PROGRAM(${QTPATHS6} ARGS --plugin-dir OUTPUT_VARIABLE KF6_PLUGIN_INSTALL_DIR)
		IF(NOT KF6_PLUGIN_INSTALL_DIR)
			MESSAGE(FATAL_ERROR "`qtpaths6` isn't working correctly.")
		ENDIF(NOT KF6_PLUGIN_INSTALL_DIR)
		# FIXME: Mageia has the Qt path set to "/usr/lib64/qt6" instead of "/usr".
		# Reference: https://github.com/GerbilSoft/rom-properties/issues/69
		INCLUDE(ReplaceHardcodedPrefix)
		REPLACE_HARDCODED_PREFIX(KF6_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
		SET(QT_PLUGIN_INSTALL_DIR "${KF6_PLUGIN_INSTALL_DIR}")

		### FIXME: KF6 isn't available yet, so we're only
		### compile-testing against Qt6.

		# Find KF6.
		FIND_PACKAGE(KF6 ${REQUIRE_KF6} COMPONENTS KIO WidgetsAddons FileMetaData)
		IF(NOT KF6KIO_FOUND OR NOT KF6WidgetsAddons_FOUND OR NOT KF6FileMetaData_FOUND)
			# KF6 not found.
			SET(BUILD_KF6 OFF CACHE INTERNAL "Build the KDE Frameworks 6 plugin." FORCE)
		ENDIF(NOT KF6KIO_FOUND OR NOT KF6WidgetsAddons_FOUND OR NOT KF6FileMetaData_FOUND)

		# KIO::ThumbnailCreator is always available in KF6.
		SET(HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H 1)

		SET(KF6_PRPD_PLUGIN_INSTALL_DIR  "${KF6_PLUGIN_INSTALL_DIR}/kf6/propertiesdialog")
		SET(KF6_KFMD_PLUGIN_INSTALL_DIR  "${KF6_PLUGIN_INSTALL_DIR}/kf6/kfilemetadata")
		SET(KF6_KOVI_PLUGIN_INSTALL_DIR  "${KF6_PLUGIN_INSTALL_DIR}/kf6/overlayicon")
		SET(KF6_THUMB_PLUGIN_INSTALL_DIR "${KF6_PLUGIN_INSTALL_DIR}/kf6/thumbcreator")

		IF(Qt6DBus_FOUND)
			SET(HAVE_QtDBus 1)
			IF(ENABLE_ACHIEVEMENTS)
				# QtDBus is used for notifications.
				# TODO: Make notifications optional.
				SET(HAVE_QtDBus_NOTIFY 1)
			ENDIF(ENABLE_ACHIEVEMENTS)
		ENDIF(Qt6DBus_FOUND)
	ELSE()
		# Qt6 not found.
		SET(BUILD_KF6 OFF CACHE INTERNAL "Build the KDE Frameworks 6 plugin." FORCE)
	ENDIF()
ELSE()
	# KF6 Extra CMake Modules not found.
	SET(BUILD_KF6 OFF CACHE INTERNAL "Build the KDE Frameworks 6 plugin." FORCE)
ENDIF()

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
	QT6_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_SRCS
		"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.NetworkManager.xml"
		)
	IF(HAVE_QtDBus_NOTIFY)
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_SRCS "${rom-properties-kde-notify_SRCS}")
		STRING(REGEX REPLACE "([^;]+)" "../\\1" ${PROJECT_NAME}-notify_H    "${rom-properties-kde-notify_H}")
		QT6_ADD_DBUS_INTERFACES(${PROJECT_NAME}_DBUS_IFACE_SRCS_2
			"${CMAKE_CURRENT_SOURCE_DIR}/../../dbus/org.freedesktop.Notifications.xml"
			)
	ENDIF(HAVE_QtDBus_NOTIFY)
ENDIF(HAVE_QtDBus)

# Main plugin
SET(${PROJECT_NAME}_SRCS
	${${PROJECT_NAME}_SRCS}
	${rom-properties-kio6_SRCS}
	${${PROJECT_NAME}-notify_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_SRCS}
	${${PROJECT_NAME}_DBUS_IFACE_SRCS_2}
	${${PROJECT_NAME}_RCC_O}
	PluginFactoryKF6.cpp
	../ExtractorPlugin.cpp
	../OverlayIconPlugin.cpp
	)
SET(${PROJECT_NAME}_H
	${${PROJECT_NAME}_H}
	${rom-properties-kio6_H}
	${${PROJECT_NAME}-notify_H}
	../ExtractorPlugin.hpp
	../OverlayIconPlugin.hpp
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
			"${RP_CONFIG_LIBC_H}" "${RP_CXX11_COMPAT_H}"
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
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC KF6::KIOGui KF6::KIOCore KF6::KIOWidgets KF6::WidgetsAddons KF6::FileMetaData)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core)
	IF(HAVE_QtDBus)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC Qt6::DBus)
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
	#		"${RP_CONFIG_LIBC_H}" "${RP_CXX11_COMPAT_H}"
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
	# NOTE: KF6 is still using the KF6 namespace for now.
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PRIVATE KF6::KIOCore KF6::KIOWidgets)
	TARGET_LINK_LIBRARIES(${XATTRVIEW_NAME} PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core)

	### Plugin Forwarders ###

	# ExtractorPluginForwarder
	ADD_LIBRARY(kfilemetadata_${PROJECT_NAME} MODULE
		ExtractorPluginForwarder.cpp
		ExtractorPluginForwarder.hpp
		)
	SET_TARGET_PROPERTIES(kfilemetadata_${PROJECT_NAME} PROPERTIES PREFIX "")
	DO_SPLIT_DEBUG(kfilemetadata_${PROJECT_NAME})
	# NOTE: KF6 is still using the KF6 namespace for now.
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE KF6::KIOCore KF6::FileMetaData)
	TARGET_LINK_LIBRARIES(kfilemetadata_${PROJECT_NAME} PRIVATE Qt6::Core)

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
	# NOTE: KF6 is still using the KF6 namespace for now.
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE KF6::KIOCore KF6::KIOWidgets)
	TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE Qt6::Core)

	# Link in libdl if it's required for dlopen().
	IF(CMAKE_DL_LIBS)
		TARGET_LINK_LIBRARIES(overlayiconplugin_${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_DL_LIBS)

	# FIXME: Workaround for gcc-5.4.0 LTO bug.
	# Disabling LTO for the kf6 build if using gcc-5.4.0 or earlier,
	# and decryption is enabled. (It crashes when compiling the moc
	# file for KeyManagerTab.cpp.)
	# NOTE: We can't add it to the automoc.cpp file because then automoc
	# will try including itself, resulting in a failure.
	IF(GCC_5xx_LTO_ISSUES AND ENABLE_DECRYPTION)
		SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fno-lto")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-lto")
	ENDIF(GCC_5xx_LTO_ISSUES AND ENABLE_DECRYPTION)
ENDIF(BUILD_KF6)

#######################
# Install the plugin. #
#######################

IF(BUILD_KF6)
	# Process MIME types for the .json files.
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
	CONFIGURE_FILE(${PROJECT_NAME}.json.in ${PROJECT_NAME}.json @ONLY)
	CONFIGURE_FILE(ExtractorPluginForwarder.json.in ExtractorPluginForwarder.json @ONLY)
	CONFIGURE_FILE(OverlayIconPluginForwarder.json.in OverlayIconPluginForwarder.json @ONLY)

	# Main plugin is installed in the propertiesdialog subdirectory.
	# Create a symlink for ThumbCreator.
	ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ln -sf "${KF6_PRPD_PLUGIN_INSTALL_DIR}/${PROJECT_NAME}.so" "${PROJECT_NAME}.symlink.so"
		WORKING_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
		VERBATIM)
	INSTALL(TARGETS ${PROJECT_NAME} ${XATTRVIEW_NAME}
		LIBRARY DESTINATION "${KF6_PRPD_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	INSTALL(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PROJECT_NAME}.symlink.so"
		DESTINATION "${KF6_THUMB_PLUGIN_INSTALL_DIR}"
		RENAME "${PROJECT_NAME}.so"
		COMPONENT "plugin")

	INSTALL(TARGETS kfilemetadata_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF6_KFMD_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	INSTALL(TARGETS overlayiconplugin_${PROJECT_NAME}
		LIBRARY DESTINATION "${KF6_KOVI_PLUGIN_INSTALL_DIR}"
		COMPONENT "plugin")
	# FIXME: Run kbuildsycoca6?

	# Service menus
	# FIXME: Conflicts with KF6. Figure out how to fix this later.
	#INSTALL(FILES ../servicemenus/rp-convert-to-png.desktop
	#	DESTINATION "${CMAKE_INSTALL_PREFIX}/share/kio/servicemenus"
	#	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	#	COMPONENT "plugin")

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
