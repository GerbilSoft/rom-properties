PROJECT(rom-properties-win32)

# Create the manifest file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.dll.manifest.in" "${CMAKE_CURRENT_BINARY_DIR}/rom-properties.dll.manifest" @ONLY)

# Sources and headers.
SET(rom-properties-win32_SRCS
	DllMain.cpp
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
	CreateThumbnail.cpp
	config/ConfigDialog.cpp
	config/PropSheetIcon.cpp
	config/RP_EmptyVolumeCacheCallback.cpp
	config/ImageTypesTab.cpp
	config/DownloadsTab.cpp
	config/CacheTab.cpp
	config/KeyStore_OwnerDataCallback.cpp
	config/AboutTab.cpp
	resource.rc
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
	RP_ThumbnailProvider.hpp
	RP_ThumbnailProvider_p.hpp
	CreateThumbnail.hpp
	config/ConfigDialog.hpp
	config/PropSheetIcon.hpp
	config/RP_EmptyVolumeCacheCallback.hpp
	config/ITab.hpp
	config/ImageTypesTab.hpp
	config/DownloadsTab.hpp
	config/CacheTab.hpp
	config/KeyStore_OwnerDataCallback.hpp
	config/AboutTab.hpp
	resource.h
	)

IF(ENABLE_DECRYPTION)
	SET(rom-properties-win32_CRYPTO_SRCS
		config/KeyManagerTab.cpp
		config/KeyStoreWin32.cpp
		)
	SET(rom-properties-win32_CRYPTO_H
		config/KeyManagerTab.hpp
		config/KeyStoreWin32.hpp
		)
ENDIF(ENABLE_DECRYPTION)

IF(MSVC)
	# Precompiled headers.
	# NOTE: Only enabling PCH for MSVC builds.
	INCLUDE(PrecompiledHeader)
	SET(rom-properties-win32_PCH_SRC stdafx.cpp)
	SET(rom-properties-win32_PCH_H stdafx.h)

	# Delay-load sources.
	SET(rom-properties-win32-DELAY_LOAD_SRC ../libwin32common/DelayLoadHelper.cpp)
	SET(rom-properties-win32-DELAY_LOAD_H ../libwin32common/DelayLoadHelper.hpp)
ENDIF(MSVC)

#####################
# Build the plugin. #
#####################

### rp-config stub.
# NOTE: This must be *before* including the DelayLoad flags.
ADD_SUBDIRECTORY(config)

IF(MSVC)
	# Delay-load extlibs.
	INCLUDE(../libwin32common/DelayLoadHelper.cmake)
	SET_DELAY_LOAD_FLAGS()
ENDIF(MSVC)

# FIXME: Build both Win32 and Win64 DLLs?
# NOTE: MODULE results in the .dll being placed in
# the 'lib' directory. Use SHARED instead.
# TODO: Disable building of import libraries.
ADD_LIBRARY(rom-properties SHARED
	${rom-properties-win32_PCH_SRC} ${rom-properties-win32_PCH_H}
	${rom-properties-win32-DELAY_LOAD_SRC} ${rom-properties-win32-DELAY_LOAD_H}
	${rom-properties-win32_SRCS} ${rom-properties-win32_H}
	${rom-properties-win32_CRYPTO_SRCS} ${rom-properties-win32_CRYPTO_H}
	)
# NOTE: Only enabling PCH for MSVC.
IF(MSVC)
	ADD_PRECOMPILED_HEADER(rom-properties ${rom-properties-win32_PCH_H}
		SOURCE_CXX ${rom-properties-win32_PCH_SRC}
		)
ENDIF(MSVC)
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
TARGET_LINK_LIBRARIES(rom-properties cachemgr romdata rpbase win32common)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(rom-properties i18n)
ENDIF(ENABLE_NLS)
TARGET_LINK_LIBRARIES(rom-properties comctl32 advapi32 wtsapi32)

# Add zlib and png directories for the About tab.
IF(HAVE_ZLIB)
	TARGET_INCLUDE_DIRECTORIES(rom-properties PRIVATE ${ZLIB_INCLUDE_DIRS})
ENDIF(HAVE_ZLIB)
IF(HAVE_PNG)
	TARGET_INCLUDE_DIRECTORIES(rom-properties PRIVATE ${PNG_INCLUDE_DIRS})
ENDIF(HAVE_PNG)

# Delay-load some Windows system libraries.
# TODO: MinGW delay-load?
IF(MSVC)
	SET(RP_SYS_DELAY_LOAD_FLAGS "/DELAYLOAD:urlmon.dll /DELAYLOAD:wininet.dll")
	TARGET_LINK_LIBRARIES(rom-properties delayimp)
ENDIF(MSVC)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_BINARY_DIR}\"")
ENDIF(MINGW)

# Use the .def file for DLL exports.
# Also add delay-load flags here. (TODO: MinGW?)
IF(MSVC)
	SET(RP_DLL_FLAGS "${RP_SYS_DELAY_LOAD_FLAGS} /DEF:${CMAKE_CURRENT_SOURCE_DIR}/rom-properties.def")
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
