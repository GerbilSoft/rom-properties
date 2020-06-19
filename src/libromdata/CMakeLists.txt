# RomData subclasses and factory class.
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(libromdata LANGUAGES CXX)

IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

IF(NOT WIN32)
	CHECK_SYMBOL_EXISTS(posix_spawn "spawn.h" HAVE_POSIX_SPAWN)
ENDIF(NOT WIN32)

# Sources.
SET(libromdata_SRCS
	RomDataFactory.cpp

	Console/Dreamcast.cpp
	Console/DreamcastSave.cpp
	Console/GameCube.cpp
	Console/GameCubeBNR.cpp
	Console/GameCubeSave.cpp
	Console/GameCubeRegions.cpp
	Console/iQuePlayer.cpp
	Console/MegaDrive.cpp
	Console/MegaDriveRegions.cpp
	Console/N64.cpp
	Console/NES.cpp
	Console/Sega8Bit.cpp
	Console/SegaSaturn.cpp
	Console/SNES.cpp
	Console/PlayStationSave.cpp
	Console/WiiCommon.cpp
	Console/WiiSave.cpp
	Console/WiiU.cpp
	Console/WiiWAD.cpp
	Console/WiiWIBN.cpp
	Console/Xbox_XBE.cpp
	Console/Xbox360_STFS.cpp
	Console/Xbox360_XDBF.cpp
	Console/Xbox360_XEX.cpp
	Console/XboxDisc.cpp

	Handheld/DMG.cpp
	Handheld/GameBoyAdvance.cpp
	Handheld/GameCom.cpp
	Handheld/Lynx.cpp
	Handheld/NGPC.cpp
	Handheld/Nintendo3DS.cpp
	Handheld/Nintendo3DSFirm.cpp
	Handheld/Nintendo3DS_SMDH.cpp
	Handheld/NintendoDS.cpp
	Handheld/PokemonMini.cpp
	Handheld/VirtualBoy.cpp

	Audio/ADX.cpp
	Audio/BCSTM.cpp
	Audio/BRSTM.cpp
	Audio/GBS.cpp
	Audio/NSF.cpp
	Audio/PSF.cpp
	Audio/SAP.cpp
	Audio/SNDH.cpp
	Audio/SID.cpp
	Audio/SPC.cpp
	Audio/VGM.cpp

	Other/Amiibo.cpp
	Other/ELF.cpp
	Other/EXE.cpp
	Other/EXE_NE.cpp
	Other/EXE_PE.cpp
	Other/ISO.cpp
	Other/MachO.cpp
	Other/NintendoBadge.cpp
	Other/RpTextureWrapper.cpp

	data/AmiiboData.cpp
	data/ELFData.cpp
	data/EXEData.cpp
	data/MachOData.cpp
	data/NESMappers.cpp
	data/NintendoPublishers.cpp
	data/Nintendo3DSFirmData.cpp
	data/Nintendo3DSSysTitles.cpp
	data/NintendoLanguage.cpp
	data/SegaPublishers.cpp
	data/WiiSystemMenuVersion.cpp
	data/WiiUData.cpp
	data/XboxLanguage.cpp
	data/Xbox360_STFS_ContentType.cpp

	disc/Cdrom2352Reader.cpp
	disc/CIAReader.cpp
	disc/CisoGcnReader.cpp
	disc/GcnFst.cpp
	disc/GcnPartition.cpp
	disc/GcnPartitionPrivate.cpp
	disc/GdiReader.cpp
	disc/IResourceReader.cpp
	disc/IsoPartition.cpp
	disc/NASOSReader.cpp
	disc/NCCHReader.cpp
	disc/NEResourceReader.cpp
	disc/PEResourceReader.cpp
	disc/WbfsReader.cpp
	disc/WiiPartition.cpp
	disc/WuxReader.cpp
	disc/XDVDFSPartition.cpp

	#config/TImageTypesConfig.cpp	# NOT listed here due to template stuff.
	#img/TCreateThumbnail.cpp	# NOT listed here due to template stuff.
	img/CacheManager.cpp
	utils/SuperMagicDrive.cpp
	)
# Headers.
SET(libromdata_H
	RomDataFactory.hpp
	CopierFormats.h
	cdrom_structs.h
	iso_structs.h

	Console/Dreamcast.hpp
	Console/DreamcastSave.hpp
	Console/GameCube.hpp
	Console/GameCubeBNR.hpp
	Console/GameCubeSave.hpp
	Console/GameCubeRegions.hpp
	Console/iQuePlayer.hpp
	Console/MegaDrive.hpp
	Console/MegaDriveRegions.hpp
	Console/N64.hpp
	Console/NES.hpp
	Console/PlayStationSave.hpp
	Console/Sega8Bit.hpp
	Console/SegaSaturn.hpp
	Console/SNES.hpp
	Console/WiiCommon.hpp
	Console/WiiSave.hpp
	Console/WiiU.hpp
	Console/WiiWAD.hpp
	Console/WiiWIBN.hpp
	Console/Xbox_XBE.hpp
	Console/Xbox360_STFS.hpp
	Console/Xbox360_XDBF.hpp
	Console/Xbox360_XEX.hpp
	Console/XboxDisc.hpp

	Console/dc_structs.h
	Console/gcn_structs.h
	Console/gcn_banner.h
	Console/gcn_card.h
	Console/ique_player_structs.h
	Console/md_structs.h
	Console/n64_structs.h
	Console/nes_structs.h
	Console/ps1_structs.h
	Console/saturn_structs.h
	Console/sega8_structs.h
	Console/snes_structs.h
	Console/wii_structs.h
	Console/wii_banner.h
	Console/wii_wad.h
	Console/wiiu_structs.h
	Console/xbox_xbe_structs.h
	Console/xbox360_common_structs.h
	Console/xbox360_stfs_structs.h
	Console/xbox360_xdbf_structs.h
	Console/xbox360_xex_structs.h

	Handheld/DMG.hpp
	Handheld/GameBoyAdvance.hpp
	Handheld/GameCom.hpp
	Handheld/Lynx.hpp
	Handheld/NGPC.hpp
	Handheld/Nintendo3DS.hpp
	Handheld/Nintendo3DSFirm.hpp
	Handheld/Nintendo3DS_SMDH.hpp
	Handheld/NintendoDS.hpp
	Handheld/PokemonMini.hpp
	Handheld/VirtualBoy.hpp

	Handheld/dmg_structs.h
	Handheld/gba_structs.h
	Handheld/gcom_structs.h
	Handheld/n3ds_structs.h
	Handheld/nds_structs.h
	Handheld/ngpc_structs.h
	Handheld/pkmnmini_structs.h
	Handheld/lnx_structs.h
	Handheld/vb_structs.h

	Audio/ADX.hpp
	Audio/BCSTM.hpp
	Audio/BRSTM.hpp
	Audio/GBS.hpp
	Audio/NSF.hpp
	Audio/PSF.hpp
	Audio/SAP.hpp
	Audio/SNDH.hpp
	Audio/SID.hpp
	Audio/SPC.hpp
	Audio/VGM.hpp

	Audio/adx_structs.h
	Audio/bcstm_structs.h
	Audio/brstm_structs.h
	Audio/gbs_structs.h
	Audio/nsf_structs.h
	Audio/psf_structs.h
	Audio/sid_structs.h
	Audio/spc_structs.h
	Audio/vgm_structs.h

	Other/Amiibo.hpp
	Other/ELF.hpp
	Other/EXE.hpp
	Other/EXE_p.hpp
	Other/ISO.hpp
	Other/MachO.hpp
	Other/NintendoBadge.hpp
	Other/RpTextureWrapper.hpp

	Other/badge_structs.h
	Other/elf_structs.h
	Other/exe_structs.h
	Other/macho_structs.h
	Other/nfp_structs.h

	data/AmiiboData.hpp
	data/ELFData.hpp
	data/EXEData.hpp
	data/MachOData.hpp
	data/NESMappers.hpp
	data/Nintendo3DSFirmData.hpp
	data/Nintendo3DSSysTitles.hpp
	data/NintendoLanguage.hpp
	data/NintendoPublishers.hpp
	data/SegaPublishers.hpp
	data/WiiSystemMenuVersion.hpp
	data/WiiUData.hpp
	data/XboxLanguage.hpp
	data/Xbox360_STFS_ContentType.hpp

	disc/Cdrom2352Reader.hpp
	disc/CIAReader.hpp
	disc/ciso_gcn.h
	disc/CisoGcnReader.hpp
	disc/GcnFst.hpp
	disc/GcnPartition.hpp
	disc/GcnPartitionPrivate.hpp
	disc/GdiReader.hpp
	disc/IResourceReader.hpp
	disc/IsoPartition.hpp
	disc/libwbfs.h
	disc/nasos_gcn.h
	disc/NASOSReader.hpp
	disc/NCCHReader.hpp
	disc/NCCHReader_p.hpp
	disc/NEResourceReader.hpp
	disc/PEResourceReader.hpp
	disc/WbfsReader.hpp
	disc/WiiPartition.hpp
	disc/WuxReader.hpp
	disc/wux_structs.h
	disc/XDVDFSPartition.cpp
	disc/xdvdfs_structs.h

	config/TImageTypesConfig.hpp
	img/TCreateThumbnail.hpp
	img/CacheManager.hpp
	utils/SuperMagicDrive.hpp
	)

IF(ENABLE_XML)
	SET(libromdata_SRCS ${libromdata_SRCS} Other/EXE_manifest.cpp)
	IF(MSVC AND (NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL))
		SET(libromdata_SRCS ${libromdata_SRCS} Other/EXE_delayload.cpp)
	ENDIF(MSVC AND (NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL))
ENDIF(ENABLE_XML)

IF(ENABLE_DECRYPTION)
	SET(libromdata_CRYPTO_SRCS
		crypto/CtrKeyScrambler.cpp
		crypto/N3DSVerifyKeys.cpp
		crypto/KeyStoreUI.cpp
		)
	SET(libromdata_CRYPTO_H
		crypto/CtrKeyScrambler.hpp
		crypto/N3DSVerifyKeys.hpp
		crypto/KeyStoreUI.hpp
		)
ENDIF(ENABLE_DECRYPTION)

# OS-specific sources.
IF(WIN32)
	SET(libromdata_OS_SRCS img/ExecRpDownload_win32.cpp)
ELSEIF(UNIX)
	SET(libromdata_OS_SRCS img/ExecRpDownload_posix.cpp)
ELSE()
	# Dummy implementation for unsupported systems.
	SET(libromdata_OS_SRCS img/ExecRpDownload_dummy.cpp)
ENDIF()

# Optimized sources.
IF(CPU_i386 OR CPU_amd64)
	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(libromdata_IFUNC_SRCS utils/SuperMagicDrive_ifunc.cpp)
		# Disable LTO on the IFUNC files if LTO is known to be broken.
		IF(GCC_5xx_LTO_ISSUES)
			SET_SOURCE_FILES_PROPERTIES(${libromdata_IFUNC_SRCS}
				APPEND_STRING PROPERTIES COMPILE_FLAGS " -fno-lto ")
		ENDIF(GCC_5xx_LTO_ISSUES)
	ENDIF(UNIX AND NOT APPLE)

	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx
	#
	# amd64 CPUs support SSE2 as a minimum anyway, so there's
	# no point in building MMX code for 64-bit.
	IF(CPU_i386)
		SET(libromdata_MMX_SRCS utils/SuperMagicDrive_mmx.cpp)
	ENDIF(CPU_i386)
	SET(libromdata_SSE2_SRCS
		${libromdata_SSE2_SRCS}
		utils/SuperMagicDrive_sse2.cpp
		)

	IF(MSVC AND NOT CMAKE_CL_64)
		SET(SSE2_FLAG "/arch:SSE2")
	ELSEIF(NOT MSVC)
		# TODO: Other compilers?
		# TODO: Not needed on 64-bit.
		SET(MMX_FLAG "-mmmx")
		SET(SSE2_FLAG "-msse2")
	ENDIF()

	IF(MMX_FLAG)
		SET_SOURCE_FILES_PROPERTIES(utils/SuperMagicDrive_mmx.cpp
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
	ENDIF(MMX_FLAG)
	IF(SSE2_FLAG)
		SET_SOURCE_FILES_PROPERTIES(utils/SuperMagicDrive_sse2.cpp
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
	ENDIF(SSE2_FLAG)
ENDIF()

# Write the config.h file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libromdata.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libromdata.h")

IF(ENABLE_PCH)
	# Precompiled headers.
	INCLUDE(PrecompiledHeader)
	SET(libromdata_PCH_CXX stdafx.cpp)
	SET(libromdata_PCH_SRC ${libromdata_PCH_CXX})
	SET(libromdata_PCH_H stdafx.h)
ENDIF(ENABLE_PCH)

######################
# Build the library. #
######################

ADD_LIBRARY(romdata STATIC
	${libromdata_PCH_SRC} ${libromdata_PCH_H}
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	${libromdata_IFUNC_SRCS}
	${libromdata_MMX_SRCS}
	${libromdata_SSE2_SRCS}
	)
IF(ENABLE_PCH)
	ADD_PRECOMPILED_HEADER(romdata ${libromdata_PCH_H}
		SOURCE_CXX ${libromdata_PCH_CXX}
		)
ENDIF(ENABLE_PCH)
INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(romdata)

# TODO: More public/private library stuff.
TARGET_LINK_LIBRARIES(romdata PUBLIC rptexture rpfile rpbase)
TARGET_LINK_LIBRARIES(romdata PRIVATE rpcpu rpthreads cachecommon)
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(romdata PRIVATE i18n)
ENDIF(ENABLE_NLS)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(romdata
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libromdata
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libromdata
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# top-level
	)
IF(WIN32)
	# librpsecure: integrity_level.h
	TARGET_LINK_LIBRARIES(romdata PRIVATE rpsecure)

	# Windows SDK libraries.
	TARGET_LINK_LIBRARIES(romdata PRIVATE advapi32)
	IF(MSVC)
		TARGET_LINK_LIBRARIES(romdata PRIVATE delayimp)
	ENDIF(MSVC)
ENDIF(WIN32)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(romdata PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Other libraries.
TARGET_LINK_LIBRARIES(romdata PRIVATE ${ZLIB_LIBRARY})
IF(ENABLE_XML AND TinyXML2_FOUND)
	TARGET_LINK_LIBRARIES(romdata PRIVATE ${TinyXML2_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(romdata PRIVATE ${TinyXML2_INCLUDE_DIR})
ENDIF(ENABLE_XML AND TinyXML2_FOUND)

IF(ENABLE_UNICE68)
	TARGET_LINK_LIBRARIES(romdata PRIVATE unice68_lib)
ENDIF(ENABLE_UNICE68)

IF(ENABLE_LIBMSPACK)
	TARGET_LINK_LIBRARIES(romdata PRIVATE mspack)
ENDIF(ENABLE_LIBMSPACK)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
