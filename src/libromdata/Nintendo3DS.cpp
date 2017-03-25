/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS.hpp: Nintendo 3DS ROM reader.                               *
 * Handles CCI/3DS, CIA, and SMDH files.                                   *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "config.libromdata.h"

#include "Nintendo3DS.hpp"
#include "RomData_p.hpp"

#include "n3ds_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "file/FileSystem.hpp"

#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class Nintendo3DSPrivate : public RomDataPrivate
{
	public:
		Nintendo3DSPrivate(Nintendo3DS *q, IRpFile *file);
		virtual ~Nintendo3DSPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Nintendo3DSPrivate)

	public:
		// Internal images.
		// 0 == 24x24; 1 == 48x48
		rp_image *img_icon[2];

	public:
		// ROM type.
		enum RomType {
			ROM_TYPE_UNKNOWN = -1,	// Unknown ROM type.

			ROM_TYPE_SMDH    = 0,	// SMDH
			ROM_TYPE_3DSX    = 1,	// 3DSX (homebrew)
			ROM_TYPE_CCI     = 2,	// CCI/3DS (cartridge dump)
			ROM_TYPE_eMMC    = 3,	// eMMC dump
			ROM_TYPE_CIA     = 4,	// CIA
		};
		int romType;

		// What stuff do we have?
		enum HeadersPresent {
			HEADER_NONE	= 0,

			// The following headers are not exclusive,
			// so one or more can be present.
			HEADER_SMDH	= (1 << 0),
			HEADER_NCCH	= (1 << 1),

			// The following headers are mutually exclusive.
			HEADER_3DSX	= (1 << 2),
			HEADER_CIA	= (1 << 3),
			HEADER_NCSD	= (1 << 4),
		};
		uint32_t headers_loaded;	// HeadersPresent

		// Non-exclusive headers.
		// NOTE: These must be byteswapped on access.
		N3DS_SMDH_Header_t smdh_header;
		N3DS_NCCH_Header_NoSig_t ncch_header;	// Primary NCCH, i.e. for the game.

		// Mutually-exclusive headers.
		// NOTE: These must be byteswapped on access.
		union {
			N3DS_3DSX_Header_t hb3dsx_header;
			N3DS_CIA_Header_t cia_header;
			N3DS_NCSD_Header_t ncsd_header;
		} mxh;

		/**
		 * Round a value to the next highest multiple of 64.
		 * @param value Value.
		 * @return Next highest multiple of 64.
		 */
		template<typename T>
		static inline T toNext64(T val)
		{
			return (val + (T)63) & ~((T)63);
		}

		/**
		 * Load the SMDH header.
		 * @return 0 on success; non-zero on error.
		 */
		int loadSMDH(void);

		/**
		 * Load the specified NCCH header.
		 * @param idx		[in] Content/partition index.
		 * @param pNcchHeader	[out] Pointer to N3DS_NCCH_Header_NoSig_t.
		 * @return 0 on success; non-zero on error.
		 */
		int loadNCCH(int idx, N3DS_NCCH_Header_NoSig_t *pNcchHeader);

		/**
		 * Load the NCCH header for the primary content.
		 * The NCCH header is loaded into this->ncch_header.
		 * @return 0 on success; non-zero on error.
		 */
		int loadNCCH(void);

		// TODO: Add a function to get the update versions
		// from CCI images.

		/**
		 * Load the ROM image's icon.
		 * @param idx Image index. (0 == 24x24; 1 == 48x48)
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(int idx = 1);

		/**
		 * Convert a Nintendo 3DS region value to a GameTDB region code.
		 * @param smdhRegion Nintendo 3DS region. (from SMDH)
		 * @param idRegion Game ID region.
		 *
		 * NOTE: Mulitple GameTDB region codes may be returned, including:
		 * - User-specified fallback region. [TODO]
		 * - General fallback region.
		 *
		 * @return GameTDB region code(s), or empty vector if the region value is invalid.
		 */
		static vector<const char*> n3dsRegionToGameTDB(
			uint32_t smdhRegion, char idRegion);
};

/** Nintendo3DSPrivate **/

Nintendo3DSPrivate::Nintendo3DSPrivate(Nintendo3DS *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_TYPE_UNKNOWN)
	, headers_loaded(0)
{
	// Clear img_icon.
	img_icon[0] = nullptr;
	img_icon[1] = nullptr;

	// Clear the various headers.
	memset(&smdh_header, 0, sizeof(smdh_header));
	memset(&ncch_header, 0, sizeof(ncch_header));
	memset(&mxh, 0, sizeof(mxh));
}

Nintendo3DSPrivate::~Nintendo3DSPrivate()
{
	delete img_icon[0];
	delete img_icon[1];
}

/**
 * Load the SMDH header.
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::loadSMDH(void)
{
	if (headers_loaded & HEADER_SMDH) {
		// SMDH header is already loaded.
		return 0;
	}

	switch (romType) {
		case ROM_TYPE_SMDH: {
			// SMDH header is at the beginning of the file.
			file->rewind();
			size_t size = file->read(&smdh_header, sizeof(smdh_header));
			if (size != sizeof(smdh_header)) {
				// Error reading the SMDH header.
				return -1;
			}

			// SMDH header has been read.
			break;
		}

		case ROM_TYPE_3DSX: {
			// 3DSX file. SMDH is included only if we have
			// an extended header.
			// NOTE: 3DSX header should have been loaded by the constructor.
			if (!(headers_loaded & HEADER_3DSX)) {
				// 3DSX header wasn't loaded...
				return -2;
			}
			if (le32_to_cpu(mxh.hb3dsx_header.header_size) <= N3DS_3DSX_STANDARD_HEADER_SIZE) {
				// No extended header.
				return -3;
			}

			// Read the SMDH header.
			int ret = file->seek(le32_to_cpu(mxh.hb3dsx_header.smdh_offset));
			if (ret != 0) {
				// Seek error.
				return -4;
			}

			// Read the SMDH header.
			size_t size = file->read(&smdh_header, sizeof(smdh_header));
			if (size != sizeof(smdh_header)) {
				// Error reading the SMDH header.
				return -5;
			}

			// SMDH header has been read.
			break;
		}

		case ROM_TYPE_CIA: {
			// CIA file. SMDH may be located at the end
			// of the file in plaintext, or as part of
			// the executable in decrypted archives.

			// TODO: Handle decrypted archives.
			// TODO: If a CIA has an SMDH in the archive itself
			// and as a meta at the end of the file, which does
			// the FBI program prefer?

			// NOTE: CIA header should have been loaded by the constructor.
			if (!(headers_loaded & HEADER_CIA)) {
				// CIA header wasn't loaded...
				return -6;
			}

			// FBI's meta section is 15,040 bytes, but the SMDH header
			// and icon only take up 14,016 bytes.
			if (le32_to_cpu(mxh.cia_header.meta_size) < (uint32_t)(sizeof(smdh_header) + sizeof(N3DS_SMDH_Icon_t))) {
				// Meta section is either not present or too small.
				return -7;
			}

			// Determine the SMDH starting address.
			uint32_t addr = toNext64(le32_to_cpu(mxh.cia_header.header_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.cert_chain_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.ticket_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.tmd_size)) +
					toNext64(le32_to_cpu((uint32_t)mxh.cia_header.content_size)) +
					(uint32_t)sizeof(N3DS_CIA_Meta_Header_t);
			int ret = file->seek(addr);
			if (ret != 0) {
				// Seek failed.
				return -8;
			}

			// Read the SMDH header.
			size_t size = file->read(&smdh_header, sizeof(smdh_header));
			if (size != sizeof(smdh_header)) {
				// Error reading the SMDH header.
				return -5;
			}

			// SMDH header has been read.
			break;
		}

		case ROM_TYPE_CCI:
			// CCI file. SMDH can only be loaded if it's plaintext.
			// TODO: Find a plaintext CCI and/or a "null key" CCI.
			return -97;

		default:
			// Unsupported...
			return -98;
	}

	// Verify the SMDH magic number.
	if (memcmp(smdh_header.magic, N3DS_SMDH_HEADER_MAGIC, sizeof(smdh_header.magic)) != 0) {
		// SMDH magic number is incorrect.
		return -99;
	}
	// Loaded the SMDH header.
	headers_loaded |= HEADER_SMDH;
	return 0;
}

/**
 * Load the specified NCCH header.
 * @param idx		[in] Content/partition index.
 * @param pNcchHeader	[out] Pointer to N3DS_NCCH_Header_NoSig_t.
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::loadNCCH(int idx, N3DS_NCCH_Header_NoSig_t *pNcchHeader)
{
	assert(pNcchHeader != nullptr);
	if (!pNcchHeader) {
		return -100;
	}

	uint8_t media_unit_shift = 9;
	switch (romType) {
		case ROM_TYPE_CCI: {
			// The NCCH header is located at the beginning of the partition.
			// (Add 0x100 to skip the signature.)
			assert(idx >= 0 && idx < 8);
			if (idx < 0 || idx >= 8) {
				// Invalid partition index.
				return -1;
			}

			// Add the CCI media unit shift, if specified.
			media_unit_shift += mxh.ncsd_header.cci.partition_flags[NCSD_PARTITION_FLAG_MEDIA_UNIT_SIZE];

			// Get the partition offset.
			// TODO: Validate the partition length?
			const int64_t partition_offset = (int64_t)(le32_to_cpu(mxh.ncsd_header.partitions[idx].offset)) << media_unit_shift;
			// Make sure the partition starts after the card info header.
			if (partition_offset <= 0x2000) {
				// Invalid partition offset.
				return -2;
			}

			file->seek(partition_offset + 0x100);
			size_t size = file->read(pNcchHeader, sizeof(*pNcchHeader));
			if (size != sizeof(*pNcchHeader)) {
				// Error reading the NCCH header.
				return -3;
			}

			// NCCH header has been read.
			break;
		}

		default:
			// Unsupported...
			return -98;
	}

	// Verify the NCCH magic number.
	if (memcmp(pNcchHeader->magic, N3DS_NCCH_HEADER_MAGIC, sizeof(pNcchHeader->magic)) != 0) {
		// NCCH magic number is incorrect.
		return -99;
	}
	// Loaded the NCCH header.
	return 0;
}

/**
 * Load the NCCH header for the primary content.
 * The NCCH header is loaded into this->ncch_header.
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::loadNCCH(void)
{
	if (headers_loaded & HEADER_NCCH) {
		// NCCH header is already loaded.
		return 0;
	}

	// TODO: For CCIs, verify that the copy in the
	// Card Info Header matches the actual partition?
	int ret = loadNCCH(0, &ncch_header);
	if (ret == 0) {
		// NCCH header loaded.
		headers_loaded |= HEADER_NCCH;
	}
	return ret;
}

/**
 * Load the ROM image's icon.
 * @param idx Image index. (0 == 24x24; 1 == 48x48)
 * @return Icon, or nullptr on error.
 */
const rp_image *Nintendo3DSPrivate::loadIcon(int idx)
{
	assert(idx == 0 || idx == 1);
	if (idx != 0 && idx != 1) {
		// Invalid icon index.
		return nullptr;
	}

	if (img_icon[idx]) {
		// Icon has already been loaded.
		return img_icon[idx];
	} else if (!file || !isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Make sure the SMDH header is loaded.
	if (!(headers_loaded & HEADER_SMDH)) {
		// Load the SMDH header.
		if (loadSMDH() != 0) {
			// Error loading the SMDH header.
			return nullptr;
		}
	}

	// Since loadSMDH() succeeded, all required headers are present.

	// Load the SMDH icon data structure.
	// How to load this depends on the file type.
	// In all cases, the icon is located immediately
	// after the SMDH header.
	N3DS_SMDH_Icon_t smdh_icon;
	uint32_t smdh_icon_address = 0;	// If non-zero, load as plaintext here.
	switch (romType) {
		case ROM_TYPE_SMDH: {
			// SMDH file. Absolute addressing works absolutely.
			smdh_icon_address = (uint32_t)sizeof(smdh_header);
			break;
		}

		case ROM_TYPE_3DSX: {
			// SMDH icon is located past the SMDH header.
			smdh_icon_address = (uint32_t)(le32_to_cpu(mxh.hb3dsx_header.smdh_offset) + sizeof(smdh_header));
			break;
		}

		case ROM_TYPE_CIA: {
			// CIA file. SMDH may be located at the end
			// of the file in plaintext, or as part of
			// the executable in decrypted archives.

			// TODO: Handle decrypted archives.
			// TODO: If a CIA has an SMDH in the archive itself
			// and as a meta at the end of the file, which does
			// the FBI program prefer?

			// Determine the SMDH starting address.
			smdh_icon_address = toNext64(le32_to_cpu(mxh.cia_header.header_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.cert_chain_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.ticket_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.tmd_size)) +
					toNext64(le32_to_cpu((uint32_t)mxh.cia_header.content_size)) +
					(uint32_t)sizeof(N3DS_CIA_Meta_Header_t) +
					(uint32_t)sizeof(smdh_header);

			// FBI's meta section is 15,040 bytes, but the SMDH header
			// and icon only take up 14,016 bytes.
			if (le32_to_cpu(mxh.cia_header.meta_size) <
			    (uint32_t)(sizeof(smdh_header) + sizeof(smdh_icon)))
			{
				// Meta section is either not present or too small.
				return nullptr;
			}
			break;
		}

		case ROM_TYPE_CCI:
			// CCI file. SMDH can only be loaded if it's plaintext.
			// TODO: Find a plaintext CCI and/or a "null key" CCI.
			return nullptr;

		default:
			// Unsupported...
			return nullptr;
	}

	if (smdh_icon_address != 0) {
		// Load the SMDH icon at the specified address.
		int ret = file->seek(smdh_icon_address);
		if (ret != 0) {
			// Seek failed.
			return nullptr;
		}

		size_t size = file->read(&smdh_icon, sizeof(smdh_icon));
		if (size != sizeof(smdh_icon)) {
			// Read failed.
			return nullptr;
		}
	}

	// Convert the icon to rp_image.
	// NOTE: Assuming RGB565 format.
	// 3dbrew.org says it could be any of various formats,
	// but only RGB565 has been used so far.
	// Reference: https://www.3dbrew.org/wiki/SMDH#Icon_graphics
	switch (idx) {
		case 0:
			// Small icon. (24x24)
			// NOTE: Some older homebrew, including RxTools,
			// has a broken 24x24 icon.
			img_icon[0] = ImageDecoder::fromN3DSTiledRGB565(
				N3DS_SMDH_ICON_SMALL_W, N3DS_SMDH_ICON_SMALL_H,
				smdh_icon.small, sizeof(smdh_icon.small));
			break;
		case 1:
			// Large icon. (48x48)
			img_icon[1] = ImageDecoder::fromN3DSTiledRGB565(
				N3DS_SMDH_ICON_LARGE_W, N3DS_SMDH_ICON_LARGE_H,
				smdh_icon.large, sizeof(smdh_icon.large));
			break;
		default:
			// Invalid icon index.
			assert(!"Invalid 3DS icon index.");
			return nullptr;
	}

	return img_icon[idx];
}

/**
 * Convert a Nintendo 3DS region value to a GameTDB region code.
 * @param smdhRegion Nintendo 3DS region. (from SMDH)
 * @param idRegion Game ID region.
 *
 * NOTE: Mulitple GameTDB region codes may be returned, including:
 * - User-specified fallback region. [TODO]
 * - General fallback region.
 *
 * @return GameTDB region code(s), or empty vector if the region value is invalid.
 */
vector<const char*> Nintendo3DSPrivate::n3dsRegionToGameTDB(
	uint32_t smdhRegion, char idRegion)
{
	/**
	 * There are up to two region codes for Nintendo DS games:
	 * - Game ID
	 * - SMDH region (if the SMDH is readable)
	 *
	 * Some games are "technically" region-free, even though
	 * the cartridge is locked. These will need to use the
	 * host system region.
	 *
	 * The game ID will always be used as a fallback.
	 *
	 * Game ID reference:
	 * - https://github.com/dolphin-emu/dolphin/blob/4c9c4568460df91a38d40ac3071d7646230a8d0f/Source/Core/DiscIO/Enums.cpp
	 */
	vector<const char*> ret;

	int fallback_region = 0;
	switch (smdhRegion) {
		case N3DS_REGION_JAPAN:
			ret.push_back("JA");
			return ret;
		case N3DS_REGION_USA:
			ret.push_back("US");
			return ret;
		case N3DS_REGION_EUROPE:
		case N3DS_REGION_EUROPE | N3DS_REGION_AUSTRALIA:
			// Process the game ID and use "EN" as a fallback.
			fallback_region = 1;
			break;
		case N3DS_REGION_AUSTRALIA:
			// Process the game ID and use "AU","EN" as fallbacks.
			fallback_region = 2;
			break;
		case N3DS_REGION_CHINA:
			ret.push_back("ZHCN");
			ret.push_back("JA");
			ret.push_back("EN");
			return ret;
		case N3DS_REGION_SOUTH_KOREA:
			ret.push_back("KO");
			ret.push_back("JA");
			ret.push_back("EN");
			return ret;
		case N3DS_REGION_TAIWAN:
			ret.push_back("ZHTW");
			ret.push_back("JA");
			ret.push_back("EN");
			return ret;
		case 0:
		default:
			// No SMDH region, or unsupported SMDH region.
			break;
	}

	// TODO: If multiple SMDH region bits are set,
	// compare each to the host system region.

	// Check for region-specific game IDs.
	switch (idRegion) {
		case 'A':	// Region-free
			// Need to use the host system region.
			fallback_region = 3;
			break;
		case 'E':	// USA
			ret.push_back("US");
			break;
		case 'J':	// Japan
			ret.push_back("JA");
			break;
		case 'P':	// PAL
		case 'X':	// Multi-language release
		case 'Y':	// Multi-language release
		case 'L':	// Japanese import to PAL regions
		case 'M':	// Japanese import to PAL regions
		default:
			if (fallback_region == 0) {
				// Use the fallback region.
				fallback_region = 1;
			}
			break;

		// European regions.
		case 'D':	// Germany
			ret.push_back("DE");
			break;
		case 'F':	// France
			ret.push_back("FR");
			break;
		case 'H':	// Netherlands
			ret.push_back("NL");
			break;
		case 'I':	// Italy
			ret.push_back("NL");
			break;
		case 'R':	// Russia
			ret.push_back("RU");
			break;
		case 'S':	// Spain
			ret.push_back("ES");
			break;
		case 'U':	// Australia
			if (fallback_region == 0) {
				// Use the fallback region.
				fallback_region = 2;
			}
			break;
	}

	// Check for fallbacks.
	switch (fallback_region) {
		case 1:
			// Europe
			ret.push_back("EN");
			break;
		case 2:
			// Australia
			ret.push_back("AU");
			ret.push_back("EN");
			break;

		case 3:
			// TODO: Check the host system region.
			// For now, assuming US.
			ret.push_back("US");
			break;

		case 0:	// None
		default:
			break;
	}

	return ret;
}

/** Nintendo3DS **/

/**
 * Read a Nintendo 3DS ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
Nintendo3DS::Nintendo3DS(IRpFile *file)
	: super(new Nintendo3DSPrivate(this, file))
{
	// This class handles several different types of files,
	// so we'll initialize d->fileType later.
	RP_D(Nintendo3DS);
	d->fileType = FTYPE_UNKNOWN;
	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header.
	uint8_t header[0x2020];	// large enough for CIA headers
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = reinterpret_cast<const uint8_t*>(header);
	const rp_string ext = FileSystem::file_ext(file->filename());
	info.ext = ext.c_str();
	info.szFile = d->file->size();
	d->romType = isRomSupported_static(&info);

	// Determine what kind of file this is.
	// NOTE: SMDH header and icon will be loaded on demand.
	// and load the SMDH header and icon.
	switch (d->romType) {
		case Nintendo3DSPrivate::ROM_TYPE_SMDH:
			// SMDH header.
			if (info.szFile < (int64_t)(sizeof(N3DS_SMDH_Header_t) + sizeof(N3DS_SMDH_Icon_t))) {
				// File is too small.
				return;
			}
			d->fileType = FTYPE_ICON_FILE;
			// SMDH header is loaded by loadSMDH().
			break;

		case Nintendo3DSPrivate::ROM_TYPE_3DSX:
			// Save the 3DSX header for later.
			memcpy(&d->mxh.hb3dsx_header, header, sizeof(d->mxh.hb3dsx_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_3DSX;
			d->fileType = FTYPE_HOMEBREW;
			break;

		case Nintendo3DSPrivate::ROM_TYPE_CIA:
			// Save the CIA header for later.
			memcpy(&d->mxh.cia_header, header, sizeof(d->mxh.cia_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_CIA;
			d->fileType = FTYPE_APPLICATION_PACKAGE;
			break;

		case Nintendo3DSPrivate::ROM_TYPE_CCI:
			// Save the NCSD header for later.
			memcpy(&d->mxh.ncsd_header, header, sizeof(d->mxh.ncsd_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_NCSD;
			d->fileType = FTYPE_ROM_IMAGE;
			break;

		case Nintendo3DSPrivate::ROM_TYPE_eMMC:
			// Save the NCSD header for later.
			memcpy(&d->mxh.ncsd_header, header, sizeof(d->mxh.ncsd_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_NCSD;
			// TODO: eMMC image type?
			d->fileType = FTYPE_DISK_IMAGE;
			break;

		default:
			// Unknown ROM format.
			d->romType = Nintendo3DSPrivate::ROM_TYPE_UNKNOWN;
			return;
	}

	d->isValid = true;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Nintendo3DS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 512)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for CIA first. CIA doesn't have an unambiguous magic number,
	// so we'll use the file extension.
	// NOTE: The header data is usually smaller than 0x2020,
	// so only check the important contents.
	if (info->ext && info->header.size > offsetof(N3DS_CIA_Header_t, content_index) &&
	    !rp_strcasecmp(info->ext, _RP(".cia")))
	{
		// Verify the header parameters.
		const N3DS_CIA_Header_t *const cia_header =
			reinterpret_cast<const N3DS_CIA_Header_t*>(info->header.pData);
		if (le32_to_cpu(cia_header->header_size) == (uint32_t)sizeof(N3DS_CIA_Header_t) &&
		    le16_to_cpu(cia_header->type) == 0 &&
		    le16_to_cpu(cia_header->version) == 0)
		{
			// Add up all the sizes and see if it matches the file.
			// NOTE: We're only checking the minimum size in case
			// the file happens to be bigger.
			uint32_t sz_min = Nintendo3DSPrivate::toNext64(le32_to_cpu(cia_header->header_size)) +
					  Nintendo3DSPrivate::toNext64(le32_to_cpu(cia_header->cert_chain_size)) +
					  Nintendo3DSPrivate::toNext64(le32_to_cpu(cia_header->ticket_size)) +
					  Nintendo3DSPrivate::toNext64(le32_to_cpu(cia_header->tmd_size)) +
					  Nintendo3DSPrivate::toNext64(le32_to_cpu((uint32_t)cia_header->content_size)) +
					  Nintendo3DSPrivate::toNext64(le32_to_cpu(cia_header->meta_size));
			if (info->szFile >= (int64_t)sz_min) {
				// It's a match!
				return Nintendo3DSPrivate::ROM_TYPE_CIA;
			}
		}
	}

	// Check for SMDH.
	if (!memcmp(info->header.pData, N3DS_SMDH_HEADER_MAGIC, 4) &&
	    info->szFile >= (int64_t)(sizeof(N3DS_SMDH_Header_t) + sizeof(N3DS_SMDH_Icon_t)))
	{
		// We have an SMDH file.
		return Nintendo3DSPrivate::ROM_TYPE_SMDH;
	}

	// Check for 3DSX.
	if (!memcmp(info->header.pData, N3DS_3DSX_HEADER_MAGIC, 4) &&
	    info->szFile >= (int64_t)sizeof(N3DS_3DSX_Header_t))
	{
		// We have a 3DSX file.
		// NOTE: sizeof(N3DS_3DSX_Header_t) includes the
		// extended header, but that's fine, since a .3DSX
		// file with just the standard header and nothing
		// else is rather useless.
		return Nintendo3DSPrivate::ROM_TYPE_3DSX;
	}

	// Check for CCI/eMMC.
	const N3DS_NCSD_Header_t *const ncsd_header =
		reinterpret_cast<const N3DS_NCSD_Header_t*>(info->header.pData);
	if (!memcmp(ncsd_header->magic, N3DS_NCSD_HEADER_MAGIC, sizeof(ncsd_header->magic))) {
		// TODO: Validate the NCSD image size field?

		// Check if this is an eMMC dump or a CCI image.
		// This is done by checking the eMMC-specific crypt type section.
		// (All zeroes for CCI; minor variance between Old3DS and New3DS.)
		static const uint8_t crypt_cci[8]      = {0,0,0,0,0,0,0,0};
		static const uint8_t crypt_emmc_old[8] = {1,2,2,2,2,0,0,0};
		static const uint8_t crypt_emmc_new[8] = {1,2,2,2,3,0,0,0};
		if (!memcmp(ncsd_header->emmc_part_tbl.crypt_type, crypt_cci, sizeof(crypt_cci))) {
			// CCI image.
			return Nintendo3DSPrivate::ROM_TYPE_CCI;
		} else if (!memcmp(ncsd_header->emmc_part_tbl.crypt_type, crypt_emmc_old, sizeof(crypt_emmc_old)) ||
			   !memcmp(ncsd_header->emmc_part_tbl.crypt_type, crypt_emmc_new, sizeof(crypt_emmc_new))) {
			// eMMC dump.
			// NOTE: Not differentiating between Old3DS and New3DS here.
			return Nintendo3DSPrivate::ROM_TYPE_eMMC;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Nintendo3DS::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *Nintendo3DS::systemName(uint32_t type) const
{
	RP_D(const Nintendo3DS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	// TODO: *New* Nintendo 3DS for N3DS-exclusive titles.
	static const rp_char *const sysNames[4] = {
		_RP("Nintendo 3DS"), _RP("Nintendo 3DS"), _RP("3DS"), nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> Nintendo3DS::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".smdh"),	// SMDH (icon) file.
		_RP(".3dsx"),	// Homebrew application.
		_RP(".3ds"),	// ROM image. (NOTE: Conflicts with 3DS Max.)
		_RP(".cci"),	// ROM image.
		_RP(".cia"),	// CTR installable archive.
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> Nintendo3DS::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Nintendo3DS::supportedImageTypes_static(void)
{
#ifdef HAVE_JPEG
	return IMGBF_INT_ICON | IMGBF_EXT_BOX |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_FULL;
#else /* !HAVE_JPEG */
	return IMGBF_INT_ICON | IMGBF_EXT_BOX;
#endif /* HAVE_JPEG */
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Nintendo3DS::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> Nintendo3DS::supportedImageSizes_static(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return std::vector<ImageSizeDef>();
	}

	switch (imageType) {
		case IMG_INT_ICON: {
			static const ImageSizeDef sz_INT_ICON[] = {
				{nullptr, 24, 24, 0},
				{nullptr, 48, 48, 1},
			};
			return vector<ImageSizeDef>(sz_INT_ICON,
				sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
		}
		case IMG_EXT_COVER: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 160, 144, 0},
				//{"S", 128, 115, 1},	// Not currently present on GameTDB.
				{"M", 400, 352, 2},
				{"HQ", 768, 680, 3},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
		case IMG_EXT_COVER_FULL: {
			static const ImageSizeDef sz_EXT_COVER_FULL[] = {
				{nullptr, 340, 144, 0},
				//{"S", 272, 115, 1},	// Not currently present on GameTDB.
				{"M", 856, 352, 2},
				{"HQ", 1616, 680, 3},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER_FULL,
				sz_EXT_COVER_FULL + ARRAY_SIZE(sz_EXT_COVER_FULL));
		}
		case IMG_EXT_BOX: {
			static const ImageSizeDef sz_EXT_BOX[] = {
				{nullptr, 240, 216, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_BOX,
				sz_EXT_BOX + ARRAY_SIZE(sz_EXT_BOX));
		}
		default:
			break;
	}

	// Unsupported image type.
	return std::vector<ImageSizeDef>();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> Nintendo3DS::supportedImageSizes(ImageType imageType) const
{
	return supportedImageSizes_static(imageType);
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t Nintendo3DS::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	switch (imageType) {
		case IMG_INT_ICON:
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;
		default:
			break;
	}
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Nintendo3DS::loadFieldData(void)
{
	RP_D(Nintendo3DS);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM type.
		return -EIO;
	}

	d->fields->reserve(7); // Maximum of 7 fields.

	// Temporary buffer for snprintf().
	char buf[64];
	int len;

	// Load and parse the SMDH header.
	if (d->loadSMDH() == 0) {
		// SMDH header.
		// TODO: Get the system language.
		d->fields->addField_string(_RP("Title"), utf16le_to_rp_string(
			d->smdh_header.titles[1].desc_short, ARRAY_SIZE(d->smdh_header.titles[1].desc_short)));
		d->fields->addField_string(_RP("Full Title"), utf16le_to_rp_string(
			d->smdh_header.titles[1].desc_long, ARRAY_SIZE(d->smdh_header.titles[1].desc_long)));
		d->fields->addField_string(_RP("Publisher"), utf16le_to_rp_string(
			d->smdh_header.titles[1].publisher, ARRAY_SIZE(d->smdh_header.titles[1].publisher)));

		// Region code.
		// Maps directly to the SMDH field.
		static const rp_char *const n3ds_region_bitfield_names[] = {
			_RP("Japan"), _RP("USA"), _RP("Europe"),
			_RP("Australia"), _RP("China"), _RP("South Korea"),
			_RP("Taiwan")
		};
		vector<rp_string> *v_n3ds_region_bitfield_names = RomFields::strArrayToVector(
			n3ds_region_bitfield_names, ARRAY_SIZE(n3ds_region_bitfield_names));
		d->fields->addField_bitfield(_RP("Region Code"),
			v_n3ds_region_bitfield_names, 3, le32_to_cpu(d->smdh_header.settings.region_code));

		// Age rating(s).
		// Note that not all 16 fields are present on 3DS,
		// though the fields do match exactly, so no
		// mapping is necessary.
		RomFields::age_ratings_t age_ratings;
		// Valid ratings: 0-1, 3-4, 6-10
		static const uint16_t valid_ratings = 0x7DB;

		for (int i = (int)age_ratings.size()-1; i >= 0; i--) {
			if (!(valid_ratings & (1 << i))) {
				// Rating is not applicable for NintendoDS.
				age_ratings[i] = 0;
				continue;
			}

			// 3DS ratings field:
			// - 0x1F: Age rating.
			// - 0x20: No age restriction.
			// - 0x40: Rating pending.
			// - 0x80: Rating is valid if set.
			const uint8_t n3ds_rating = d->smdh_header.settings.ratings[i];
			if (!(n3ds_rating & 0x80)) {
				// Rating is unused.
				age_ratings[i] = 0;
			} else if (n3ds_rating & 0x40) {
				// Rating pending.
				age_ratings[i] = RomFields::AGEBF_ACTIVE | RomFields::AGEBF_PENDING;
			} else if (n3ds_rating & 0x20) {
				// No age restriction.
				age_ratings[i] = RomFields::AGEBF_ACTIVE | RomFields::AGEBF_NO_RESTRICTION;
			} else {
				// Set active | age value.
				age_ratings[i] = RomFields::AGEBF_ACTIVE | (n3ds_rating & 0x1F);
			}
		}
		d->fields->addField_ageRatings(_RP("Age Rating"), age_ratings);
	}

	// Is the NCSD header loaded?
	// TODO: Show before SMDH, and/or on a different subtab?
	if (d->headers_loaded & Nintendo3DSPrivate::HEADER_NCSD) {
		// Display the NCSD header.
		// TODO: Add more fields?
		const N3DS_NCSD_Header_t *const ncsd_header = &d->mxh.ncsd_header;

		// Is this eMMC?
		const bool emmc = (d->romType == Nintendo3DSPrivate::ROM_TYPE_eMMC);
		const bool new3ds = (ncsd_header->emmc_part_tbl.crypt_type[4] == 3);

		// Partition type names.
		static const rp_char *const partition_types[2][8] = {
			// CCI
			{_RP("Game"), _RP("Manual"), _RP("Download Play"),
			 nullptr, nullptr, nullptr,
			 _RP("New3DS Update"), _RP("Old3DS Update")},
			// eMMC
			{_RP("TWL NAND"), _RP("AGB SAVE"),
			 _RP("FIRM0"), _RP("FIRM1"), _RP("CTR NAND"),
			 nullptr, nullptr, nullptr},
		};

		// eMMC keyslots.
		static const uint8_t emmc_keyslots[2][8] = {
			// Old3DS keyslots.
			{0x03, 0x07, 0x06, 0x06, 0x04, 0x00, 0x00, 0x00},
			// New3DS keyslots.
			{0x03, 0x07, 0x06, 0x06, 0x05, 0x00, 0x00, 0x00},
		};

		const rp_char *const *pt_types;
		const uint8_t *keyslots = nullptr;
		vector<rp_string> *v_partitions_names;
		if (!emmc) {
			// CCI (3DS cartridge dump)

			// Media ID.
			const uint64_t media_id = le64_to_cpu(ncsd_header->media_id);
			len = snprintf(buf, sizeof(buf), "%08X %08X",
				(uint32_t)(media_id >> 32),
				(uint32_t)(media_id));
			if (len > (int)sizeof(buf))
				len = sizeof(buf);
			d->fields->addField_string(_RP("Media ID"),
				len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));

			// Partition type names.
			pt_types = partition_types[0];

			// Columns for the partition table.
			static const rp_char *const cci_partitions_names[] = {
				_RP("#"), _RP("Type"), _RP("Size")
			};
			v_partitions_names = RomFields::strArrayToVector(
				cci_partitions_names, ARRAY_SIZE(cci_partitions_names));
		} else {
			// eMMC (NAND dump)

			// eMMC type.
			d->fields->addField_string(_RP("Type"),
				(new3ds ? _RP("New3DS") : _RP("Old3DS / 2DS")));

			// Partition type names.
			// TODO: Show TWL NAND partitions?
			pt_types = partition_types[1];

			// Keyslots.
			keyslots = emmc_keyslots[new3ds];

			// Columns for the partition table.
			static const rp_char *const emmc_partitions_names[] = {
				_RP("#"), _RP("Type"), _RP("Keyslot"), _RP("Size")
			};
			v_partitions_names = RomFields::strArrayToVector(
				emmc_partitions_names, ARRAY_SIZE(emmc_partitions_names));
		}

		// Partition table.
		// TODO: Show the ListView on a separate row?
		auto partitions = new std::vector<std::vector<rp_string> >();
		partitions->reserve(8);

		// Media unit shift.
		// Default is 9, but may be increased by
		// ncsd_header->cci.partition_flags[8].
		// FIXME: Handle invalid shift values?
		uint8_t media_unit_shift = 9;
		if (d->romType == Nintendo3DSPrivate::ROM_TYPE_CCI) {
			// Add the CCI media unit shift, if specified.
			media_unit_shift += ncsd_header->cci.partition_flags[NCSD_PARTITION_FLAG_MEDIA_UNIT_SIZE];
		}

		// Process the partition table.
		for (unsigned int i = 0; i < 8; i++) {
			const uint32_t length = le32_to_cpu(ncsd_header->partitions[i].length);
			if (length == 0)
				continue;

			int vidx = (int)partitions->size();
			partitions->resize(vidx+1);
			auto &data_row = partitions->at(vidx);

			// Partition number.
			int len = snprintf(buf, sizeof(buf), "%u", i);
			if (len > (int)sizeof(buf))
				len = sizeof(buf);
			data_row.push_back(len > 0 ? latin1_to_rp_string(buf, len) : _RP("?"));

			// Partition type.
			const rp_char *type = (pt_types[i] ? pt_types[i] : _RP("Unknown"));
			data_row.push_back(type);

			if (keyslots) {
				// Keyslot.
				len = snprintf(buf, sizeof(buf), "0x%02X", keyslots[i]);
				if (len > (int)sizeof(buf))
					len = sizeof(buf);
				data_row.push_back(len > 0 ? latin1_to_rp_string(buf, len) : _RP("?"));
			}

			// Partition size.
			const int64_t length_bytes = (int64_t)length << media_unit_shift;
			data_row.push_back(d->formatFileSize(length_bytes));
		}

		// Add the partitions list data.
		d->fields->addField_listData(_RP("Partitions"), v_partitions_names, partitions);
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Nintendo3DS::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	assert(pImage != nullptr);
	if (!pImage) {
		// Invalid parameters.
		return -EINVAL;
	} else if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		*pImage = nullptr;
		return -ERANGE;
	}

	// NOTE: Assuming icon index 1. (48x48)
	const int idx = 1;

	RP_D(Nintendo3DS);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by 3DS.
		*pImage = nullptr;
		return -ENOENT;
	} else if (d->img_icon[idx]) {
		// Image has already been loaded.
		*pImage = d->img_icon[idx];
		return 0;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the icon.
	*pImage = d->loadIcon(idx);
	return (*pImage != nullptr ? 0 : -EIO);
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int Nintendo3DS::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	assert(pExtURLs != nullptr);
	if (!pExtURLs) {
		// No vector.
		return -EINVAL;
	}
	pExtURLs->clear();

	// Make sure the NCCH header is loaded.
	RP_D(const Nintendo3DS);
	if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_NCCH)) {
		// Load the NCCH header.
		if (const_cast<Nintendo3DSPrivate*>(d)->loadNCCH() != 0) {
			// Error loading the NCCH header.
			return -EIO;
		}
	}

	// Validate the program ID.
	// Reference: https://3dbrew.org/wiki/Titles
	const uint64_t program_id = le64_to_cpu(d->ncch_header.program_id);
	const uint32_t tid_hi = (uint32_t)(program_id >> 32);
	const uint32_t tid_lo = (uint32_t)program_id;
	if (tid_hi != 0x00040000 || tid_lo < 0x00030000 || tid_lo >= 0x0F800000) {
		// This is probably not a retail application
		// because one of the following conditions is met:
		// - TitleID High is not 0x00040000
		// - TitleID Low unique ID is  <   0x300 (system)
		// - TitleID Low unique ID is >= 0xF8000 (eval/proto/dev)
		return -ENOENT;
	}

	// Validate the product code.
	if (memcmp(d->ncch_header.product_code, "CTR-", 4) != 0 &&
	    memcmp(d->ncch_header.product_code, "KTR-", 4) != 0)
	{
		// Not a valid product code for GameTDB.
		return -ENOENT;
	}

	if (d->ncch_header.product_code[5] != '-' ||
	    d->ncch_header.product_code[10] != 0)
	{
		// Missing hyphen, or longer than 10 characters.
		return -ENOENT;
	}

	// Check the product type.
	// TODO: Enable demos, DLC, and updates?
	switch (d->ncch_header.product_code[4]) {
		case 'P':	// Game card
		case 'N':	// eShop
			// Product type is valid for GameTDB.
			break;
		case 'M':	// DLC
		case 'U':	// Update
		case 'T':	// Demo
		default:
			// Product type is NOT valid for GameTDB.
			return -ENOENT;
	}

	// Make sure the ID4 has only printable characters.
	const char *id4 = &d->ncch_header.product_code[6];
	for (int i = 3; i >= 0; i--) {
		if (!isprint(id4[i])) {
			// Non-printable character found.
			return -ENOENT;
		}
	}

	// Check for known unsupported game IDs.
	// TODO: Ignore eShop-only titles, or does GameTDB have those?
	if (!memcmp(id4, "CTAP", 4)) {
		// This is either a prototype, an update partition,
		// or some other non-retail title.
		// No external images are available.
		return -ENOENT;
	}

	if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *sizeDef = d->selectBestSize(sizeDefs, size);
	if (!sizeDef) {
		// No size available...
		return -ENOENT;
	}

	// NOTE: Only downloading the first size as per the
	// sort order, since GameTDB basically guarantees that
	// all supported sizes for an image type are available.
	// TODO: Add cache keys for other sizes in case they're
	// downloaded and none of these are available?

	// Determine the image type name.
	const char *imageTypeName_base;
	const char *ext;
	switch (imageType) {
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			imageTypeName_base = "cover";
			ext = ".jpg";
			break;
		case IMG_EXT_COVER_FULL:
			imageTypeName_base = "coverfull";
			ext = ".jpg";
			break;
#endif /* HAVE_JPEG */
		case IMG_EXT_BOX:
			imageTypeName_base = "box";
			ext = ".png";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Determine the GameTDB region code(s).
	const uint32_t smdhRegion =
		(d->headers_loaded & Nintendo3DSPrivate::HEADER_SMDH)
			? le32_to_cpu(d->smdh_header.settings.region_code)
			: 0;
	vector<const char*> tdb_regions = d->n3dsRegionToGameTDB(smdhRegion, id4[3]);

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	const ImageSizeDef *szdefs_dl[3];
	szdefs_dl[0] = sizeDef;
	unsigned int szdef_count;
	if (sizeDef->index >= 2) {
		// M or higher.
		szdefs_dl[1] = &sizeDefs[0];
		szdef_count = 2;
	} else {
		// Default or S.
		szdef_count = 1;
	}

	// Add the URLs.
	pExtURLs->reserve(4*szdef_count);
	for (unsigned int i = 0; i < szdef_count; i++) {
		// Current image type.
		char imageTypeName[16];
		snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
			 imageTypeName_base, (szdefs_dl[i]->name ? szdefs_dl[i]->name : ""));

		// Add the images.
		for (auto iter = tdb_regions.cbegin(); iter != tdb_regions.cend(); ++iter) {
			int idx = (int)pExtURLs->size();
			pExtURLs->resize(idx+1);
			auto &extURL = pExtURLs->at(idx);

			extURL.url = d->getURL_GameTDB("3ds", imageTypeName, *iter, id4, ext);
			extURL.cache_key = d->getCacheKey_GameTDB("3ds", imageTypeName, *iter, id4, ext);
			extURL.width = szdefs_dl[i]->width;
			extURL.height = szdefs_dl[i]->height;
			extURL.high_res = (szdefs_dl[i]->index >= 2);
		}
	}

	// All URLs added.
	return 0;
}

}
