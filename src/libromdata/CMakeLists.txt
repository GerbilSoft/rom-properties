# RomData subclasses and factory class.
PROJECT(romdata LANGUAGES CXX)

IF(ENABLE_NETWORKING AND NOT WIN32)
	INCLUDE(CheckSymbolExists)
	CHECK_SYMBOL_EXISTS(posix_spawn "spawn.h" HAVE_POSIX_SPAWN)
ENDIF(ENABLE_NETWORKING AND NOT WIN32)

# Sources.
SET(${PROJECT_NAME}_SRCS
	RomDataFactory.cpp

	Console/Atari7800.cpp
	Console/CBMCart.cpp
	Console/ColecoVision.cpp
	Console/Dreamcast.cpp
	Console/DreamcastSave.cpp
	Console/GameCube.cpp
	Console/GameCubeBNR.cpp
	Console/GameCubeSave.cpp
	Console/GameCubeRegions.cpp
	Console/Intellivision.cpp
	Console/iQuePlayer.cpp
	Console/MegaDrive.cpp
	Console/MegaDriveRegions.cpp
	Console/N64.cpp
	Console/NES.cpp
	Console/PlayStationDisc.cpp
	Console/PlayStationEXE.cpp
	Console/PlayStationSave.cpp
	Console/Sega8Bit.cpp
	Console/SegaSaturn.cpp
	Console/SNES.cpp
	Console/SufamiTurbo.cpp
	Console/WiiBNR.cpp
	Console/WiiCommon.cpp
	Console/WiiSave.cpp
	Console/WiiTicket.cpp
	Console/WiiTMD.cpp
	Console/WiiU.cpp
	Console/WiiUPackage.cpp
	Console/WiiWAD.cpp
	Console/WiiWAD_ops.cpp
	Console/WiiWIBN.cpp
	Console/Xbox_XBE.cpp
	Console/Xbox360_STFS.cpp
	Console/Xbox360_XDBF.cpp
	Console/Xbox360_XEX.cpp
	Console/XboxDisc.cpp

	Handheld/DMG.cpp
	Handheld/GameBoyAdvance.cpp
	Handheld/GameCom.cpp
	Handheld/J2ME.cpp
	Handheld/Lynx.cpp
	Handheld/NGPC.cpp
	Handheld/Nintendo3DS.cpp
	Handheld/Nintendo3DS_ops.cpp
	Handheld/Nintendo3DSFirm.cpp
	Handheld/Nintendo3DS_SMDH.cpp
	Handheld/NintendoDS.cpp
	Handheld/NintendoDS_ops.cpp
	Handheld/NintendoDS_BNR.cpp
	Handheld/PalmOS.cpp
	Handheld/PokemonMini.cpp
	Handheld/PSP.cpp
	Handheld/VirtualBoy.cpp
	Handheld/WonderSwan.cpp
	Handheld/nds_crc.cpp

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

	Media/CBMDOS.cpp
	Media/ISO.cpp
	Media/Wim.cpp

	Other/Amiibo.cpp
	Other/ELF.cpp
	Other/EXE.cpp
	Other/EXE_NE.cpp
	Other/EXE_PE.cpp
	Other/MachO.cpp
	Other/NintendoBadge.cpp
	Other/RpTextureWrapper.cpp
	Other/Lua.cpp

	data/AmiiboData.cpp
	data/CBMData.cpp
	data/DMGSpecialCases.cpp
	data/ELFData.cpp
	data/EXEData.cpp
	data/EXENEEntries.cpp
	data/MachOData.cpp
	data/NESMappers.cpp
	data/NintendoPublishers.cpp
	data/Nintendo3DSFirmData.cpp
	data/Nintendo3DSSysTitles.cpp
	data/NintendoLanguage.cpp
	data/SegaPublishers.cpp
	data/WiiSystemMenuVersion.cpp
	data/WiiUData.cpp
	data/WonderSwanPublishers.cpp
	data/XboxLanguage.cpp
	data/XboxPublishers.cpp
	data/Xbox360_STFS_ContentType.cpp

	disc/Cdrom2352Reader.cpp
	disc/CdiReader.cpp
	disc/CIAReader.cpp
	disc/CisoGcnReader.cpp
	disc/CisoPspReader.cpp
	disc/DpfReader.cpp
	disc/GcnFst.cpp
	disc/GcnPartition.cpp
	disc/GcnPartition_p.cpp
	disc/GczReader.cpp
	disc/GdiReader.cpp
	disc/IResourceReader.cpp
	disc/IsoPartition.cpp
	disc/NASOSReader.cpp
	disc/NCCHReader.cpp
	disc/NEResourceReader.cpp
	disc/PEResourceReader.cpp
	disc/WbfsReader.cpp
	disc/WiiPartition.cpp
	disc/WiiUFst.cpp
	disc/WiiUH3Reader.cpp
	disc/WuxReader.cpp
	disc/XDVDFSPartition.cpp

	config/ImageTypesConfig.cpp
	#config/TImageTypesConfig.cpp	# NOT listed here due to template stuff.
	#img/TCreateThumbnail.cpp	# NOT listed here due to template stuff.
	img/CacheManager.cpp
	utils/SuperMagicDrive.cpp
	)
# Headers.
SET(${PROJECT_NAME}_H
	RomDataFactory.hpp
	CopierFormats.h
	cdrom_structs.h
	iso_structs.h
	nintendo_system_id.h

	Console/Atari7800.hpp
	Console/CBMCart.hpp
	Console/ColecoVision.hpp
	Console/Dreamcast.hpp
	Console/DreamcastSave.hpp
	Console/GameCube.hpp
	Console/GameCubeBNR.hpp
	Console/GameCubeSave.hpp
	Console/GameCubeRegions.hpp
	Console/Intellivision.hpp
	Console/iQuePlayer.hpp
	Console/MegaDrive.hpp
	Console/MegaDriveRegions.hpp
	Console/N64.hpp
	Console/NES.hpp
	Console/PlayStationDisc.hpp
	Console/PlayStationEXE.cpp
	Console/PlayStationSave.hpp
	Console/Sega8Bit.hpp
	Console/SegaSaturn.hpp
	Console/SNES.hpp
	Console/SufamiTurbo.hpp
	Console/WiiBNR.hpp
	Console/WiiCommon.hpp
	Console/WiiTicket.hpp
	Console/WiiTMD.hpp
	Console/WiiSave.hpp
	Console/WiiU.hpp
	Console/WiiUPackage.hpp
	Console/WiiUPackage_p.hpp
	Console/WiiWAD.hpp
	Console/WiiWAD_p.hpp
	Console/WiiWIBN.hpp
	Console/Xbox_XBE.hpp
	Console/Xbox360_STFS.hpp
	Console/Xbox360_XDBF.hpp
	Console/Xbox360_XEX.hpp
	Console/XboxDisc.hpp

	Console/atari_7800_structs.h
	Console/cv_structs.h
	Console/cbm_cart_structs.h
	Console/dc_structs.h
	Console/gcn_structs.h
	Console/gcn_banner.h
	Console/gcn_card.h
	Console/intv_structs.h
	Console/ique_player_structs.h
	Console/md_structs.h
	Console/n64_structs.h
	Console/nes_structs.h
	Console/ps1_structs.h
	Console/ps1_exe_structs.h
	Console/ps2_structs.h
	Console/saturn_structs.h
	Console/sega8_structs.h
	Console/snes_structs.h
	Console/st_structs.h
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
	Handheld/J2ME.hpp
	Handheld/Lynx.hpp
	Handheld/NGPC.hpp
	Handheld/Nintendo3DS.hpp
	Handheld/Nintendo3DS_p.hpp
	Handheld/Nintendo3DSFirm.hpp
	Handheld/Nintendo3DS_SMDH.hpp
	Handheld/NintendoDS.hpp
	Handheld/NintendoDS_p.hpp
	Handheld/NintendoDS_BNR.hpp
	Handheld/PalmOS.hpp
	Handheld/PokemonMini.hpp
	Handheld/PSP.hpp
	Handheld/VirtualBoy.hpp
	Handheld/WonderSwan.hpp
	Handheld/nds_crc.hpp

	Handheld/dmg_structs.h
	Handheld/gba_structs.h
	Handheld/gcom_structs.h
	Handheld/n3ds_structs.h
	Handheld/nds_structs.h
	Handheld/ngpc_structs.h
	Handheld/palmos_structs.h
	Handheld/pkmnmini_structs.h
	Handheld/lnx_structs.h
	Handheld/vb_structs.h
	Handheld/ws_structs.h

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

	Media/CBMDOS.hpp
	Media/ISO.hpp
	Media/Wim.hpp

	Media/cbmdos_structs.h
	Media/hsfs_structs.h
	Media/wim_structs.h

	Other/Amiibo.hpp
	Other/ELF.hpp
	Other/EXE.hpp
	Other/EXE_p.hpp
	Other/MachO.hpp
	Other/NintendoBadge.hpp
	Other/RpTextureWrapper.hpp
	Other/Lua.hpp

	Other/badge_structs.h
	Other/elf_structs.h
	Other/exe_le_structs.h
	Other/exe_mz_structs.h
	Other/exe_ne_structs.h
	Other/exe_pe_structs.h
	Other/exe_res_structs.h
	Other/macho_structs.h
	Other/nfp_structs.h

	data/AmiiboData.hpp
	data/CBMData.hpp
	data/CBM_C64_Cartridge_Type_data.h
	data/CBM_Plus4_Cartridge_Type_data.h
	data/CBM_VIC20_Cartridge_Type_data.h
	data/DMGSpecialCases.hpp
	data/ELFData.hpp
	data/ELFMachineTypes_data.h
	data/ELF_OSABI_data.h
	data/EXEData.hpp
	data/EXELEMachineTypes_data.h
	data/EXENEEntries.hpp
	data/EXENEEntries_data.h
	data/EXEPEMachineTypes_data.h
	data/MachOData.hpp
	data/NESMappers.hpp
	data/NESMappers_data.h
	data/Nintendo3DSFirmData.hpp
	data/Nintendo3DSSysTitles.hpp
	data/NintendoLanguage.hpp
	data/NintendoPublishers.hpp
	data/SegaPublishers.hpp
	data/SegaPublishers_data.h
	data/WiiSystemMenuVersion.hpp
	data/WiiUData.hpp
	data/WonderSwanPublishers.hpp
	data/XboxLanguage.hpp
	data/XboxPublishers.hpp
	data/XboxPublishers_data.h
	data/Xbox360_STFS_ContentType.hpp

	disc/CdiReader.hpp
	disc/Cdrom2352Reader.hpp
	disc/CIAReader.hpp
	disc/CisoGcnReader.hpp
	disc/CisoPspReader.hpp
	disc/DpfReader.hpp
	disc/GcnFst.hpp
	disc/GcnPartition.hpp
	disc/GcnPartition_p.hpp
	disc/GczReader.hpp
	disc/GdiReader.hpp
	disc/IResourceReader.hpp
	disc/IsoPartition.hpp
	disc/MultiTrackSparseDiscReader.hpp
	disc/NASOSReader.hpp
	disc/NCCHReader.hpp
	disc/NCCHReader_p.hpp
	disc/NEResourceReader.hpp
	disc/PEResourceReader.hpp
	disc/WbfsReader.hpp
	disc/WiiPartition.hpp
	disc/WiiUFst.hpp
	disc/WiiUH3Reader.hpp
	disc/WuxReader.hpp
	disc/XDVDFSPartition.cpp

	disc/ciso_gcn.h
	disc/ciso_psp_structs.h
	disc/dpf_structs.h
	disc/gcz_structs.h
	disc/libwbfs.h
	disc/nasos_gcn.h
	disc/wux_structs.h
	disc/xdvdfs_structs.h

	config/ImageTypesConfig.hpp
	config/TImageTypesConfig.hpp
	img/TCreateThumbnail.hpp
	img/CacheManager.hpp
	utils/SuperMagicDrive.hpp
	)

IF(ENABLE_XML)
	SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS}
		Console/WiiUPackage_xml.cpp
		Other/EXE_manifest.cpp
		)
	IF(MSVC AND (NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL))
		SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} Other/EXE_delayload.cpp)
	ENDIF(MSVC AND (NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL))
ENDIF(ENABLE_XML)

IF(ENABLE_DECRYPTION)
	SET(${PROJECT_NAME}_CRYPTO_SRCS
		crypto/CtrKeyScrambler.cpp
		crypto/N3DSVerifyKeys.cpp
		crypto/KeyStoreUI.cpp

		Handheld/ndscrypt.cpp
		)
	SET(${PROJECT_NAME}_CRYPTO_H
		crypto/CtrKeyScrambler.hpp
		crypto/N3DSVerifyKeys.hpp
		crypto/KeyStoreUI.hpp

		Handheld/ndscrypt.hpp
		)
ENDIF(ENABLE_DECRYPTION)

# Network-related sources
IF(ENABLE_NETWORKING)
	IF(WIN32)
		SET(${PROJECT_NAME}_NETWORKING_SRCS img/ExecRpDownload_win32.cpp)
	ELSEIF(UNIX)
		SET(${PROJECT_NAME}_NETWORKING_SRCS img/ExecRpDownload_posix.cpp)
	ENDIF()
ENDIF(ENABLE_NETWORKING)
IF(NOT ${PROJECT_NAME}_NETWORKING_SRCS)
	# Dummy implementation for unsupported systems, or if networking is disabled.
	SET(${PROJECT_NAME}_NETWORKING_SRCS img/ExecRpDownload_dummy.cpp)
ENDIF(NOT ${PROJECT_NAME}_NETWORKING_SRCS)

# Windows resource file (for RomData DLL builds only)
IF(WIN32 AND RP_LIBROMDATA_IS_DLL)
	SET(${PROJECT_NAME}_RC res/resource.rc)
ENDIF(WIN32 AND RP_LIBROMDATA_IS_DLL)

# Optimized sources.
INCLUDE(CPUInstructionSetFlags)
IF(CPU_i386 OR CPU_amd64)
	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(${PROJECT_NAME}_IFUNC_SRCS utils/SuperMagicDrive_ifunc.cpp)
	ENDIF(UNIX AND NOT APPLE)

	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://docs.microsoft.com/en-us/cpp/cpp/m64?view=msvc-160
	#
	# amd64 CPUs support SSE2 as a minimum anyway, so there's
	# no point in building MMX code for 64-bit.
	IF(CPU_i386)
		SET(${PROJECT_NAME}_MMX_SRCS utils/SuperMagicDrive_mmx.cpp)
	ENDIF(CPU_i386)
	SET(${PROJECT_NAME}_SSE2_SRCS
		${${PROJECT_NAME}_SSE2_SRCS}
		utils/SuperMagicDrive_sse2.cpp
		)

	IF(MMX_FLAG)
		SET_SOURCE_FILES_PROPERTIES(utils/SuperMagicDrive_mmx.cpp
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${MMX_FLAG} ")
	ENDIF(MMX_FLAG)
	IF(SSE2_FLAG)
		SET_SOURCE_FILES_PROPERTIES(utils/SuperMagicDrive_sse2.cpp
			APPEND_STRING PROPERTIES COMPILE_FLAGS " ${SSE2_FLAG} ")
	ENDIF(SSE2_FLAG)
ENDIF()

# libromdata can be built as a shared library.
IF(RP_LIBROMDATA_IS_DLL)
	SET(${PROJECT_NAME}_LINKAGE SHARED)
	ADD_DEFINITIONS(-DRP_BUILDING_FOR_DLL=1)
	# NOTE: Not using the project version here.
	# This represents the ABI version.
	SET(${PROJECT_NAME}_VERSION 6.0)
	SET(${PROJECT_NAME}_SOVERSION 6)
	IF(MSVC)
		# Delay-load sources.
		SET(${PROJECT_NAME}-DELAYLOAD_SRC ../libwin32common/DelayLoadHelper.c)
		SET(${PROJECT_NAME}-DELAYLOAD_H   ../libwin32common/DelayLoadHelper.h)
	ENDIF(MSVC)
ELSE()
	SET(${PROJECT_NAME}_LINKAGE STATIC)
ENDIF()

IF(ENABLE_XML)
	IF(USE_INTERNAL_XML)
		# Using the internal TinyXML2.
		# It definitely has XMLElement::IntText(int defaultValue).
		SET(HAVE_TINYXML2_XMLELEMENT_INTTEXT 1 CACHE INTERNAL "Test HAVE_TINYXML2_XMLELEMENT_INTTEXT" FORCE)
	ELSE(USE_INTERNAL_XML)
		# Check if TinyXML2 has XMLElement::IntText(int defaultValue).
		# Was added sometime between TinyXML2 4.0.1 and 5.0.0.
		# NOTE: Can't just do an #if on the version macro because
		# TinyXML2 didn't have version macros until 6.2.0.
		SET(CMAKE_REQUIRED_INCLUDES ${TinyXML2_INCLUDE_DIR})
		SET(CMAKE_REQUIRED_LIBRARIES ${TinyXML2_LIBRARY})
		INCLUDE(CheckCXXSourceCompiles)
		CHECK_CXX_SOURCE_COMPILES("#include <stdio.h>
#include \"tinyxml2.h\"
using namespace tinyxml2;

int main(void) {
        XMLDocument document;
        XMLElement *element = document.NewElement(\"test\");
        element->SetText(\"12345\"); \
        int value = element->IntText(0); \
        printf(\"value == %d, should be 12345\", value);
        puts(\"\");

        return !(value == 12345);
}" HAVE_TINYXML2_XMLELEMENT_INTTEXT)
		UNSET(CMAKE_REQUIRED_INCLUDES)
		UNSET(CMAKE_REQUIRED_LIBRARIES)
	ENDIF(USE_INTERNAL_XML)
ENDIF(ENABLE_XML)

# Write the config.h file.
INCLUDE(DirInstallPaths)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.lib${PROJECT_NAME}.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.lib${PROJECT_NAME}.h")

IF(WIN32)
	# Create the manifest file.
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.dll.manifest.in"
		"${CMAKE_CURRENT_BINARY_DIR}/res/${PROJECT_NAME}.dll.manifest" @ONLY)
ENDIF(WIN32)

######################
# Build the library. #
######################

IF(WIN32)
	# Delay-load extlibs.
	INCLUDE(../libwin32common/DelayLoadHelper.cmake)
	SET_DELAYLOAD_EXTLIB_FLAGS()
ENDIF(WIN32)

ADD_LIBRARY(${PROJECT_NAME} ${${PROJECT_NAME}_LINKAGE}
	${${PROJECT_NAME}-DELAYLOAD_SRC} ${${PROJECT_NAME}-DELAYLOAD_H}
	${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_H}
	${${PROJECT_NAME}_NETWORKING_SRCS}
	${${PROJECT_NAME}_CRYPTO_SRCS} ${${PROJECT_NAME}_CRYPTO_H}
	${${PROJECT_NAME}_IFUNC_SRCS}
	${${PROJECT_NAME}_MMX_SRCS}
	${${PROJECT_NAME}_SSE2_SRCS}
	${${PROJECT_NAME}_RC}
	)
IF(ENABLE_PCH)
	TARGET_PRECOMPILE_HEADERS(${PROJECT_NAME} PRIVATE
		"${RP_CONFIG_LIBC_H}" "${RP_COMPILER_COMPAT_H}"
		stdafx.h
		)
ENDIF(ENABLE_PCH)

# Enable PIC.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

IF(WIN32)
	# Append the SOVERSION and "d" for debug.
	SET(CMAKE_DEBUG_POSTFIX "d")
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "-${${PROJECT_NAME}_SOVERSION}d")
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX "-${${PROJECT_NAME}_SOVERSION}")
ENDIF(WIN32)

INCLUDE(SetMSVCDebugPath)
SET_MSVC_DEBUG_PATH(${PROJECT_NAME})

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libromdata
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libromdata
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# top-level
	)

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

IF(${PROJECT_NAME}_LINKAGE STREQUAL SHARED)
	# When linking as a shared library, all internal library components
	# should be linked in as PRIVATE, and include directories added
	# manually. Otherwise, the .a libraries get linked in to the
	# UI frontend plugins.
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpbyteswap-dll rptexture-dll rpfile-dll rpbase-dll rptext-dll)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpthreads-dll cachecommon-dll)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpcpuid)	# for CPU dispatch
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../librpbyteswap>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../librpbyteswap>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../librptexture>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../librptexture>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../librpfile>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../librpfile>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../librpbase>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../librpbase>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../librptext>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../librptext>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../librpthreads>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../librpthreads>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../libcachecommon>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../libcachecommon>
		)
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE i18n-dll)
		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} INTERFACE
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../libi18n>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../libi18n>
			)

		# Need to link to gettext directly, since UI frontends
		# won't link to i18n due to DLL shenanigans.
		INCLUDE(CheckGettext)
		CHECK_GETTEXT()
		IF(Intl_INCLUDE_DIR)
			TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} INTERFACE ${Intl_INCLUDE_DIR})
		ENDIF(Intl_INCLUDE_DIR)
		IF(Intl_LIBRARY)
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ${Intl_LIBRARY})
		ENDIF(Intl_LIBRARY)
	ENDIF(ENABLE_NLS)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE win32common-dll)
		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} INTERFACE
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../libwin32common>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../libwin32common>
			)
	ELSEIF(UNIX)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE unixcommon-dll)
		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} INTERFACE
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../libunixcommon>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../libunixcommon>
			)
	ENDIF()
ELSE()
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC rpbyteswap rptexture rpfile rpbase rptext)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC rpthreads cachecommon)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpcpuid)	# for CPU dispatch
	IF(ENABLE_NLS)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC i18n)
	ENDIF(ENABLE_NLS)
	IF(WIN32)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC win32common)
	ELSEIF(UNIX)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC unixcommon)
	ENDIF()
ENDIF()
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE inih)

# libfmt
IF(Fmt_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${Fmt_LIBRARY})
ENDIF(Fmt_FOUND)

# Shared library stuff
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE RP_BUILDING_LIBROMDATA=1)
IF(${PROJECT_NAME}_LINKAGE STREQUAL SHARED)
	DO_SPLIT_DEBUG(${PROJECT_NAME})
	SET_WINDOWS_SUBSYSTEM(${PROJECT_NAME} WIN32)
	SET_WINDOWS_NO_MANIFEST(${PROJECT_NAME})
	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE RP_BUILDING_FOR_DLL=1)

	# HACK: Needed to ensure that some object files aren't
	# trimmed by the linker, since libromdata doesn't use
	# any of these functions directly. (The UI frontends
	# *do* use these functions.)
	UNSET(SYM_PREFIX)
	SET(SYMS_FORCE
		RP_LibRpBase_AboutTabText_ForceLinkage
		RP_LibRpBase_IconAnimHelper_ForceLinkage
		RP_LibRpBase_RpImageLoader_ForceLinkage
		RP_LibRpBase_TextOut_json_ForceLinkage
		RP_LibRpBase_TextOut_text_ForceLinkage
		RP_LibRpFile_RecursiveScan_ForceLinkage
		RP_LibRpFile_XAttrReader_ForceLinkage
		RP_LibRpFile_XAttrReader_impl_ForceLinkage
		)
	IF(WIN32)
		SET(SYMS_FORCE ${SYMS_FORCE}
			RP_LibRpTexture_GdiplusHelper_ForceLinkage
			RP_LibWin32Common_propsys_xp_ForceLinkage
			)
		IF(CPU_i386)
			SET(SYM_PREFIX "_")
		ENDIF(CPU_i386)
	ELSE(WIN32)
		# FIXME: Getting a missing symbol error on MSVC ARM64EC builds if these
		# are unconditionally added to SYMS_FORCE.
		SET(SYMS_FORCE ${SYMS_FORCE}
			ext2AttrCheckboxInfo	# librpfile
			xfsAttrCheckboxInfo	# librpfile
			)
	ENDIF(WIN32)
	IF(MSVC)
		FOREACH(_sym ${SYMS_FORCE})
			SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /include:${SYM_PREFIX}${_sym}")
		ENDFOREACH(_sym)
	ELSE(MSVC)
		FOREACH(_sym ${SYMS_FORCE})
			SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--undefined=${SYM_PREFIX}${_sym}")
		ENDFOREACH(_sym)
	ENDIF(MSVC)
	UNSET(SYM_PREFIX)
ENDIF(${PROJECT_NAME}_LINKAGE STREQUAL SHARED)
# SOVERSION
IF(${PROJECT_NAME}_SOVERSION)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
		VERSION ${${PROJECT_NAME}_VERSION}
		SOVERSION ${${PROJECT_NAME}_SOVERSION}
		)
ENDIF(${PROJECT_NAME}_SOVERSION)

IF(WIN32)
	# librpsecure: integrity_level.h
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE rpsecure)

	# Windows SDK libraries
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE advapi32)

	# Delay-load shell32.dll and ole32.dll to prevent a performance penalty due to gdi32.dll.
	# Also delay-load user32.dll to prevent user object allocation when it's not necessary.
	# Reference: https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
	ADD_DELAYLOAD_FLAGS(${PROJECT_NAME} shell32.dll ole32.dll user32.dll)
	IF(MSVC)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE delayimp)
	ENDIF(MSVC)
ENDIF(WIN32)

# Other libraries.
# NOTE: Marking these as PUBLIC so the UI frontends can use them.
# NOTE: Nettle isn't used directly, but librpbase is PRIVATE if
# libromdata is built as a shared library.
IF(ZLIB_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${ZLIB_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ${ZLIB_DEFINITIONS})

	# MiniZip (used by J2ME)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE minizip)
ELSE(ZLIB_FOUND)
	MESSAGE(FATAL_ERROR "ZLIB_LIBRARIES has not been set by CheckZLIB.cmake.")
ENDIF(ZLIB_FOUND)
IF(PNG_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${PNG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${PNG_INCLUDE_DIRS})
	TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE ${PNG_DEFINITIONS})
ENDIF(PNG_FOUND)
IF(JPEG_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${JPEG_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${JPEG_INCLUDE_DIRS})
ENDIF(JPEG_FOUND)
IF(ENABLE_DECRYPTION AND NOT WIN32)
	INCLUDE(CheckNettle2or3)
	CHECK_NETTLE_2_OR_3()
	IF(NETTLE_FOUND)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${NETTLE_LIBRARY})
		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${NETTLE_INCLUDE_DIRS})
	ENDIF(NETTLE_FOUND)
ENDIF(ENABLE_DECRYPTION AND NOT WIN32)

IF(ENABLE_XML AND TinyXML2_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${TinyXML2_LIBRARY})
	TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${TinyXML2_INCLUDE_DIR})
ENDIF(ENABLE_XML AND TinyXML2_FOUND)

IF(ENABLE_UNICE68)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE unice68_lib)
ENDIF(ENABLE_UNICE68)

IF(ENABLE_LIBMSPACK)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE mspack)
ENDIF(ENABLE_LIBMSPACK)

IF(ENABLE_LZ4 AND LZ4_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${LZ4_LIBRARY})
ENDIF(ENABLE_LZ4 AND LZ4_FOUND)
IF(ENABLE_LZO AND LZO_FOUND)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${LZO_LIBRARY})
ENDIF(ENABLE_LZO AND LZO_FOUND)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)

######################################
# Install the library. (SHARED only) #
######################################

IF(${PROJECT_NAME}_LINKAGE STREQUAL SHARED)
	IF(WIN32)
		# NOTE: Don't install libraries.
		# That installs the import library, which we don't
		# want to distribute.
		INSTALL(TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
			LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
			#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
			COMPONENT "dll"
			)
	ELSE(WIN32)
		INSTALL(TARGETS ${PROJECT_NAME}
			LIBRARY DESTINATION "${DIR_INSTALL_LIB}"
			COMPONENT "lib"
			)
	ENDIF(WIN32)

	# Check if a split debug file should be installed.
	IF(INSTALL_DEBUG)
		# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
		GET_TARGET_PROPERTY(DEBUG_FILENAME ${PROJECT_NAME} PDB)
		IF(WIN32)
			# TODO: Test on MinGW.
			# FIXME: Generator PDB expression doesn't have the debug postfix for some reason,
			# so cpack fails in debug builds if we get the PDB property.
			SET(PDB_FILENAME_D "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION}d.pdb")
			SET(PDB_FILENAME_R "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}-${${PROJECT_NAME}_SOVERSION}.pdb")
			INSTALL(FILES "${PDB_FILENAME_D}" "${PDB_FILENAME_R}"
				DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
				COMPONENT "debug"
				OPTIONAL
				)
			UNSET(PDB_FILENAME_D)
			UNSET(PDB_FILENAME_R)
		ELSE(WIN32)
			INSTALL(FILES "${DEBUG_FILENAME}"
				DESTINATION "${DIR_INSTALL_DLL_DEBUG}"
				COMPONENT "debug"
				)
		ENDIF(WIN32)
		UNSET(DEBUG_FILENAME)
	ENDIF(INSTALL_DEBUG)
ENDIF(${PROJECT_NAME}_LINKAGE STREQUAL SHARED)
