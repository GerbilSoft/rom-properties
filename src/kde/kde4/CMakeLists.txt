PROJECT(rom-properties-kde4)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

# Find Qt4.
SET(QT4_NO_LINK_QTMAIN 1)
FIND_PACKAGE(Qt4 4.6.0 COMPONENTS QtCore QtGui)
IF(QT4_FOUND)
	# Find KDE4. (TODO: Version?)
	FIND_PACKAGE(KDE4)
	IF(NOT KDE4_FOUND)
		# KDE4 not found.
		SET(BUILD_KDE4 OFF CACHE "" INTERNAL FORCE)
	ENDIF(NOT KDE4_FOUND)
ELSE(QT4_FOUND)
	# Qt4 not found.
	SET(BUILD_KDE4 OFF CACHE "" INTERNAL FORCE)
ENDIF(QT4_FOUND)

# Sources and headers. (common)
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_SRCS "${rom-properties-kde_SRCS}")
STRING(REGEX REPLACE "([^;]+)" "../\\1" rom-properties-kde4_H "${rom-properties-kde_H}")

# Sources and headers. (KDE4-specific)
SET(rom-properties-kde4_SRCS
	${rom-properties-kde4_SRCS}
	RomPropertiesDialogPluginFactoryKDE4.cpp
	)

#####################
# Build the plugin. #
#####################

IF(BUILD_KDE4)
	KDE4_ADD_PLUGIN(rom-properties-kde4
		${rom-properties-kde4_SRCS}
		${rom-properties-kde4_H}
		)
	DO_SPLIT_DEBUG(rom-properties-kde4)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4
		PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>
		)
	TARGET_LINK_LIBRARIES(rom-properties-kde4
		romdata16
		cachemgr16
		${KDE4_KFILE_LIBRARY}
		Qt4::QtGui Qt4::QtCore
		)
	TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PUBLIC ${KDE4_INCLUDE_DIR})
ENDIF(BUILD_KDE4)

# Define -DQT_NO_DEBUG in release builds.
SET(CMAKE_C_FLAGS_RELEASE   "-DQT_NO_DEBUG ${CMAKE_C_FLAGS_RELEASE}")
SET(CMAKE_CXX_FLAGS_RELEASE "-DQT_NO_DEBUG ${CMAKE_CXX_FLAGS_RELEASE}")

# Qt options:
# - Fast QString concatenation. (Qt 4.6+, plus 4.8-specific version)
# - Disable implicit QString ASCII casts.
# - Use strict iterators.
ADD_DEFINITIONS(-DQT_USE_FAST_CONCATENATION
	-DQT_USE_FAST_OPERATOR_PLUS
	-DQT_USE_QSTRINGBUILDER
	-DQT_NO_CAST_FROM_ASCII
	-DQT_NO_CAST_TO_ASCII
	-DQT_STRICT_ITERATORS
	)

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE4)
	# FIXME: Determine the correct plugin path.
	# NOTE: Debian multiarch still uses /usr/lib/kde4.
	# Fedora 64-bit uses /usr/lib64/kde4. (/usr/lib is 32-bit)
	SET(KDE4_PLUGIN_DIRECTORY "lib/kde4")
	INSTALL(TARGETS rom-properties-kde4
		LIBRARY DESTINATION ${KDE4_PLUGIN_DIRECTORY}
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom-properties-kde4.desktop"
		DESTINATION "share/kde4/services"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca4.

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties-kde4 PDB)
		IF(DEBUG_FILENAME)
			INSTALL(FILES ${DEBUG_FILENAME}
				DESTINATION "lib/debug/${CMAKE_INSTALL_PREFIX}/${KDE4_PLUGIN_DIRECTORY}"
				COMPONENT "debug"
				)
		ENDIF(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(BUILD_KDE4)
