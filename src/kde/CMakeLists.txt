PROJECT(rom-properties-kde)
# CMake 2.8.11 adds TARGET_INCLUDE_DIRECTORIES() and Qt4::xxx targets, similar to Qt5.
# CMake 2.8.12 automates adding compile flags for Qt5, e.g. -fPIC on Linux.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

# Common files for both KDE4 and KDE5.

# Sources and headers.
SET(rom-properties-kde_SRCS
	RomPropertiesDialogPlugin.cpp
	RomDataView.cpp
	RomThumbCreator.cpp
	RpQt.cpp
	)
SET(rom-properties-kde_H
	RomPropertiesDialogPlugin.hpp
	RomDataView.hpp
	RomThumbCreator.hpp
	RpQt.hpp
	)
IF(BUILD_KDE4)
	SET(rom-properties-kde4_SRCS
		RomPropertiesDialogPluginFactoryKDE4.cpp
		)
ENDIF(BUILD_KDE4)
IF(BUILD_KDE5)
	SET(rom-properties-kde5_SRCS
		RomPropertiesDialogPluginFactoryKDE5.cpp
		)
ENDIF(BUILD_KDE5)

IF(BUILD_KDE4)
	ADD_SUBDIRECTORY(kde4)
ENDIF(BUILD_KDE4)
IF(BUILD_KDE5)
	ADD_SUBDIRECTORY(kde5)
ENDIF(BUILD_KDE5)
