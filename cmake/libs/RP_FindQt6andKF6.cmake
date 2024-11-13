# Reference: http://www.proli.net/2014/06/21/porting-your-project-to-qt5kf6/
# Find Qt6 and KF6.
MACRO(FIND_QT6_AND_KF6)
	# FIXME: KF6 is overwriting CMAKE_LIBRARY_OUTPUT_DIRECTORY and CMAKE_INSTALL_LIBDIR.
	SET(TMP_CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
	SET(TMP_CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")

	SET(ENV{QT_SELECT} qt6)
	SET(QT_DEFAULT_MAJOR_VERSION 6)
	SET(QT_NO_CREATE_VERSIONLESS_TARGETS TRUE)

	# FIXME: Search for Qt6 first instead of ECM?
	SET(KF6_MIN 5.248.0)

	# Find KF6 Extra CMake Modules.
	FIND_PACKAGE(ECM ${REQUIRE_KF6} ${KF6_MIN} NO_MODULE)
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
			# - Actual:   lib/qt6/plugins
			#

			# Find the qtpaths6 executable.
			FIND_PROGRAM(QTPATHS6 NAMES qtpaths6 qtpaths
				PATHS /usr/local/lib/qt6/bin	# FreeBSD
				/usr/lib/qt6/bin	#Archlinux
				)
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

			# Find KF6.
			# FIXME: Specifying the minimum version here breaks on Kubuntu 24.10.
			#FIND_PACKAGE(KF6 ${REQUIRE_KF6} ${KF6_MIN} COMPONENTS KIO WidgetsAddons FileMetaData Crash)
			FIND_PACKAGE(KF6 ${REQUIRE_KF6} COMPONENTS KIO WidgetsAddons FileMetaData Crash)
			IF(NOT KF6KIO_FOUND OR NOT KF6WidgetsAddons_FOUND OR NOT KF6FileMetaData_FOUND OR NOT KF6Crash_FOUND)
				# KF6 not found.
				SET(BUILD_KF6 OFF CACHE INTERNAL "Build the KDE Frameworks 6 plugin." FORCE)
			ENDIF(NOT KF6KIO_FOUND OR NOT KF6WidgetsAddons_FOUND OR NOT KF6FileMetaData_FOUND OR NOT KF6Crash_FOUND)

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

	# FIXME: KF6 is overwriting CMAKE_LIBRARY_OUTPUT_DIRECTORY and CMAKE_INSTALL_LIBDIR.
	SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${TMP_CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
	SET(CMAKE_INSTALL_LIBDIR ${TMP_CMAKE_INSTALL_LIBDIR})
	UNSET(TMP_CMAKE_LIBRARY_OUTPUT_DIRECTORY)
	UNSET(TMP_CMAKE_INSTALL_LIBDI)
ENDMACRO(FIND_QT6_AND_KF6)
