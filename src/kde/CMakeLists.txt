PROJECT(rom-properties-kde)

# Common files for both KDE4 and KDE5.

# Sources and headers.
SET(rom-properties-kde_SRCS
	RomPropertiesDialogPlugin.cpp
	RomDataView.cpp
	RomThumbCreator.cpp
	RpQt.cpp
	RpQImageBackend.cpp
	QTreeWidgetOpt.cpp
	config/stub-export.cpp
	config/ConfigDialog.cpp
	config/ITab.cpp
	config/ImageTypesTab.cpp
	config/DownloadsTab.cpp
	config/AboutTab.cpp
	config/QTreeViewOpt.cpp
	config/KeyStoreQt.cpp
	config/KeyStoreModel.cpp
	config/KeyStoreItemDelegate.cpp
	config/MessageWidget.cpp
	config/MessageWidgetStack.cpp
	)
SET(rom-properties-kde_H
	RomPropertiesDialogPlugin.hpp
	RomDataView.hpp
	RomThumbCreator.hpp
	RpQt.hpp
	RpQImageBackend.hpp
	QTreeWidgetOpt.hpp
	QImageData_qt4.hpp
	config/ConfigDialog.hpp
	config/ITab.hpp
	config/ImageTypesTab.hpp
	config/DownloadsTab.hpp
	config/AboutTab.hpp
	config/QTreeViewOpt.hpp
	config/KeyStoreQt.hpp
	config/KeyStoreModel.hpp
	config/KeyStoreItemDelegate.hpp
	config/MessageWidget.hpp
	config/MessageWidgetStack.hpp
	)
SET(rom-properties-kde_UIS
	RomDataView.ui
	config/ConfigDialog.ui
	config/ImageTypesTab.ui
	config/DownloadsTab.ui
	config/AboutTab.ui
	)

IF(ENABLE_DECRYPTION)
	SET(rom-properties-kde_SRCS ${rom-properties-kde_SRCS} config/KeyManagerTab.cpp)
	SET(rom-properties-kde_H    ${rom-properties-kde_H}    config/KeyManagerTab.hpp)
	SET(rom-properties-kde_UIS  ${rom-properties-kde_UIS}  config/KeyManagerTab.ui)
ENDIF(ENABLE_DECRYPTION)

IF(ENABLE_NLS)
	SET(rom-properties-kde_SRCS ${rom-properties-kde_SRCS} GettextTranslator.cpp)
	SET(rom-properties-kde_H    ${rom-properties-kde_H}    GettextTranslator.hpp)
ENDIF(ENABLE_NLS)

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
