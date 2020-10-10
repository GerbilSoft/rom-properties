# Windows UI frontend
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(rom-properties-win32 LANGUAGES C CXX)

# Check for Windows SDK headers and symbols.
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckCXXSymbolExists)
# IThumbnailProvider
CHECK_INCLUDE_FILES("thumbcache.h" HAVE_THUMBCACHE_H)
# Property key symbols and functions for RP_PropertyStore
CHECK_SYMBOL_EXISTS(PKEY_Music_Conductor "windows.h;propvarutil.h;propkey.h" HAVE_PKEY_MUSIC_CONDUCTOR)
CHECK_CXX_SYMBOL_EXISTS(InitPropVariantFromUInt16 "windows.h;propvarutil.h" HAVE_INITPROPVARIANTFROMUINT16)
IF(HAVE_PKEY_MUSIC_CONDUCTOR AND HAVE_INITPROPVARIANTFROMUINT16)
	SET(HAVE_RP_PROPERTYSTORE_DEPS 1)
ELSE()
	MESSAGE(WARNING "Windows SDK is missing required IPropertyStore symbols.\nIPropertyStore functionality will not be available.")
ENDIF()

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.win32.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.win32.h")

# Create the manifest file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/res/rom-properties.dll.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/res/rom-properties.dll.manifest" @ONLY)

# Sources and headers.
SET(rom-properties-win32_SRCS
	DllMain.cpp
	com-iids.c
	RP_ExtractIcon.cpp
	RP_ExtractIcon_Register.cpp
	RP_ExtractIcon_Fallback.cpp
	RP_ExtractImage.cpp
	RP_ExtractImage_Register.cpp
	RP_ExtractImage_Fallback.cpp
	RpImageWin32.cpp
	RP_ShellPropSheetExt.cpp
	RP_ShellPropSheetExt_Register.cpp
	RpFile_IStream.cpp
	RP_ThumbnailProvider.cpp
	RP_ThumbnailProvider_Register.cpp
	RP_ThumbnailProvider_Fallback.cpp
	RP_ShellIconOverlayIdentifier.cpp
	RP_ShellIconOverlayIdentifier_Register.cpp
	CreateThumbnail.cpp
	DragImageLabel.cpp
	FontHandler.cpp
	MessageWidget.cpp
	LvData.cpp
	config/ConfigDialog.cpp
	config/PropSheetIcon.cpp
	config/RP_EmptyVolumeCacheCallback.cpp
	config/ITab.cpp
	config/ImageTypesTab.cpp
	config/SystemsTab.cpp
	config/OptionsTab.cpp
	config/CacheTab.cpp
	config/AchievementsTab.cpp
	config/AboutTab.cpp
	)
SET(rom-properties-win32_H
	RP_ExtractIcon.hpp
	RP_ExtractIcon_p.hpp
	RP_ClassFactory.hpp
	RP_ExtractImage.hpp
	RP_ExtractImage_p.hpp
	RpImageWin32.hpp
	RP_ShellPropSheetExt.hpp
	RpFile_IStream.hpp
	thumbcache-wrapper.hpp
	RP_ThumbnailProvider.hpp
	RP_ThumbnailProvider_p.hpp
	RP_ShellIconOverlayIdentifier.hpp
	RP_ShellIconOverlayIdentifier_p.hpp
	CreateThumbnail.hpp
	DragImageLabel.hpp
	FontHandler.hpp
	MessageWidget.hpp
	LvData.hpp
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
	res/resource.h
	)
SET(rom-properties-win32_RC
	res/resource.rc
	res/rp-en_US.rc
	res/rp-pt_BR.rc
	)

IF(HAVE_RP_PROPERTYSTORE_DEPS)
	SET(rom-properties-win32-PropKey_SRCS
		RP_PropertyStore.cpp
		RP_PropertyStore_Register.cpp
		)
	SET(rom-properties-win32-PropKey_H
		RP_PropertyStore.hpp
		RP_PropertyStore_p.hpp
		)
ENDIF(HAVE_RP_PROPERTYSTORE_DEPS)

IF(ENABLE_DECRYPTION)
	SET(rom-properties-win32_CRYPTO_SRCS
		config/KeyManagerTab.cpp
		config/KeyStoreWin32.cpp
		config/KeyStore_OwnerDataCallback.cpp
		)
	SET(rom-properties-win32_CRYPTO_H
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
	SET(rom-properties-win32_ACH_SRC AchWin32.cpp)
	SET(rom-properties-win32_ACH_H AchWin32.hpp)
ENDIF(ENABLE_ACHIEVEMENTS)

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(rom-properties-win32_PCH_C stdafx.c)
	SET(rom-properties-win32_PCH_CXX stdafx.cpp)
	SET(rom-properties-win32_PCH_SRC ${rom-properties-win32_PCH_C} ${rom-properties-win32_PCH_CXX})
	SET(rom-properties-win32_PCH_H stdafx.h)
ENDIF(ENABLE_PCH)

IF(MSVC)
	# Delay-load sources.
	SET(rom-properties-win32-DELAYLOAD_SRC ../libwin32common/DelayLoadHelper.c)
	SET(rom-properties-win32-DELAYLOAD_H ../libwin32common/DelayLoadHelper.h)
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

# NOTE: MODULE results in the .dll being placed in
# the 'lib' directory. Use SHARED instead.
# TODO: Disable building of import libraries.
ADD_LIBRARY(rom-properties SHARED
	${rom-properties-win32_PCH_SRC} ${rom-properties-win32_PCH_H}
	${rom-properties-win32-DELAYLOAD_SRC} ${rom-properties-win32-DELAYLOAD_H}
	${rom-properties-win32_SRCS} ${rom-properties-win32_H}
	${rom-properties-win32-PropKey_SRCS} ${rom-properties-win32-PropKey_H}
	${rom-properties-win32_CRYPTO_SRCS} ${rom-properties-win32_CRYPTO_H}
	${rom-properties-win32_ACH_SRC} ${rom-properties-win32_ACH_H}
	${rom-properties-win32_RC}
	)
IF(ENABLE_PCH)
	ADD_PRECOMPILED_HEADER(rom-properties ${rom-properties-win32_PCH_H}
		SOURCE_C ${rom-properties-win32_PCH_C}
		SOURCE_CXX ${rom-properties-win32_PCH_CXX}
		)
ENDIF(ENABLE_PCH)
SET_TARGET_PROPERTIES(rom-properties PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rom-properties)
SET_WINDOWS_SUBSYSTEM(rom-properties WIN32)
SET_WINDOWS_NO_MANIFEST(rom-properties)
# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rom-properties
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(rom-properties PRIVATE romdata rpfile rpcpu rpbase rpthreads win32common)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties PRIVATE i18n)
ENDIF(ENABLE_NLS)
TARGET_LINK_LIBRARIES(rom-properties PRIVATE comctl32 advapi32 shell32 gdi32)
IF(HAVE_RP_PROPERTYSTORE_DEPS)
	TARGET_LINK_LIBRARIES(rom-properties PRIVATE propsys)
ENDIF(HAVE_RP_PROPERTYSTORE_DEPS)

# Add zlib, png, and tinyxml2 directories for the About tab.
IF(HAVE_ZLIB)
	TARGET_INCLUDE_DIRECTORIES(rom-properties PRIVATE ${ZLIB_INCLUDE_DIRS})
ENDIF(HAVE_ZLIB)
IF(HAVE_PNG)
	TARGET_INCLUDE_DIRECTORIES(rom-properties PRIVATE ${PNG_INCLUDE_DIRS})
ENDIF(HAVE_PNG)
IF(HAVE_XML)
	TARGET_INCLUDE_DIRECTORIES(rom-properties PRIVATE ${TinyXML2_INCLUDE_DIR})
ENDIF(HAVE_XML)

# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(rpbase git_version)
ENDIF(TARGET git_version)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}\"")
ENDIF(MINGW)

# Use the .def file for DLL exports.
IF(MSVC)
	SET(RP_DLL_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def")
	# Extended delay-load functionality.
	TARGET_LINK_LIBRARIES(rom-properties PRIVATE delayimp)
ELSEIF(MINGW)
	# NOTE: Enabling stdcall fixups so we can use
	# the same .def file for Win32 and Win64,
	# since Win64 doesn't use stdcall.
	SET(RP_DLL_FLAGS "-Wl,-enable-stdcall-fixup ${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def")
ENDIF()

IF(RP_DLL_FLAGS)
	GET_TARGET_PROPERTY(RP_LINK_FLAGS rom-properties LINK_FLAGS)
	IF(RP_LINK_FLAGS)
		SET(RP_LINK_FLAGS "${RP_LINK_FLAGS} ${RP_DLL_FLAGS}")
	ELSE()
		SET(RP_LINK_FLAGS "${RP_DLL_FLAGS}")
	ENDIF()
	SET_TARGET_PROPERTIES(rom-properties PROPERTIES LINK_FLAGS "${RP_LINK_FLAGS}")
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
INSTALL(TARGETS rom-properties
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "dll"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rom-properties PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
