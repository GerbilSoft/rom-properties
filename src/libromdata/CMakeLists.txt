PROJECT(libromdata)

# Sources.
SET(libromdata_SRCS
	MegaDrive.cpp
	MegaDriveRegions.cpp
	GameCube.cpp
	NintendoDS.cpp
	RomDataFactory.cpp
	DMG.cpp
	GameBoyAdvance.cpp
	GameCubeSave.cpp
	N64.cpp
	SNES.cpp
	DreamcastSave.cpp
	PlayStationSave.cpp
	VirtualBoy.cpp
	Amiibo.cpp
	NES.cpp
	EXE.cpp
	WiiU.cpp
	Nintendo3DS.cpp
	Nintendo3DSFirm.cpp
	Sega8Bit.cpp
	SegaPVR.cpp
	DirectDrawSurface.cpp
	NintendoBadge.cpp
	Dreamcast.cpp
	SegaSaturn.cpp
	Lynx.cpp
	data/SegaPublishers.cpp
	data/NintendoPublishers.cpp
	data/AmiiboData.cpp
	data/WiiSystemMenuVersion.cpp
	data/NESMappers.cpp
	data/EXEData.cpp
	data/WiiUData.cpp
	data/Nintendo3DSFirmData.cpp
	data/Nintendo3DSSysTitles.cpp
	data/NintendoLanguage.cpp
	#img/TCreateThumbnail.cpp	# NOT listed here due to template stuff.
	disc/WbfsReader.cpp
	disc/CisoGcnReader.cpp
	disc/GcnPartition.cpp
	disc/GcnPartitionPrivate.cpp
	disc/WiiPartition.cpp
	disc/GcnFst.cpp
	disc/PEResourceReader.cpp
	disc/NEResourceReader.cpp
	disc/NCCHReader.cpp
	disc/CIAReader.cpp
	disc/Cdrom2352Reader.cpp
	disc/IsoPartition.cpp
	disc/GdiReader.cpp
	#config/TImageTypesConfig.cpp	# NOT listed here due to template stuff.
	utils/SuperMagicDrive.cpp
	)
# Headers.
SET(libromdata_H
	MegaDrive.hpp
	MegaDriveRegions.hpp
	GameCube.hpp
	NintendoDS.hpp
	RomDataFactory.hpp
	DMG.hpp
	GameBoyAdvance.hpp
	CopierFormats.h
	GameCubeSave.hpp
	PlayStationSave.hpp
	N64.hpp
	SNES.hpp
	DreamcastSave.hpp
	VirtualBoy.hpp
	Amiibo.hpp
	NES.hpp
	EXE.hpp
	EXE_p.hpp
	WiiU.hpp
	Nintendo3DS.hpp
	Nintendo3DSFirm.hpp
	Sega8Bit.hpp
	SegaPVR.hpp
	DirectDrawSurface.hpp
	NintendoBadge.hpp
	Dreamcast.hpp
	SegaSaturn.hpp
	Lynx.hpp
	md_structs.h
	gcn_structs.h
	gcn_card.h
	nds_structs.h
	n64_structs.h
	dmg_structs.h
	gba_structs.h
	ps1_structs.h
	snes_structs.h
	dc_structs.h
	vb_structs.h
	nfp_structs.h
	nes_structs.h
	wiiu_structs.h
	n3ds_structs.h
	sega8_structs.h
	pvr_structs.h
	dds_structs.h
	badge_structs.h
	cdrom_structs.h
	iso_structs.h
	saturn_structs.h
	lnx_structs.h
	data/SegaPublishers.hpp
	data/NintendoPublishers.hpp
	data/AmiiboData.hpp
	data/WiiSystemMenuVersion.hpp
	data/NESMappers.hpp
	data/EXEData.hpp
	data/WiiUData.hpp
	data/Nintendo3DSFirmData.hpp
	data/Nintendo3DSSysTitles.hpp
	data/NintendoLanguage.hpp
	img/TCreateThumbnail.hpp
	disc/WbfsReader.hpp
	disc/libwbfs.h
	disc/CisoGcnReader.hpp
	disc/ciso_gcn.h
	disc/GcnPartition.hpp
	disc/GcnPartitionPrivate.hpp
	disc/WiiPartition.hpp
	disc/GcnFst.hpp
	disc/IResourceReader.hpp
	disc/PEResourceReader.hpp
	disc/NEResourceReader.hpp
	disc/NCCHReader.hpp
	disc/NCCHReader_p.hpp
	disc/CIAReader.hpp
	disc/Cdrom2352Reader.hpp
	disc/IsoPartition.hpp
	disc/GdiReader.hpp
	config/TImageTypesConfig.hpp
	utils/SuperMagicDrive.hpp
	)

IF(ENABLE_XML)
	SET(libromdata_SRCS ${libromdata_SRCS} EXE_manifest.cpp)
	IF(MSVC AND (NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL))
		SET(libromdata_SRCS ${libromdata_SRCS} EXE_delayload.cpp)
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

# Optimized sources.
STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
IF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
	# IFUNC requires glibc.
	# We're not checking for glibc here, but we do have preprocessor
	# checks, so even if this does get compiled on a non-glibc system,
	# it won't do anything.
	# TODO: Might be supported on other Unix-like operating systems...
	IF(UNIX AND NOT APPLE)
		SET(libromdata_IFUNC_SRCS utils/SuperMagicDrive_ifunc.cpp)
	ENDIF(UNIX AND NOT APPLE)

	# MSVC does not support MMX intrinsics in 64-bit builds.
	# Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx
	# TODO: Disable MMX on all 64-bit builds?
	IF(NOT MSVC OR NOT CMAKE_CL_64)
		SET(libromdata_SSE2_SRCS utils/SuperMagicDrive_mmx.cpp)
	ENDIF(NOT MSVC OR NOT CMAKE_CL_64)
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
UNSET(arch)

######################
# Build the library. #
######################

ADD_LIBRARY(romdata STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	${libromdata_IFUNC_SRCS}
	${libromdata_SSE2_SRCS}
	)
TARGET_LINK_LIBRARIES(romdata rpbase)

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
IF(MSVC)
	TARGET_LINK_LIBRARIES(romdata delayimp)
ENDIF(MSVC)
# Exclude from ALL builds.
SET_TARGET_PROPERTIES(romdata PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Other libraries.
TARGET_LINK_LIBRARIES(romdata ${ZLIB_LIBRARY})
IF(ENABLE_XML AND TinyXML2_FOUND)
	TARGET_LINK_LIBRARIES(romdata ${TinyXML2_LIBRARY})
ENDIF(ENABLE_XML AND TinyXML2_FOUND)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
