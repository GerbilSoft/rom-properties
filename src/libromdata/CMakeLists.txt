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
	data/MegaDrivePublishers.cpp
	data/NintendoPublishers.cpp
	data/AmiiboData.cpp
	data/WiiSystemMenuVersion.cpp
	data/NESMappers.cpp
	data/EXEData.cpp
	data/WiiUData.cpp
	data/Nintendo3DSFirmData.cpp
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
	#config/TImageTypesConfig.cpp	# NOT listed here due to template stuff.
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
	data/MegaDrivePublishers.hpp
	data/NintendoPublishers.hpp
	data/AmiiboData.hpp
	data/WiiSystemMenuVersion.hpp
	data/NESMappers.hpp
	data/EXEData.hpp
	data/WiiUData.hpp
	data/Nintendo3DSFirmData.hpp
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
	config/TImageTypesConfig.hpp
	)

IF(ENABLE_XML)
	SET(libromdata_SRCS ${libromdata_SRCS} EXE_manifest.cpp)
	IF(WIN32 AND (NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL))
		SET(libromdata_SRCS ${libromdata_SRCS} EXE_delayload.cpp)
	ENDIF(WIN32 AND (NOT USE_INTERNAL_XML OR USE_INTERNAL_XML_DLL))
ENDIF(ENABLE_XML)

IF(ENABLE_DECRYPTION)
	SET(libromdata_CRYPTO_SRCS
		crypto/CtrKeyScrambler.cpp
		disc/NCCHReader_VerifyKeys.cpp
		)
	SET(libromdata_CRYPTO_H
		crypto/CtrKeyScrambler.hpp
		)
ENDIF(ENABLE_DECRYPTION)

######################
# Build the library. #
######################

# UTF-8 version.
ADD_LIBRARY(romdata8 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	)
TARGET_LINK_LIBRARIES(romdata8 rpbase8)

# UTF-16 version.
ADD_LIBRARY(romdata16 STATIC
	${libromdata_SRCS} ${libromdata_H}
	${libromdata_OS_SRCS} ${libromdata_OS_H}
	${libromdata_CRYPTO_SRCS} ${libromdata_CRYPTO_H}
	)
TARGET_LINK_LIBRARIES(romdata16 rpbase16)

# Common properties for romdata8 and romdata16.
FOREACH(_target romdata8 romdata16)
	# Include paths:
	# - Public: Current source and binary directories.
	# - Private: Parent source and binary directories,
	#            and top-level binary directory for git_version.h.
	TARGET_INCLUDE_DIRECTORIES(${_target}
		PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libromdata
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libromdata
		PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
			$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# top-level
		)
	IF(MSVC)
		TARGET_LINK_LIBRARIES(${_target} delayimp)
	ENDIF(MSVC)
	# Exclude from ALL builds.
	SET_TARGET_PROPERTIES(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)

	# Other libraries.
	IF(ENABLE_XML AND TinyXML2_FOUND)
		TARGET_LINK_LIBRARIES(${_target} ${TinyXML2_LIBRARY})
	ENDIF(ENABLE_XML AND TinyXML2_FOUND)
ENDFOREACH()

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)

# Test suite.
IF(BUILD_TESTING)
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTING)
