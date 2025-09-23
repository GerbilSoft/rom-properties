# Windows UI frontend
PROJECT(rom-properties LANGUAGES C CXX)

# Check for Windows SDK headers and symbols.
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckCXXSymbolExists)
# IThumbnailProvider
CHECK_INCLUDE_FILES("thumbcache.h" HAVE_THUMBCACHE_H)
# Property key symbols and functions for RP_PropertyStore
CHECK_SYMBOL_EXISTS(PKEY_Subject "windows.h;propvarutil.h;propkey.h" HAVE_PKEY_SUBJECT)
CHECK_CXX_SYMBOL_EXISTS(InitPropVariantFromUInt16 "windows.h;propvarutil.h" HAVE_INITPROPVARIANTFROMUINT16)
IF(HAVE_PKEY_SUBJECT AND HAVE_INITPROPVARIANTFROMUINT16)
	SET(HAVE_RP_PROPERTYSTORE_DEPS 1)
ELSE()
	MESSAGE(WARNING "Windows SDK is missing required IPropertyStore symbols.\nIPropertyStore functionality will not be available.")
ENDIF()

# Check for std::ofstream::open(const wchar_t*, ios_base::openmode).
CHECK_CXX_SOURCE_COMPILES("
#include <fstream>
int main(void) {
	std::ofstream ofs;
	ofs.open(L\"wide-file.bin\");
	return 0;
}
"
HAVE_OFSTREAM_CTOR_WCHAR_T)
# Sanity check: Should always be present on MSVC.
IF(MSVC AND NOT HAVE_OFSTREAM_CTOR_WCHAR_T)
	MESSAGE(FATAL_ERROR "HAVE_OFSTREAM_CTOR_WCHAR_T failed on MSVC.")
ENDIF(MSVC AND NOT HAVE_OFSTREAM_CTOR_WCHAR_T)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.win32.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.win32.h")

# Create the manifest file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.dll.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/res/${PROJECT_NAME}.dll.manifest" @ONLY)

# Sources and headers.
SET(${PROJECT_NAME}_SRCS
	DllMain.cpp
	DllRegisterServer.cpp
	RP_ExtractIcon.cpp
	RP_ExtractIcon_Register.cpp
	RP_ExtractIcon_Fallback.cpp
	RP_ExtractImage.cpp
	RP_ExtractImage_Register.cpp
	RP_ExtractImage_Fallback.cpp
	RpImageWin32.cpp
	RP_ShellPropSheetExt.cpp
	RP_ShellPropSheetExt_ops.cpp
	RP_ShellPropSheetExt_Register.cpp
	RP_ThumbnailProvider.cpp
	RP_ThumbnailProvider_Register.cpp
	RP_ThumbnailProvider_Fallback.cpp
	RP_ShellIconOverlayIdentifier.cpp
	RP_ShellIconOverlayIdentifier_Register.cpp
	RP_ContextMenu.cpp
	RP_ContextMenu_Register.cpp
	RP_XAttrView.cpp
	RP_XAttrView_Register.cpp
	RP_ColumnProvider.cpp
	RP_ColumnProvider_Register.cpp
	CreateThumbnail.cpp
	DragImageLabel.cpp
	FontHandler.cpp
	MessageWidget.cpp
	LvData.cpp
	LanguageComboBox.cpp
	OptionsMenuButton.cpp
	RomDataFormat.cpp
	ISpriteSheet.cpp
	AchSpriteSheet.cpp
	FlagSpriteSheet.cpp
	config/ConfigDialog.cpp
	config/PropSheetIcon.cpp
	config/RP_EmptyVolumeCacheCallback.cpp
	config/ImageTypesTab.cpp
	config/SystemsTab.cpp
	config/OptionsTab.cpp
	config/CacheTab.cpp
	config/AchievementsTab.cpp
	config/AboutTab.cpp
	file/RpFile_IStream.cpp
	file/RpFile_windres.cpp
	)
SET(${PROJECT_NAME}_H
	RP_ExtractIcon.hpp
	RP_ExtractIcon_p.hpp
	RP_ClassFactory.hpp
	RP_ExtractImage.hpp
	RP_ExtractImage_p.hpp
	RpImageWin32.hpp
	RP_ShellPropSheetExt.hpp
	RP_ShellPropSheetExt_p.hpp
	thumbcache-wrapper.hpp
	RP_ThumbnailProvider.hpp
	RP_ThumbnailProvider_p.hpp
	RP_ShellIconOverlayIdentifier.hpp
	RP_ShellIconOverlayIdentifier_p.hpp
	RP_ContextMenu.hpp
	RP_ContextMenu_p.hpp
	RP_XAttrView.hpp
	RP_XAttrView_p.hpp
	RP_ColumnProvider.hpp
	CreateThumbnail.hpp
	DragImageLabel.hpp
	FontHandler.hpp
	MessageWidget.hpp
	LvData.hpp
	LanguageComboBox.hpp
	OptionsMenuButton.hpp
	RomDataFormat.hpp
	ISpriteSheet.hpp
	AchSpriteSheet.hpp
	FlagSpriteSheet.hpp
	config/ConfigDialog.hpp
	config/PropSheetIcon.hpp
	config/RP_EmptyVolumeCacheCallback.hpp
	config/ITab.hpp
	config/ImageTypesTab.hpp
	config/SystemsTab.hpp
	config/OptionsTab.hpp
	config/CacheTab.hpp
	config/AchievementsTab.hpp
	config/AboutTab.hpp
	file/RpFile_IStream.hpp
	file/RpFile_windres.hpp
	res/resource.h
	)
SET(${PROJECT_NAME}_RC
	res/resource.rc
	res/rp-de_DE.rc
	res/rp-en_US.rc
	res/rp-es_ES.rc
	res/rp-fr_FR.rc
	res/rp-it_IT.rc
	res/rp-pt_BR.rc
	res/rp-ro_RO.rc
	res/rp-ru_RU.rc
	)

IF(HAVE_RP_PROPERTYSTORE_DEPS)
	SET(${PROJECT_NAME}-PropKey_SRCS
		RP_PropertyStore.cpp
		RP_PropertyStore_Register.cpp
		RP_PropertyStore_GUIDs.c
		)
	SET(${PROJECT_NAME}-PropKey_H
		RP_PropertyStore.hpp
		RP_PropertyStore_p.hpp
		RP_PropertyStore_GUIDs.h
		)

	# Disable PCH for the IPropertyStore GUIDs file.
	SET_SOURCE_FILES_PROPERTIES(RP_PropertyStore_GUIDs.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
ENDIF(HAVE_RP_PROPERTYSTORE_DEPS)

IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_CRYPTO_SRCS
		config/KeyManagerTab.cpp
		config/KeyStoreWin32.cpp
		config/KeyStore_OwnerDataCallback.cpp
		)
	SET(${PROJECT_NAME}_CRYPTO_H
		config/KeyManagerTab.hpp
		config/KeyStoreWin32.hpp
		config/KeyStore_OwnerDataCallback.hpp
		)
	IF(MSVC)
		SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /dENABLE_DECRYPTION")
	ELSE(MSVC)
		SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -D ENABLE_DECRYPTION")
	ENDIF(MSVC)
ENDIF(ENABLE_DECRYPTION)

IF(ENABLE_ACHIEVEMENTS)
	SET(${PROJECT_NAME}_ACH_SRC AchWin32.cpp)
	SET(${PROJECT_NAME}_ACH_H AchWin32.hpp)
ENDIF(ENABLE_ACHIEVEMENTS)

IF(ENABLE_NETWORKING)
	# Networking code, e.g. update checker
	SET(${PROJECT_NAME}_NETWORKING_SRC
		NetworkStatus.c
		config/UpdateChecker.cpp
		)
	SET(${PROJECT_NAME}_NETWORKING_H
		NetworkStatus.h
		config/UpdateChecker.hpp
		)
ENDIF(ENABLE_NETWORKING)

IF(MSVC)
	# Delay-load sources.
	SET(${PROJECT_NAME}-DELAYLOAD_SRC ../libwin32common/DelayLoadHelper.c)
	SET(${PROJECT_NAME}-DELAYLOAD_H   ../libwin32common/DelayLoadHelper.h)
ENDIF(MSVC)

#####################
# Build the plugin. #
#####################

### rp-config stub.
# NOTE: This must be *before* including the DelayLoad flags.
ADD_SUBDIRECTORY(config)

# Delay-load extlibs.
INCLUDE(../libwin32common/DelayLoadHelper.cmake)
SET_DELAYLOAD_EXTLIB_FLAGS()

IF(RP_LIBROMDATA_IS_DLL AND MSVC)
	# Delay-load libromdata if it's a DLL.
	# Otherwise, we won't be able to load it if it's in an
	# architecture-specific subdirectory.
	# TODO: Global variable with the libromdata SOVERSION?
	SET(DL_DEBUG_FLAGS "${DL_DEBUG_FLAGS} /DELAYLOAD:romdata-8d.dll")
	SET(DL_RELEASE_FLAGS "${DL_RELEASE_FLAGS} /DELAYLOAD:romdata-8.dll")

	SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG	"${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${DL_DEBUG_FLAGS}")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE	"${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${DL_RELEASE_FLAGS}")
ENDIF(RP_LIBROMDATA_IS_DLL AND MSVC)

# NOTE: MODULE results in the .dll being placed in
# the 'lib' directory. Use SHARED instead.
# TODO: Disable building of import libraries.
ADD_LIBRARY(${PROJECT_NAME} SHARED
	${${PROJECT_NAME}-DELAYLOAD_SRC} ${${PROJECT_NAME}-DELAYLOAD_H}
	${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
	${${PROJECT_NAME}-PropKey_SRCS} ${${PROJECT_NAME}-PropKey_H}
	${${PROJECT_NAME}_CRYPTO_SRCS} ${${PROJECT_NAME}_CRYPTO_H}
	${${PROJECT_NAME}_ACH_SRC} ${${PROJECT_NAME}_ACH_H}
	${${PROJECT_NAME}_NETWORKING_SRC} ${${PROJECT_NAME}_NETWORKING_H}
	${${PROJECT_NAME}_RC}
	)
IF(ENABLE_PCH)
	TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} PRIVATE
		"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
		stdafx.h
		)
ENDIF(ENABLE_PCH)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(${PROJECT_NAME})
SET_WINDOWS_SUBSYSTEM(${PROJECT_NAME} WIN32)
SET_WINDOWS_NO_MANIFEST(${PROJECT_NAME})

IF(RP_LIBROMDATA_IS_DLL)
	# libromdata is being built as a DLL.
	# Need to enable dllimports.
	# FIXME: This *should* be inherited by linking to romdata...
	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE RP_BUILDING_FOR_DLL=1)
ENDIF(RP_LIBROMDATA_IS_DLL)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE win32ui)	# depends on SystemRegion::getLanguageCode()
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE win32darkmode)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE romdata)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE comctl32 advapi32 shell32 gdi32)
IF(HAVE_RP_PROPERTYSTORE_DEPS)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE propsys)
ENDIF(HAVE_RP_PROPERTYSTORE_DEPS)

IF(MSVC)
	# Delay-load uxtheme.dll.
	SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG	"${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DELAYLOAD:uxtheme.dll")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE	"${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DELAYLOAD:uxtheme.dll")
ENDIF(MSVC)

# zlib is needed for RpFile_IStream.
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ${ZLIB_DEFINITIONS})

# PugiXML headers are needed for AboutTab.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${pugixml_INCLUDE_DIR})

IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(${PROJECT_NAME} git_version)
ENDIF(TARGET git_version)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}\"")
ENDIF(MINGW)

# Use the .def file for DLL exports.
IF(MSVC)
	SET(RP_DLL_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.def")
	# Extended delay-load functionality.
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE delayimp)
ELSEIF(MINGW)
	# NOTE: Enabling stdcall fixups so we can use
	# the same .def file for Win32 and Win64,
	# since Win64 doesn't use stdcall.
	SET(RP_DLL_FLAGS "-Wl,--enable-stdcall-fixup ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.def")
ENDIF()

IF(RP_DLL_FLAGS)
	GET_TARGET_PROPERTY(RP_LINK_FLAGS ${PROJECT_NAME} LINK_FLAGS)
	IF(RP_LINK_FLAGS)
		SET(RP_LINK_FLAGS "${RP_LINK_FLAGS} ${RP_DLL_FLAGS}")
	ELSE()
		SET(RP_LINK_FLAGS "${RP_DLL_FLAGS}")
	ENDIF()
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${RP_LINK_FLAGS}")
	UNSET(RP_LINK_FLAGS)
	UNSET(RP_DLL_FLAGS)
ENDIF(RP_DLL_FLAGS)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "dll"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)

IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
