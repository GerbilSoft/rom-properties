PROJECT(rom-properties-kde)

# Common files for both KDE4 and KDE5.

# Sources and headers.
SET(rom-properties-kde_SRCS
	RomPropertiesDialogPlugin.cpp
	RomDataView.cpp
	RomThumbCreator.cpp
	RpQt.cpp
	RpQImageBackend.cpp
	)
SET(rom-properties-kde_H
	RomPropertiesDialogPlugin.hpp
	RomDataView.hpp
	RomThumbCreator.hpp
	RpQt.hpp
	RpQImageBackend.hpp
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
