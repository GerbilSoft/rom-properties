PROJECT(rom-properties-kde)
# CMake 2.8.11 adds Qt4::xxx targets, similar to Qt5.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

IF(BUILD_KDE4)
	# Find Qt4.
	FIND_PACKAGE(Qt4 4.6.0 REQUIRED COMPONENTS QtCore QtGui)
	# Find KDE4. (TODO: Version?)
	FIND_PACKAGE(KDE4 REQUIRED)
ENDIF(BUILD_KDE4)

# TODO: Check for iconv on non-Windows systems.
# NOTE: KDE4_ADD_PLUGIN() uses automoc.

# Sources and headers.
SET(rom-properties-kde_SRCS
	RomPropertiesDialogPlugin.cpp
	)
SET(rom-properties-kde_H
	RomPropertiesDialogPlugin.hpp
	)

# UI files.
#SET(rom-properties-kde_UIS
	#MegaDrive.ui
#	)
#QT4_WRAP_UI(rom-properties-kde4_UIS_H ${rom-properties-kde_UIS})

#####################
# Build the plugin. #
#####################

KDE4_ADD_PLUGIN(rom-properties-kde4
	${rom-properties-kde_SRCS}
	${rom-properties-kde_H}
	#${rom-properties-kde4_UIS_H}
	)
TARGET_LINK_LIBRARIES(rom-properties-kde4
	${KDE4_KFILE_LIBRARY}
	Qt4::QtGui Qt4::QtCore
	)
TARGET_INCLUDE_DIRECTORIES(rom-properties-kde4 PUBLIC ${KDE4_INCLUDE_DIR})

# Qt options:
# - Fast QString concatenation. (Qt 4.6+, plus 4.8-specific version)
# - Disable implicit QString ASCII casts.
ADD_DEFINITIONS(-DQT_USE_FAST_CONCATENATION
	-DQT_USE_FAST_OPERATOR_PLUS
	-DQT_USE_QSTRINGBUILDER
	-DQT_NO_CAST_FROM_ASCII
	-DQT_NO_CAST_TO_ASCII
	)

#######################
# Install the plugin. #
#######################

IF(BUILD_KDE4)
	# FIXME: Determine lib, lib64, lib/[triplet]
	# For now, assume lib.
	INSTALL(TARGETS rom-properties-kde4
		LIBRARY DESTINATION "lib/kde4"
		COMPONENT "plugin"
		)
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom-properties-kde4.desktop"
		DESTINATION "share/kde4/services"
		COMPONENT "plugin"
		)
	# FIXME: Run kbuildsycoca4.
ENDIF(BUILD_KDE4)
