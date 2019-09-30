PROJECT(rom-properties-kde)

# Common files for both KDE4 and KDE5.

# Sources and headers.
SET(rom-properties-kde_SRCS
	RomPropertiesDialogPlugin.cpp
	RomDataView.cpp
	RomThumbCreator.cpp
	RpQt.cpp
	RpQImageBackend.cpp
	DragImageLabel.cpp
	config/stub-export.cpp
	config/ConfigDialog.cpp
	config/ITab.cpp
	config/ImageTypesTab.cpp
	config/OptionsTab.cpp
	config/AboutTab.cpp
	)
SET(rom-properties-kde_H
	RomPropertiesDialogPlugin.hpp
	RomDataView.hpp
	RomThumbCreator.hpp
	RpQt.hpp
	RpQImageBackend.hpp
	QImageData_qt4.hpp
	DragImageLabel.hpp
	config/ConfigDialog.hpp
	config/ITab.hpp
	config/ImageTypesTab.hpp
	config/OptionsTab.hpp
	config/AboutTab.hpp
	)
SET(rom-properties-kde_UIS
	RomDataView.ui
	config/ConfigDialog.ui
	config/ImageTypesTab.ui
	config/OptionsTab.ui
	config/AboutTab.ui
	)

IF(ENABLE_DECRYPTION)
	SET(rom-properties-kde_SRCS
		${rom-properties-kde_SRCS}
		config/KeyManagerTab.cpp
		config/KeyStoreQt.cpp
		config/KeyStoreModel.cpp
		config/KeyStoreItemDelegate.cpp
		)
	SET(rom-properties-kde_H
		${rom-properties-kde_H}
		config/KeyManagerTab.hpp
		config/KeyStoreQt.hpp
		config/KeyStoreModel.hpp
		config/KeyStoreItemDelegate.hpp
		)
	SET(rom-properties-kde_UIS
		${rom-properties-kde_UIS}
		config/KeyManagerTab.ui
		)
ENDIF(ENABLE_DECRYPTION)

IF(ENABLE_NLS)
	SET(rom-properties-kde_SRCS ${rom-properties-kde_SRCS} GettextTranslator.cpp)
	SET(rom-properties-kde_H    ${rom-properties-kde_H}    GettextTranslator.hpp)
ENDIF(ENABLE_NLS)

# NOTE: RpExtractorPlugin cannot use common source because
# Qt's moc doesn't support preprocessor directores like #ifdef.

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

# Disable deprecated Qt functionality in debug builds.
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_DISABLE_DEPRECATED_BEFORE=0x050d01")

# Define -DQT_NO_DEBUG in release builds.
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG")

IF(BUILD_KDE4)
	ADD_SUBDIRECTORY(kde4)
ENDIF(BUILD_KDE4)
IF(BUILD_KDE5)
	ADD_SUBDIRECTORY(kde5)
ENDIF(BUILD_KDE5)
