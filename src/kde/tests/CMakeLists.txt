# KDE UI frontend test suite
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(kde-tests LANGUAGES CXX)

# Top-level src directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../..)

# KDE source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

# NOTE: Tests are currently only built for a single Qt version.
# Using Qt5 for now.
# TODO: Build for all selected Qt versions?

# TODO: The Qt5 check is used in both libunixcommon and the kf5/ subdir as well.
# Split it out into a .cmake file.

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

IF(BUILD_KF5)

# ListDataSortProxyModel test
ADD_EXECUTABLE(ListDataSortProxyModelTest
	ListDataSortProxyModelTest.cpp
	../ListDataModel.cpp
	../ListDataModel.hpp
	../ListDataSortProxyModel.cpp
	../ListDataSortProxyModel.hpp
	../RomDataFormat.cpp
	../RomDataFormat.hpp
	../RpQImageBackend.cpp
	../RpQImageBackend.hpp
	../RpQUrl.cpp
	../RpQUrl.hpp
	../RpQt.cpp
	../RpQt.hpp
	)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest PRIVATE rptest romdata)
TARGET_LINK_LIBRARIES(ListDataSortProxyModelTest PUBLIC Qt5::Widgets Qt5::Gui Qt5::Core)
#TARGET_INCLUDE_DIRECTORIES(ListDataSortProxyModelTest PRIVATE ${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
#TARGET_COMPILE_DEFINITIONS(ListDataSortProxyModelTest PRIVATE ${ZLIB_DEFINITIONS} ${PNG_DEFINITIONS})
DO_SPLIT_DEBUG(ListDataSortProxyModelTest)
SET_WINDOWS_SUBSYSTEM(ListDataSortProxyModelTest CONSOLE)
SET_WINDOWS_ENTRYPOINT(ListDataSortProxyModelTest wmain OFF)
ADD_TEST(NAME ListDataSortProxyModelTest COMMAND ListDataSortProxyModelTest --gtest_brief)

ENDIF(BUILD_KF5)
