# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf5/
# Find Qt5 and KF5.
MACRO(FIND_QT5_AND_KF5)
	# FIXME: KF5 is overwriting CMAKE_LIBRARY_OUTPUT_DIRECTORY and CMAKE_INSTALL_LIBDIR.
	# NOTE: CMAKE_LIBRARY_OUTPUT_DIRECTORY might only be overwritten by KF6...
	SET(TMP_CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
	SET(TMP_CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")

	SET(ENV{QT_SELECT} qt5)
	SET(QT_DEFAULT_MAJOR_VERSION 5)

	# FIXME: Search for Qt5 first instead of ECM?

	# Find KF5 Extra CMake Modules.
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
			FIND_PROGRAM(QTPATHS5 NAMES qtpaths5 qtpaths
				PATHS /usr/local/lib/qt5/bin	# FreeBSD
				)
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
			FIND_PACKAGE(KF5 ${REQUIRE_KF5} COMPONENTS CoreAddons KIO WidgetsAddons FileMetaData Crash)
			IF(NOT KF5CoreAddons_FOUND OR NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND OR NOT KF5Crash_FOUND)
				# KF5 not found.
				SET(BUILD_KF5 OFF CACHE INTERNAL "Build the KDE Frameworks 5 plugin." FORCE)
			ENDIF(NOT KF5CoreAddons_FOUND OR NOT KF5KIO_FOUND OR NOT KF5WidgetsAddons_FOUND OR NOT KF5FileMetaData_FOUND OR NOT KF5Crash_FOUND)

			# CoreAddons: If earlier than 5.85, install service menus in ${SERVICES_INSTALL_DIR}.
			IF(TARGET KF5::CoreAddons AND KF5CoreAddons_VERSION VERSION_LESS 5.84.79)
				SET(HAVE_KF5_DEPRECATED_SERVICE_MENU_DIR 1)
			ENDIF(TARGET KF5::CoreAddons AND KF5CoreAddons_VERSION VERSION_LESS 5.84.79)

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

	# FIXME: KF5 is overwriting CMAKE_LIBRARY_OUTPUT_DIRECTORY and CMAKE_INSTALL_LIBDIR.
	# NOTE: CMAKE_LIBRARY_OUTPUT_DIRECTORY might only be overwritten by KF6...
	SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${TMP_CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
	SET(CMAKE_INSTALL_LIBDIR ${TMP_CMAKE_INSTALL_LIBDIR})
	UNSET(TMP_CMAKE_LIBRARY_OUTPUT_DIRECTORY)
	UNSET(TMP_CMAKE_INSTALL_LIBDI)
ENDMACRO(FIND_QT5_AND_KF5)
