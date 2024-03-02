# KDE (Qt) UI frontends
PROJECT(rom-properties-kde LANGUAGES CXX)
SET(CMAKE_AUTOMOC ON)

# Common files for all KDE versions.

# Sources and headers.
SET(${PROJECT_NAME}_SRCS
	plugins/RomPropertiesDialogPlugin.cpp
	plugins/RomThumbCreator.cpp
	RomDataView.cpp
	RomDataView_ops.cpp
	RpQt.cpp
	RpQUrl.cpp
	RpQImageBackend.cpp
	DragImageLabel.cpp
	RpQByteArrayFile.cpp
	DragImageTreeView.cpp
	MessageSound.cpp
	ListDataModel.cpp
	ListDataSortProxyModel.cpp
	LanguageComboBox.cpp
	OptionsMenuButton.cpp
	ProxyForUrl.cpp
	RomDataFormat.cpp
	ISpriteSheet.cpp
	AchSpriteSheet.cpp
	FlagSpriteSheet.cpp
	config/stub-export.cpp
	config/ConfigDialog.cpp
	config/ITab.cpp
	config/ImageTypesTab.cpp
	config/SystemsTab.cpp
	config/OptionsTab.cpp
	config/CacheTab.cpp
	config/CacheCleaner.cpp
	config/RpProgressBar.cpp
	config/AchievementsTab.cpp
	config/AchievementsItemDelegate.cpp
	config/AboutTab.cpp
	config/UpdateChecker.cpp
	)
SET(${PROJECT_NAME}_H
	plugins/RomPropertiesDialogPlugin.hpp
	plugins/RomThumbCreator.hpp
	RomDataView.hpp
	RomDataView_p.hpp
	RpQt.hpp
	RpQtNS.hpp
	RpQUrl.hpp
	RpQImageBackend.hpp
	QImageData_qt4.hpp
	DragImageLabel.hpp
	RpQByteArrayFile.hpp
	DragImageTreeView.hpp
	MessageSound.hpp
	ListDataModel.hpp
	ListDataSortProxyModel.hpp
	LanguageComboBox.hpp
	OptionsMenuButton.hpp
	ProxyForUrl.hpp
	check-uid.hpp
	RomDataFormat.hpp
	ISpriteSheet.hpp
	AchSpriteSheet.hpp
	FlagSpriteSheet.hpp
	config/ConfigDialog.hpp
	config/ITab.hpp
	config/ImageTypesTab.hpp
	config/SystemsTab.hpp
	config/OptionsTab.hpp
	config/CacheTab.hpp
	config/CacheCleaner.hpp
	config/RpProgressBar.hpp
	config/AchievementsTab.hpp
	config/AchievementsItemDelegate.hpp
	config/AboutTab.hpp
	config/UpdateChecker.hpp
	)
SET(${PROJECT_NAME}_UIS
	RomDataView.ui
	config/ConfigDialog.ui
	config/ImageTypesTab.ui
	config/SystemsTab.ui
	config/OptionsTab.ui
	config/CacheTab.ui
	config/AchievementsTab.ui
	config/AboutTab.ui
	)
SET(${PROJECT_NAME}_RCC_SRCS
	../res/flags/flags.qrc
	../res/ach/ach.qrc
	)

# XAttrView plugin
SET(${PROJECT_NAME}_XATTR_SRCS
	RpQUrl.cpp	# NOTE: duplicated here
	xattr/XAttrViewPropertiesDialogPlugin.cpp
	xattr/Ext2AttrView.cpp
	xattr/XfsAttrView.cpp
	xattr/DosAttrView.cpp
	xattr/XAttrView.cpp
	)
SET(${PROJECT_NAME}_XATTR_H
	RpQUrl.hpp	# NOTE: duplicated here
	xattr/XAttrViewPropertiesDialogPlugin.hpp
	xattr/Ext2AttrView.hpp
	xattr/XfsAttrView.hpp
	xattr/DosAttrView.hpp
	xattr/XAttrView.hpp
	)
SET(${PROJECT_NAME}_XATTR_UIS
	xattr/Ext2AttrView.ui
	xattr/XfsAttrView.ui
	xattr/DosAttrView.ui
	xattr/XAttrView.ui
	)

IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_SRCS
		${${PROJECT_NAME}_SRCS}
		config/KeyManagerTab.cpp
		config/KeyStoreQt.cpp
		config/KeyStoreModel.cpp
		config/KeyStoreItemDelegate.cpp
		)
	SET(${PROJECT_NAME}_H
		${${PROJECT_NAME}_H}
		config/KeyManagerTab.hpp
		config/KeyStoreQt.hpp
		config/KeyStoreModel.hpp
		config/KeyStoreItemDelegate.hpp
		)
	SET(${PROJECT_NAME}_UIS
		${${PROJECT_NAME}_UIS}
		config/KeyManagerTab.ui
		)
ENDIF(ENABLE_DECRYPTION)

IF(ENABLE_NLS)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} GettextTranslator.cpp)
	SET(${PROJECT_NAME}_H    ${${PROJECT_NAME}_H}    GettextTranslator.hpp)
ENDIF(ENABLE_NLS)

IF(ENABLE_ACHIEVEMENTS)
	# D-Bus notification for achievements
	SET(${PROJECT_NAME}-notify_SRCS AchQtDBus.cpp)
	SET(${PROJECT_NAME}-notify_H    AchQtDBus.hpp)
ENDIF(ENABLE_ACHIEVEMENTS)

# RpFile_kio requires KDE Frameworks 5.
# FIXME: Dolphin ends up hanging for some reason...
#SET(rom-properties-kio_SRCS RpFile_kio.cpp)
#SET(rom-properties-kio_H RpFile_kio.hpp)

# Qt options:
# - Fast QString concatenation. (Qt 4.6+, plus 4.8-specific version)
# - Disable implicit QString ASCII casts.
# - Use strict iterators.
# - Disable implicit cast from QString to QUrl.
# - Use lean headers. (Qt6)
ADD_DEFINITIONS(-DQT_USE_FAST_CONCATENATION
	-DQT_USE_FAST_OPERATOR_PLUS
	-DQT_USE_QSTRINGBUILDER
	-DQT_NO_CAST_FROM_ASCII
	-DQT_NO_CAST_TO_ASCII
	-DQT_STRICT_ITERATORS
	-DQT_NO_URL_CAST_FROM_STRING
	-DQT_NO_CAST_FROM_BYTEARRAY
	-DQT_NO_FOREACH
	-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
	-DQT_LEAN_HEADERS=1	# Qt6
	)

### FIXME: The following Qt declarations are being picked up by libromdata and rpcli...

# Disable deprecated Qt and KDE functionality in debug builds.
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_DISABLE_DEPRECATED_BEFORE=0x050E01 -DQT_DEPRECATED_WARNINGS_SINCE=0x060000")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055200 -DKF_DEPRECATED_WARNINGS_SINCE=0x060000")

# Define -DQT_NO_DEBUG in release builds.
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DQT_NO_DEBUG")

IF(BUILD_KDE4)
	ADD_SUBDIRECTORY(kde4)
ENDIF(BUILD_KDE4)
IF(BUILD_KF5)
	ADD_SUBDIRECTORY(kf5)
ENDIF(BUILD_KF5)
IF(BUILD_KF6)
	ADD_SUBDIRECTORY(kf6)
ENDIF(BUILD_KF6)

IF(BUILD_KF5 OR BUILD_KF6)
	# Service menus
	# KF5 and KF6 use the same servicemenus directory.
	# NOTE: KF5 <5.85 uses ${SERVICES_INSTALL_DIR} instead.
	# That will be handled by the KF5 subdirectory.
	# TODO: How does KF5 handle it if the .desktop file is in both locations?
	INSTALL(FILES servicemenus/rp-convert-to-png.desktop
		DESTINATION "${CMAKE_INSTALL_PREFIX}/share/kio/servicemenus"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		COMPONENT "plugin")
ENDIF(BUILD_KF5 OR BUILD_KF6)

IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
