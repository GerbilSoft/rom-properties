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

# Qt options:
# - Fast QString concatenation. (Qt 4.6+, plus 4.8-specific version)
# - Disable implicit QString ASCII casts.
# - Use strict iterators.
# - Disable implicit cast from QString to QUrl.
ADD_DEFINITIONS(-DQT_USE_FAST_CONCATENATION
	-DQT_USE_FAST_OPERATOR_PLUS
	-DQT_USE_QSTRINGBUILDER
	-DQT_NO_CAST_FROM_ASCII
	-DQT_NO_CAST_TO_ASCII
	-DQT_STRICT_ITERATORS
	-DQT_NO_URL_CAST_FROM_STRING
	)

IF(BUILD_KDE4)
	ADD_SUBDIRECTORY(kde4)
ENDIF(BUILD_KDE4)
IF(BUILD_KDE5)
	ADD_SUBDIRECTORY(kde5)
ENDIF(BUILD_KDE5)
