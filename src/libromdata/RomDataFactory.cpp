/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataFactory.cpp: RomData factory class.                              *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "libromdata/config.libromdata.h"

#include "RomDataFactory.hpp"
#include "RomData_p.hpp"	// for RomDataInfo

// librpbase, librpfile
#include "librpfile/RelatedFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// librpthreads
#include "librpthreads/pthread_once.h"

// librptexture
#include "librptexture/FileFormatFactory.hpp"
using LibRpTexture::FileFormatFactory;

// C++ STL classes.
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

// RomData subclasses: Consoles
#include "Console/CBMCart.hpp"
#include "Console/Dreamcast.hpp"
#include "Console/DreamcastSave.hpp"
#include "Console/GameCube.hpp"
#include "Console/GameCubeBNR.hpp"
#include "Console/GameCubeSave.hpp"
#include "Console/iQuePlayer.hpp"
#include "Console/MegaDrive.hpp"
#include "Console/N64.hpp"
#include "Console/NES.hpp"
#include "Console/PlayStationEXE.hpp"
#include "Console/PlayStationSave.hpp"
#include "Console/Sega8Bit.hpp"
#include "Console/SegaSaturn.hpp"
#include "Console/SNES.hpp"
#include "Console/SufamiTurbo.hpp"
#include "Console/WiiSave.hpp"
#include "Console/WiiU.hpp"
#include "Console/WiiWAD.hpp"
#include "Console/WiiWIBN.hpp"
#include "Console/Xbox_XBE.hpp"
#include "Console/Xbox360_STFS.hpp"
#include "Console/Xbox360_XDBF.hpp"
#include "Console/Xbox360_XEX.hpp"

// Special handling for Xbox and PlayStation discs.
#include "cdrom_structs.h"
#include "iso_structs.h"
#include "Console/XboxDisc.hpp"
#include "Console/PlayStationDisc.hpp"
#include "disc/xdvdfs_structs.h"

// RomData subclasses: Handhelds
#include "Handheld/DMG.hpp"
#include "Handheld/GameBoyAdvance.hpp"
#include "Handheld/GameCom.hpp"
#include "Handheld/Lynx.hpp"
#include "Handheld/NGPC.hpp"
#include "Handheld/Nintendo3DS.hpp"
#include "Handheld/Nintendo3DSFirm.hpp"
#include "Handheld/Nintendo3DS_SMDH.hpp"
#include "Handheld/NintendoDS.hpp"
#include "Handheld/PokemonMini.hpp"
#include "Handheld/PSP.hpp"
#include "Handheld/VirtualBoy.hpp"
#include "Handheld/WonderSwan.hpp"

// RomData subclasses: Audio
#include "Audio/ADX.hpp"
#include "Audio/BCSTM.hpp"
#include "Audio/BRSTM.hpp"
#include "Audio/GBS.hpp"
#include "Audio/NSF.hpp"
#include "Audio/PSF.hpp"
#include "Audio/SAP.hpp"
#include "Audio/SNDH.hpp"
#include "Audio/SID.hpp"
#include "Audio/SPC.hpp"
#include "Audio/VGM.hpp"

// RomData subclasses: Other
#include "Other/Amiibo.hpp"
#include "Other/ELF.hpp"
#include "Other/EXE.hpp"
#include "Other/ISO.hpp"
#include "Other/MachO.hpp"
#include "Other/NintendoBadge.hpp"
#include "Other/RpTextureWrapper.hpp"
#include "Other/Lua.hpp"

// Special case for Dreamcast save files.
#include "Console/dc_structs.h"

namespace LibRomData {

class RomDataFactoryPrivate
{
	private:
		RomDataFactoryPrivate();
		~RomDataFactoryPrivate();

	private:
		RP_DISABLE_COPY(RomDataFactoryPrivate)

	public:
		typedef int (*pfnIsRomSupported_t)(const RomData::DetectInfo *info);
		typedef const RomDataInfo * (*pfnRomDataInfo_t)(void);
		typedef RomData* (*pfnNewRomData_t)(IRpFile *file);

		struct RomDataFns {
			pfnIsRomSupported_t isRomSupported;
			pfnNewRomData_t newRomData;
			pfnRomDataInfo_t romDataInfo;
			unsigned int attrs;

			// Extra fields for files whose headers
			// appear at specific addresses.
			uint32_t address;
			uint32_t size;	// Contains magic number for fast 32-bit magic checking.
		};

		/**
		 * Templated function to construct a new RomData subclass.
		 * @param klass Class name.
		 */
		template<typename klass>
		static LibRpBase::RomData *RomData_ctor(LibRpFile::IRpFile *file)
		{
			return new klass(file);
		}

#define GetRomDataFns(sys, attrs) \
	{sys::isRomSupported_static, \
	 RomDataFactoryPrivate::RomData_ctor<sys>, \
	 sys::romDataInfo, \
	 attrs, 0, 0}

#define GetRomDataFns_addr(sys, attrs, address, size) \
	{sys::isRomSupported_static, \
	 RomDataFactoryPrivate::RomData_ctor<sys>, \
	 sys::romDataInfo, \
	 attrs, address, size}

		// RomData subclasses that use a header at 0 and
		// definitely have a 32-bit magic number in the header.
		// - address: Address of magic number within the header.
		// - size: 32-bit magic number.
		static const RomDataFns romDataFns_magic[];

		// RomData subclasses that use a header.
		// Headers with addresses other than 0 should be
		// placed at the end of this array.
		static const RomDataFns romDataFns_header[];

		// RomData subclasses that use a footer.
		static const RomDataFns romDataFns_footer[];

		// Table of pointers to tables.
		// This reduces duplication by only requiring a single loop
		// in each function.
		static const RomDataFns *const romDataFns_tbl[];

		/**
		 * Attempt to open the other file in a Dreamcast .VMI+.VMS pair.
		 * @param file One opened file in the .VMI+.VMS pair.
		 * @return DreamcastSave if valid; nullptr if not.
		 */
		static RomData *openDreamcastVMSandVMI(IRpFile *file);

		// Vectors for file extensions and MIME types.
		// We want to collect them once per session instead of
		// repeatedly collecting them, since the caller might
		// not cache them.
		// pthread_once() control variable.
		static vector<RomDataFactory::ExtInfo> vec_exts;
		static vector<const char*> vec_mimeTypes;
		static pthread_once_t once_exts;
		static pthread_once_t once_mimeTypes;

		/**
		 * Initialize the vector of supported file extensions.
		 * Used for Win32 COM registration.
		 *
		 * Internal function; must be called using pthread_once().
		 *
		 * NOTE: The return value is a struct that includes a flag
		 * indicating if the file type handler supports thumbnails.
		 */
		static void init_supportedFileExtensions(void);

		/**
		 * Initialize the vector of supported MIME types.
		 * Used for KFileMetaData.
		 *
		 * Internal function; must be called using pthread_once().
		 */
		static void init_supportedMimeTypes(void);

		/**
		 * Check an ISO-9660 disc image for a game-specific file system.
		 *
		 * If this is a valid ISO-9660 disc image, but no game-specific
		 * RomData subclasses support it, an ISO object will be returned.
		 *
		 * @param file ISO-9660 disc image
		 * @return Game-specific RomData subclass, or nullptr if none are supported.
		 */
		static RomData *checkISO(IRpFile *file);
};

/** RomDataFactoryPrivate **/

vector<RomDataFactory::ExtInfo> RomDataFactoryPrivate::vec_exts;
vector<const char*> RomDataFactoryPrivate::vec_mimeTypes;
pthread_once_t RomDataFactoryPrivate::once_exts = PTHREAD_ONCE_INIT;
pthread_once_t RomDataFactoryPrivate::once_mimeTypes = PTHREAD_ONCE_INIT;

#define ATTR_NONE		RomDataFactory::RDA_NONE
#define ATTR_HAS_THUMBNAIL	RomDataFactory::RDA_HAS_THUMBNAIL
#define ATTR_HAS_DPOVERLAY	RomDataFactory::RDA_HAS_DPOVERLAY
#define ATTR_HAS_METADATA	RomDataFactory::RDA_HAS_METADATA
#define ATTR_CHECK_ISO		RomDataFactory::RDA_CHECK_ISO
#define ATTR_SUPPORTS_DEVICES	RomDataFactory::RDA_SUPPORTS_DEVICES

// RomData subclasses that use a header at 0 and
// definitely have a 32-bit magic number in the header.
// - address: Address of magic number within the header.
// - size: 32-bit magic number.
// TODO: Add support for multiple magic numbers per class.
const RomDataFactoryPrivate::RomDataFns RomDataFactoryPrivate::romDataFns_magic[] = {
	// Consoles
	GetRomDataFns_addr(PlayStationEXE, 0, 0, 'PS-X'),
	GetRomDataFns_addr(SufamiTurbo, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 8, 'FC-A'),	// Less common than "BAND"
	GetRomDataFns_addr(WiiU, ATTR_HAS_THUMBNAIL | ATTR_SUPPORTS_DEVICES, 0, 'WUP-'),
	GetRomDataFns_addr(WiiU, ATTR_HAS_THUMBNAIL | ATTR_SUPPORTS_DEVICES, 0, 'WUX0'),
	GetRomDataFns_addr(WiiWIBN, ATTR_HAS_THUMBNAIL, 0, 'WIBN'),
	GetRomDataFns_addr(Xbox_XBE, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'XBEH'),
	GetRomDataFns_addr(Xbox360_XDBF, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'XDBF'),
	GetRomDataFns_addr(Xbox360_XEX, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'XEX1'),
	GetRomDataFns_addr(Xbox360_XEX, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'XEX2'),

	// Handhelds
	GetRomDataFns_addr(DMG, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0x104, 0xCEED6666),
	GetRomDataFns_addr(DMG, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0x304, 0xCEED6666),	// headered
	GetRomDataFns_addr(GameBoyAdvance, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0x04, 0x24FFAE51),
	GetRomDataFns_addr(Lynx, ATTR_NONE, 0, 'LYNX'),
	GetRomDataFns_addr(NGPC, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 12, ' SNK'),
	GetRomDataFns_addr(Nintendo3DSFirm, ATTR_NONE, 0, 'FIRM'),
	GetRomDataFns_addr(Nintendo3DS_SMDH, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'SMDH'),
	GetRomDataFns_addr(NintendoDS, ATTR_HAS_THUMBNAIL | ATTR_HAS_DPOVERLAY | ATTR_HAS_METADATA, 0xC0, 0x24FFAE51),
	GetRomDataFns_addr(NintendoDS, ATTR_HAS_THUMBNAIL | ATTR_HAS_DPOVERLAY | ATTR_HAS_METADATA, 0xC0, 0xC8604FE2),
	GetRomDataFns_addr(PSP, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'CISO'),
#ifdef HAVE_LZ4
	GetRomDataFns_addr(PSP, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'ZISO'),
#endif /* HAVE_LZ4 */
#ifdef HAVE_LZO
	GetRomDataFns_addr(PSP, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'JISO'),
#endif /* HAVE_LZO */
	GetRomDataFns_addr(PSP, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 0x44415800),	// 'DAX\0'

	// Audio
	GetRomDataFns_addr(BRSTM, ATTR_HAS_METADATA, 0, 'RSTM'),
	GetRomDataFns_addr(GBS, ATTR_HAS_METADATA, 0, 0x47425301U),	// 'GBS\x01'
	GetRomDataFns_addr(GBS, ATTR_HAS_METADATA, 0, 0x47425246U),	// 'GBRF'
	GetRomDataFns_addr(NSF, ATTR_HAS_METADATA, 0, 'NESM'),
	GetRomDataFns_addr(SPC, ATTR_HAS_METADATA, 0, 'SNES'),
	GetRomDataFns_addr(VGM, ATTR_HAS_METADATA, 0, 'Vgm '),

	// Other
	GetRomDataFns_addr(ELF, ATTR_NONE, 0, 0x7F454C46),		// '\177ELF'
	GetRomDataFns_addr(Lua, ATTR_NONE, 0, 0x1B4C7561),		// '\033Lua'

	// Consoles: Xbox 360 STFS
	// Moved here to prevent conflicts with the Nintendo DS ROM image
	// "Live On Card Live-R DS".
	GetRomDataFns_addr(Xbox360_STFS, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'CON '),
	GetRomDataFns_addr(Xbox360_STFS, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'PIRS'),
	GetRomDataFns_addr(Xbox360_STFS, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'LIVE'),

	// Consoles: CBMCart
	// Moved here because they're less common.
	GetRomDataFns_addr(CBMCart, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'C64 '),
	GetRomDataFns_addr(CBMCart, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'C128'),
	GetRomDataFns_addr(CBMCart, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'CBM2'),
	GetRomDataFns_addr(CBMCart, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'VIC2'),
	GetRomDataFns_addr(CBMCart, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0, 'PLUS'),

	{nullptr, nullptr, nullptr, ATTR_NONE, 0, 0}
};

// RomData subclasses that use a header.
// Headers with addresses other than 0 should be
// placed at the end of this array.
const RomDataFactoryPrivate::RomDataFns RomDataFactoryPrivate::romDataFns_header[] = {
	// Consoles
	GetRomDataFns(Dreamcast, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA | ATTR_SUPPORTS_DEVICES),
	GetRomDataFns(DreamcastSave, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),
	GetRomDataFns(GameCube, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA | ATTR_SUPPORTS_DEVICES),
	GetRomDataFns(GameCubeBNR, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),
	GetRomDataFns(GameCubeSave, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),
	GetRomDataFns(iQuePlayer, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),
	// MegaDrive: ATTR_SUPPORTS_DEVICES for Sega CD
	GetRomDataFns(MegaDrive, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA | ATTR_SUPPORTS_DEVICES),
	GetRomDataFns(N64, ATTR_HAS_METADATA),
	GetRomDataFns(NES, ATTR_NONE),
	GetRomDataFns(SNES, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),
	GetRomDataFns(SegaSaturn, ATTR_NONE | ATTR_HAS_METADATA | ATTR_SUPPORTS_DEVICES),
	GetRomDataFns(WiiSave, ATTR_HAS_THUMBNAIL),
	GetRomDataFns(WiiWAD, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),

	// Handhelds
	GetRomDataFns(Nintendo3DS, ATTR_HAS_THUMBNAIL | ATTR_HAS_DPOVERLAY | ATTR_HAS_METADATA),

	// Audio
	GetRomDataFns(ADX, ATTR_HAS_METADATA),
	GetRomDataFns(BCSTM, ATTR_HAS_METADATA),
	GetRomDataFns(PSF, ATTR_HAS_METADATA),
	GetRomDataFns(SAP, ATTR_HAS_METADATA),	// "SAP\r\n", "SAP\n"; maybe move to _magic[]?
	GetRomDataFns(SNDH, ATTR_HAS_METADATA),	// "SNDH", or "ICE!" or "Ice!" if packed.
	GetRomDataFns(SID, ATTR_HAS_METADATA),	// PSID/RSID; maybe move to _magic[]?

	// Other
	GetRomDataFns(Amiibo, ATTR_HAS_THUMBNAIL),
	GetRomDataFns(MachO, ATTR_NONE),
	GetRomDataFns(NintendoBadge, ATTR_HAS_THUMBNAIL),

	// The following formats have 16-bit magic numbers,
	// so they should go at the end of the address=0 section.
	GetRomDataFns(EXE, ATTR_HAS_DPOVERLAY),	// TODO: Thumbnailing on non-Windows platforms.
	GetRomDataFns(PlayStationSave, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),

	// NOTE: game.com may be at either 0 or 0x40000.
	// The 0x40000 address is checked below.
	GetRomDataFns(GameCom, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),

	// Headers with non-zero addresses.
	GetRomDataFns_addr(Sega8Bit, ATTR_HAS_METADATA, 0x7FE0, 0x20),
	GetRomDataFns_addr(PokemonMini, ATTR_HAS_METADATA, 0x2100, 0xD0),
	// NOTE: game.com may be at either 0 or 0x40000.
	// The 0 address is checked above.
	GetRomDataFns_addr(GameCom, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA, 0x40000, 0x20),

	// Last chance: ISO-9660 disc images.
	// NOTE: This might include some console-specific disc images
	// that don't have an identifying boot sector at 0x0000.
	// NOTE: Keeping the same address as the previous entry, since ISO only checks the file extension.
	// NOTE: ATTR_HAS_THUMBNAIL is needed for Xbox 360.
	GetRomDataFns_addr(ISO, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA | ATTR_SUPPORTS_DEVICES | ATTR_CHECK_ISO, 0x40000, 0x20),

	{nullptr, nullptr, nullptr, ATTR_NONE, 0, 0}
};

// RomData subclasses that use a footer.
const RomDataFactoryPrivate::RomDataFns RomDataFactoryPrivate::romDataFns_footer[] = {
	GetRomDataFns(VirtualBoy, ATTR_NONE),
	GetRomDataFns(WonderSwan, ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA),
	{nullptr, nullptr, nullptr, ATTR_NONE, 0, 0}
};

// Table of pointers to tables.
// This reduces duplication by only requiring a single loop
// in each function.
const RomDataFactoryPrivate::RomDataFns *const RomDataFactoryPrivate::romDataFns_tbl[] = {
	romDataFns_magic,
	romDataFns_header,
	romDataFns_footer,
	nullptr
};

/**
 * Attempt to open the other file in a Dreamcast .VMI+.VMS pair.
 * @param file One opened file in the .VMI+.VMS pair.
 * @return DreamcastSave if valid; nullptr if not.
 */
RomData *RomDataFactoryPrivate::openDreamcastVMSandVMI(IRpFile *file)
{
	// We're assuming the file extension was already checked.
	// VMS files are always a multiple of 512 bytes,
	// or 160 bytes for some monochrome ICONDATA_VMS.
	// VMI files are always 108 bytes;
	off64_t filesize = file->size();
	bool has_dc_vms = (filesize % DC_VMS_BLOCK_SIZE == 0) ||
			  (filesize == DC_VMS_ICONDATA_MONO_MINSIZE);
	bool has_dc_vmi = (filesize == sizeof(DC_VMI_Header));
	if (!(has_dc_vms ^ has_dc_vmi)) {
		// Can't be none or both...
		return nullptr;
	}

	// Determine which file we should look for.
	IRpFile *vms_file;
	IRpFile *vmi_file;
	IRpFile **other_file;	// Points to vms_file or vmi_file.

	const char *rel_ext;
	if (has_dc_vms) {
		// We have the VMS file.
		// Find the VMI file.
		vms_file = file;
		vmi_file = nullptr;
		other_file = &vmi_file;
		rel_ext = ".VMI";
	} else /*if (has_dc_vmi)*/ {
		// We have the VMI file.
		// Find the VMS file.
		vms_file = nullptr;
		vmi_file = file;
		other_file = &vms_file;
		rel_ext = ".VMS";
	}

	// Attempt to open the other file in the pair.
	// TODO: Verify length.
	// TODO: For .vmi, check the VMS resource name?
	const string filename = file->filename();
	*other_file = FileSystem::openRelatedFile(filename.c_str(), nullptr, rel_ext);
	if (!*other_file) {
		// Can't open the other file.
		return nullptr;
	}

	// Attempt to create a DreamcastSave using both the
	// VMS and VMI files.
	DreamcastSave *const dcSave = new DreamcastSave(vms_file, vmi_file);
	(*other_file)->unref();	// Not needed anymore.
	if (!dcSave->isValid()) {
		// Not valid.
		dcSave->unref();
		return nullptr;
	}

	// DreamcastSave opened.
	return dcSave;
}

/**
 * Check an ISO-9660 disc image for a game-specific file system.
 *
 * If this is a valid ISO-9660 disc image, but no game-specific
 * RomData subclasses support it, an ISO object will be returned.
 *
 * @param file ISO-9660 disc image
 * @return Game-specific RomData subclass, or nullptr if none are supported.
 */
RomData *RomDataFactoryPrivate::checkISO(IRpFile *file)
{
	// Check for a CD file system with 2048-byte sectors.
	CDROM_2352_Sector_t sector;
	size_t size = file->seekAndRead(ISO_PVD_ADDRESS_2048, &sector.m1.data, sizeof(sector.m1.data));
	if (size != sizeof(sector.m1.data)) {
		// Unable to read the PVD.
		return nullptr;
	}

	bool is2048;
	const ISO_Primary_Volume_Descriptor *pvd = nullptr;
	int discType = ISO::checkPVD(sector.m1.data);
	if (discType >= 0) {
		// Found a PVD with 2048-byte sectors.
		pvd = reinterpret_cast<const ISO_Primary_Volume_Descriptor*>(sector.m1.data);
		is2048 = true;
	} else {
		// Check for a PVD with 2352-byte or 2448-byte sectors.
		static const unsigned int sector_sizes[] = {2352, 2448, 0};
		is2048 = false;

		for (const unsigned int *p = sector_sizes; *p != 0; p++) {
			size_t size = file->seekAndRead(*p * ISO_PVD_LBA, &sector, sizeof(sector));
			if (size != sizeof(sector)) {
				// Unable to read the PVD.
				return nullptr;
			}

			const uint8_t *const pData = cdromSectorDataPtr(&sector);
			if (ISO::checkPVD(pData) >= 0) {
				// Found the correct sector size.
				pvd = reinterpret_cast<const ISO_Primary_Volume_Descriptor*>(pData);
				break;
			}
		}
	}

	if (!pvd) {
		// Unable to get the PVD.
		return nullptr;
	}

	// Console/Handheld disc formats.
	typedef int (*pfnIsRomSupported_ISO_t)(const ISO_Primary_Volume_Descriptor *pvd);
	struct RomDataFns_ISO {
		pfnIsRomSupported_ISO_t isRomSupported;
		pfnNewRomData_t newRomData;
	};
#define GetRomDataFns_ISO(sys) \
	{sys::isRomSupported_static, \
	 RomDataFactoryPrivate::RomData_ctor<sys>}
	static const RomDataFns_ISO romDataFns_ISO[] = {
		GetRomDataFns_ISO(PlayStationDisc),
		GetRomDataFns_ISO(PSP),
		GetRomDataFns_ISO(XboxDisc),

		{nullptr, nullptr}
	};

	const RomDataFns_ISO *fns = &romDataFns_ISO[0];
	for (; fns->isRomSupported != nullptr; fns++) {
		if (fns->isRomSupported(pvd) >= 0) {
			// This might be the correct RomData subclass.
			RomData *const romData = fns->newRomData(file);
			if (romData->isValid()) {
				// Found the correct RomData subclass.
				return romData;
			}
			romData->unref();
		}
	}

	// Check for extracted XDVDFS. (2048-byte sector images only)
	if (is2048) {
		// Check for the magic number at the base offset.
		XDVDFS_Header xdvdfsHeader;
		size = file->seekAndRead(XDVDFS_HEADER_LBA_OFFSET * XDVDFS_BLOCK_SIZE,
			&xdvdfsHeader, sizeof(xdvdfsHeader));
		if (size == sizeof(xdvdfsHeader)) {
			// Check the magic numbers.
			if (!memcmp(xdvdfsHeader.magic, XDVDFS_MAGIC, sizeof(xdvdfsHeader.magic)) &&
			    !memcmp(xdvdfsHeader.magic_footer, XDVDFS_MAGIC, sizeof(xdvdfsHeader.magic_footer)))
			{
				// It's a match! Try opening as XboxDisc.
				RomData *const romData = new XboxDisc(file);
				if (romData->isValid()) {
					// Found the correct RomData subclass.
					return romData;
				}
				romData->unref();
			}
		}
	}

	// Not a game-specific file system.
	// Use the generic ISO-9660 parser.
	return new ISO(file);
}

/** RomDataFactory **/

/**
 * Create a RomData subclass for the specified ROM file.
 *
 * NOTE: RomData::isValid() is checked before returning a
 * created RomData instance, so returned objects can be
 * assumed to be valid as long as they aren't nullptr.
 *
 * If imgbf is non-zero, at least one of the specified image
 * types must be supported by the RomData subclass in order to
 * be returned.
 *
 * @param file ROM file.
 * @param attrs RomDataAttr bitfield. If set, RomData subclass must have the specified attributes.
 * @return RomData subclass, or nullptr if the ROM isn't supported.
 */
RomData *RomDataFactory::create(IRpFile *file, unsigned int attrs)
{
	RomData::DetectInfo info;

	// Get the file size.
	info.szFile = file->size();

	// Read 4,096+256 bytes from the ROM header.
	// This should be enough to detect most systems.
	union {
		uint8_t u8[4096+256];
		uint32_t u32[(4096+256)/4];
	} header;
	file->rewind();
	info.header.addr = 0;
	info.header.pData = header.u8;
	info.header.size = static_cast<uint32_t>(file->read(header.u8, sizeof(header.u8)));
	if (info.header.size == 0) {
		// Read error.
		return nullptr;
	}

	// File extension.
	string file_ext;	// temporary storage
	info.ext = nullptr;
	if (file->isDevice()) {
		// Device file. Assume it's a CD-ROM.
		info.ext = ".iso";
		// Subclass must support devices.
		attrs |= ATTR_SUPPORTS_DEVICES;
	} else {
		// Get the actual file extension.
		const string filename = file->filename();
		if (!filename.empty()) {
			const char *pExt = FileSystem::file_ext(filename);
			if (pExt) {
				file_ext = pExt;
				info.ext = file_ext.c_str();
			}
		}
	}

	// Special handling for Dreamcast .VMI+.VMS pairs.
	if (info.ext != nullptr &&
	    (!strcasecmp(info.ext, ".vms") ||
	     !strcasecmp(info.ext, ".vmi")))
	{
		// Dreamcast .VMI+.VMS pair.
		// Attempt to open the other file in the pair.
		RomData *romData = RomDataFactoryPrivate::openDreamcastVMSandVMI(file);
		if (romData) {
			if (romData->isValid()) {
				// .VMI+.VMS pair opened.
				return romData;
			}
			// Not a .VMI+.VMS pair.
			romData->unref();
		}

		// Not a .VMI+.VMS pair.
	}

	// Check RomData subclasses that take a header at 0
	// and definitely have a 32-bit magic number in the header.
	const RomDataFactoryPrivate::RomDataFns *fns =
		&RomDataFactoryPrivate::romDataFns_magic[0];
	for (; fns->romDataInfo != nullptr; fns++) {
		if ((fns->attrs & attrs) != attrs) {
			// This RomData subclass doesn't have the
			// required attributes.
			continue;
		}

		// Check the magic number.
		// TODO: Verify alignment restrictions.
		assert(fns->address % 4 == 0);
		assert(fns->address + sizeof(uint32_t) <= sizeof(header.u32));
		uint32_t magic = header.u32[fns->address/4];
		if (be32_to_cpu(magic) == fns->size) {
			// Found a matching magic number.
			if (fns->isRomSupported(&info) >= 0) {
				RomData *const romData = fns->newRomData(file);
				if (romData->isValid()) {
					// RomData subclass obtained.
					return romData;
				}

				// Not actually supported.
				romData->unref();
			}
		}
	}

	// Check for supported textures.
	{
		// TODO: RpTextureWrapper::isRomSupported()?
		RomData *const romData = new RpTextureWrapper(file);
		if (romData->isValid()) {
			// RomData subclass obtained.
			return romData;
		}

		// Not actually supported.
		romData->unref();
	}

	// Check other RomData subclasses that take a header,
	// but don't have a simple 32-bit magic number check.
	fns = &RomDataFactoryPrivate::romDataFns_header[0];
	bool checked_exts = false;
	for (; fns->romDataInfo != nullptr; fns++) {
		if ((fns->attrs & attrs) != attrs) {
			// This RomData subclass doesn't have the
			// required attributes.
			continue;
		}

		if (fns->address != info.header.addr ||
		    fns->size > info.header.size)
		{
			// Header address has changed.
			if (!checked_exts) {
				// Check the file extension to reduce overhead
				// for file types that don't use this.
				// TODO: Don't hard-code this.
				// Use a pointer to supportedFileExtensions_static() instead?
				static const char *const exts[] = {
					".bin",		// generic .bin
					".sms",		// Sega Master System
					".gg",		// Game Gear
					".tgc",		// game.com
					".iso",		// ISO-9660
					".img",		// CCD/IMG
					".xiso",	// Xbox disc image
					".min",		// Pokémon Mini
					nullptr
				};

				if (info.ext == nullptr) {
					// No file extension...
					break;
				}

				// Check for a matching extension.
				bool found = false;
				for (const char *const *ext = exts; *ext != nullptr; ext++) {
					if (!strcasecmp(info.ext, *ext)) {
						// Found a match!
						found = true;
					}
				}
				if (!found) {
					// No match.
					break;
				}

				// File extensions have been checked.
				checked_exts = true;
			}

			// Read the new header data.

			// NOTE: fns->size == 0 is only correct
			// for headers located at 0, since we
			// read the whole 4096+256 bytes for these.
			assert(fns->size != 0);
			assert(fns->size <= sizeof(header));
			if (fns->size == 0 || fns->size > sizeof(header))
				continue;

			// Make sure the file is big enough to
			// have this header.
			if ((static_cast<off64_t>(fns->address) + fns->size) > info.szFile)
				continue;

			// Read the header data.
			info.header.addr = fns->address;
			int ret = file->seek(info.header.addr);
			if (ret != 0)
				continue;
			info.header.size = static_cast<uint32_t>(file->read(header.u8, fns->size));
			if (info.header.size != fns->size)
				continue;
		}

		if (fns->isRomSupported(&info) >= 0) {
			RomData *romData;
			if (fns->attrs & RDA_CHECK_ISO) {
				// Check for a game-specific ISO subclass.
				romData = RomDataFactoryPrivate::checkISO(file);
			} else {
				// Standard RomData subclass.
				romData = fns->newRomData(file);
			}

			if (romData) {
				if (romData->isValid()) {
					// RomData subclass obtained.
					return romData;
				}
				// Not actually supported.
				romData->unref();
			}
		}
	}

	// Check RomData subclasses that take a footer.
	if (info.szFile > (1LL << 30)) {
		// No subclasses that expect footers support
		// files larger than 1 GB.
		return nullptr;
	}

	bool readFooter = false;
	fns = &RomDataFactoryPrivate::romDataFns_footer[0];
	for (; fns->romDataInfo != nullptr; fns++) {
		if ((fns->attrs & attrs) != attrs) {
			// This RomData subclass doesn't have the
			// required attributes.
			continue;
		}

		// Do we have a matching extension?
		// FIXME: Instead of hard-coded, check romDataInfo()->exts.
		static const char *const exts[] = {
			".vb",		// VirtualBoy
			".ws",		// WonderSwan
			".wsc",		// WonderSwan Color
			".pc2",		// Pocket Challenge v2 (WS-compatible)
			nullptr
		};

		if (info.ext == nullptr) {
			// No file extension...
			break;
		}

		// Check for a matching extension.
		bool found = false;
		for (const char *const *ext = exts; *ext != nullptr; ext++) {
			if (!strcasecmp(info.ext, *ext)) {
				// Found a match!
				found = true;
			}
		}
		if (!found) {
			// No match.
			break;
		}

		// Make sure we've read the footer.
		if (!readFooter) {
			static const int footer_size = 1024;
			if (info.szFile > footer_size) {
				info.header.addr = static_cast<uint32_t>(info.szFile - footer_size);
				info.header.size = static_cast<uint32_t>(file->seekAndRead(info.header.addr, header.u8, footer_size));
				if (info.header.size == 0) {
					// Seek and/or read error.
					return nullptr;
				}
			}
			readFooter = true;
		}

		if (fns->isRomSupported(&info) >= 0) {
			RomData *const romData = fns->newRomData(file);
			if (romData->isValid()) {
				// RomData subclass obtained.
				return romData;
			}

			// Not actually supported.
			romData->unref();
		}
	}

	// Not supported.
	return nullptr;
}

/**
 * Initialize the vector of supported file extensions.
 * Used for Win32 COM registration.
 *
 * Internal function; must be called using pthread_once().
 *
 * NOTE: The return value is a struct that includes a flag
 * indicating if the file type handler supports thumbnails.
 */
void RomDataFactoryPrivate::init_supportedFileExtensions(void)
{
	// In order to handle multiple RomData subclasses
	// that support the same extensions, we're using
	// an unordered_map<string, unsigned int>. If any of the
	// handlers for a given extension support thumbnails,
	// then the thumbnail handlers will be registered.
	//
	// The actual data is stored in the vector<ExtInfo>.
	unordered_map<string, unsigned int> map_exts;

	static const size_t reserve_size =
		(ARRAY_SIZE(romDataFns_magic) +
		 ARRAY_SIZE(romDataFns_header) +
		 ARRAY_SIZE(romDataFns_footer)) * 2;
	vec_exts.reserve(reserve_size);
#ifdef HAVE_UNORDERED_MAP_RESERVE
	map_exts.reserve(reserve_size);
#endif /* HAVE_UNORDERED_MAP_RESERVE */

	for (const RomDataFns *const *tblptr = &romDataFns_tbl[0];
	     *tblptr != nullptr; tblptr++)
	{
		const RomDataFns *fns = *tblptr;
		for (; fns->romDataInfo != nullptr; fns++) {
			const char *const *sys_exts = fns->romDataInfo()->exts;
			if (!sys_exts)
				continue;

			for (; *sys_exts != nullptr; sys_exts++) {
				auto iter = map_exts.find(*sys_exts);
				if (iter != map_exts.end()) {
					// We already had this extension.
					// Update its attributes.
					iter->second |= fns->attrs;
				} else {
					// First time encountering this extension.
					map_exts[*sys_exts] = fns->attrs;
					vec_exts.emplace_back(RomDataFactory::ExtInfo(
						*sys_exts, fns->attrs));
				}
			}
		}
	}

	// Get file extensions from FileFormatFactory.
	static const unsigned int FFF_ATTRS = ATTR_HAS_THUMBNAIL | ATTR_HAS_METADATA;
	vector<const char*> vec_exts_fileFormat = FileFormatFactory::supportedFileExtensions();
	std::for_each(vec_exts_fileFormat.cbegin(), vec_exts_fileFormat.cend(),
		[&map_exts](const char *ext) {
			auto iter = map_exts.find(ext);
			if (iter != map_exts.end()) {
				// We already had this extension.
				// Update its attributes.
				iter->second |= FFF_ATTRS;
			} else {
				// First time encountering this extension.
				map_exts[ext] = FFF_ATTRS;
				vec_exts.emplace_back(RomDataFactory::ExtInfo(ext, FFF_ATTRS));
			}
		}
	);

	// Make sure the vector's attributes fields are up to date.
	std::for_each(vec_exts.begin(), vec_exts.end(),
		[&map_exts](RomDataFactory::ExtInfo &extInfo) {
			extInfo.attrs = map_exts[extInfo.ext];
		}
	);
}

/**
 * Get all supported file extensions.
 * Used for Win32 COM registration.
 *
 * NOTE: The return value is a struct that includes a flag
 * indicating if the file type handler supports thumbnails
 * and/or may have "dangerous" permissions.
 *
 * @return All supported file extensions, including the leading dot.
 */
const vector<RomDataFactory::ExtInfo> &RomDataFactory::supportedFileExtensions(void)
{
	pthread_once(&RomDataFactoryPrivate::once_exts, RomDataFactoryPrivate::init_supportedFileExtensions);
	return RomDataFactoryPrivate::vec_exts;
}

/**
 * Initialize the vector of supported MIME types.
 * Used for KFileMetaData.
 *
 * Internal function; must be called using pthread_once().
 */
void RomDataFactoryPrivate::init_supportedMimeTypes(void)
{
	// TODO: Add generic types, e.g. application/octet-stream?

	// In order to handle multiple RomData subclasses
	// that support the same MIME types, we're using
	// an unordered_set<string>. The actual data
	// is stored in the vector<const char*>.
	unordered_set<string> set_mimeTypes;

	static const size_t reserve_size =
		(ARRAY_SIZE(romDataFns_magic) +
		 ARRAY_SIZE(romDataFns_header) +
		 ARRAY_SIZE(romDataFns_footer)) * 2;
	vec_mimeTypes.reserve(reserve_size);
#ifdef HAVE_UNORDERED_SET_RESERVE
	set_mimeTypes.reserve(reserve_size);
#endif /* HAVE_UNORDERED_SET_RESERVE */

	for (const RomDataFns *const *tblptr = &romDataFns_tbl[0];
	     *tblptr != nullptr; tblptr++)
	{
		const RomDataFns *fns = *tblptr;
		for (; fns->romDataInfo != nullptr; fns++) {
			const char *const *sys_mimeTypes = fns->romDataInfo()->mimeTypes;
			if (!sys_mimeTypes)
				continue;

			for (; *sys_mimeTypes != nullptr; sys_mimeTypes++) {
				auto iter = set_mimeTypes.find(*sys_mimeTypes);
				if (iter == set_mimeTypes.end()) {
					set_mimeTypes.insert(*sys_mimeTypes);
					vec_mimeTypes.emplace_back(*sys_mimeTypes);
				}
			}
		}
	}

	// Get MIME types from FileFormatFactory.
	vector<const char*> vec_mimeTypes_fileFormat = FileFormatFactory::supportedMimeTypes();
	std::for_each(vec_mimeTypes_fileFormat.cbegin(), vec_mimeTypes_fileFormat.cend(),
		[&set_mimeTypes](const char *mimeType) {
			auto iter = set_mimeTypes.find(mimeType);
			if (iter == set_mimeTypes.end()) {
				set_mimeTypes.emplace(mimeType);
				vec_mimeTypes.emplace_back(mimeType);
			}
		}
	);
}

/**
 * Get all supported MIME types.
 * Used for KFileMetaData.
 *
 * @return All supported MIME types.
 */
const vector<const char*> &RomDataFactory::supportedMimeTypes(void)
{
	pthread_once(&RomDataFactoryPrivate::once_mimeTypes, RomDataFactoryPrivate::init_supportedMimeTypes);
	return RomDataFactoryPrivate::vec_mimeTypes;
}

}
