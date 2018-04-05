PROJECT(libromdata)

# Check for gl.h.
# FIXME: This check is failing with MSVC...
IF(NOT MSVC)
	IF(APPLE)
		SET(GL_H_FILENAME "OpenGL/gl.h")
	ELSE(APPLE)
		SET(GL_H_FILENAME "GL/gl.h")
	ENDIF(APPLE)
	CHECK_INCLUDE_FILES(${GL_H_FILENAME} HAVE_GL_H)
	IF(NOT HAVE_GL_H)
		SET(ERR_MSG "gl.h not found. (Required for Khronos KTX.)")
		IF(UNIX AND NOT APPLE)
			SET(ERR_MSG "${ERR_MSG} Install one of these packages:
  - Debian/Ubuntu: mesa-common-dev
  - Red Hat/Fedora: mesa-libGL-devel")
		ENDIF(UNIX AND NOT APPLE)
		MESSAGE(FATAL_ERROR "${ERR_MSG}")
	ENDIF(NOT HAVE_GL_H)
ENDIF(NOT MSVC)

# Sources.
SET(libromdata_SRCS
	RomDataFactory.cpp

	Console/MegaDrive.cpp
	Console/MegaDriveRegions.cpp
	Console/Dreamcast.cpp
	Console/DreamcastSave.cpp
	Console/GameCube.cpp
	Console/GameCubeSave.cpp
	Console/N64.cpp
	Console/NES.cpp
	Console/Sega8Bit.cpp
	Console/SegaSaturn.cpp
	Console/SNES.cpp
	Console/PlayStationSave.cpp
	Console/WiiU.cpp

	Handheld/DMG.cpp
	Handheld/Lynx.cpp
	Handheld/GameBoyAdvance.cpp
	Handheld/Nintendo3DS.cpp
	Handheld/Nintendo3DSFirm.cpp
	Handheld/NintendoDS.cpp
	Handheld/VirtualBoy.cpp

	Texture/DirectDrawSurface.cpp
	Texture/KhronosKTX.cpp
	Texture/SegaPVR.cpp
	Texture/ValveVTF.cpp
	Texture/ValveVTF3.cpp

	Other/Amiibo.cpp
	Other/ELF.cpp
	Other/EXE.cpp
	Other/NintendoBadge.cpp

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
	data/GLenumStrings.cpp
	data/ELFData.cpp
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
	RomDataFactory.hpp
	CopierFormats.h
	cdrom_structs.h
	iso_structs.h

	Console/Dreamcast.hpp
	Console/DreamcastSave.hpp
	Console/GameCube.hpp
	Console/GameCubeSave.hpp
	Console/MegaDrive.hpp
	Console/MegaDriveRegions.hpp
	Console/N64.hpp
	Console/NES.hpp
	Console/PlayStationSave.hpp
	Console/Sega8Bit.hpp
	Console/SegaSaturn.hpp
	Console/SNES.hpp
	Console/WiiU.hpp

	Console/dc_structs.h
	Console/gcn_structs.h
	Console/gcn_card.h
	Console/md_structs.h
	Console/n64_structs.h
	Console/nes_structs.h
	Console/ps1_structs.h
	Console/saturn_structs.h
	Console/sega8_structs.h
	Console/snes_structs.h
	Console/wiiu_structs.h

	Handheld/DMG.hpp
	Handheld/Lynx.hpp
	Handheld/GameBoyAdvance.hpp
	Handheld/Nintendo3DS.hpp
	Handheld/Nintendo3DSFirm.hpp
	Handheld/NintendoDS.hpp
	Handheld/VirtualBoy.hpp

	Handheld/dmg_structs.h
	Handheld/gba_structs.h
	Handheld/n3ds_structs.h
	Handheld/nds_structs.h
	Handheld/lnx_structs.h
	Handheld/vb_structs.h

	Texture/DirectDrawSurface.hpp
	Texture/KhronosKTX.hpp
	Texture/SegaPVR.hpp
	Texture/ValveVTF.hpp
	Texture/ValveVTF3.hpp

	Texture/dds_structs.h
	Texture/ktx_structs.h
	Texture/pvr_structs.h
	Texture/vtf_structs.h
	Texture/vtf3_structs.h

	Other/Amiibo.hpp
	Other/ELF.hpp
	Other/EXE.hpp
	Other/EXE_p.hpp
	Other/NintendoBadge.hpp

	Other/badge_structs.h
	Other/elf_structs.h
	Other/exe_structs.h
	Other/nfp_structs.h

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
	data/GLenumStrings.hpp
	data/ELFData.hpp
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
IF(ENABLE_NLS)
	TARGET_LINK_LIBRARIES(romdata i18n)
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
