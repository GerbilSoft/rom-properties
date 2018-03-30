/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS.hpp: Nintendo 3DS ROM reader.                               *
 * Handles CCI/3DS, CIA, and SMDH files.                                   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"

#include "Nintendo3DS.hpp"
#include "librpbase/RomData_p.hpp"

#include "n3ds_structs.h"
#include "data/NintendoLanguage.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/file/FileSystem.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// For DSiWare SRLs embedded in CIAs.
#include "librpbase/disc/DiscReader.hpp"
#include "librpbase/disc/PartitionFile.hpp"
#include "NintendoDS.hpp"

// NCCH and CIA readers.
#include "disc/NCCHReader.hpp"
#include "disc/CIAReader.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

// zlib for crc32()
#include <zlib.h>

namespace LibRomData {

ROMDATA_IMPL(Nintendo3DS)
ROMDATA_IMPL_IMG_SIZES(Nintendo3DS)

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

			ROM_TYPE_SMDH	= 0,	// SMDH
			ROM_TYPE_3DSX	= 1,	// 3DSX (homebrew)
			ROM_TYPE_CCI	= 2,	// CCI/3DS (cartridge dump)
			ROM_TYPE_eMMC	= 3,	// eMMC dump
			ROM_TYPE_CIA	= 4,	// CIA
			ROM_TYPE_NCCH	= 5,	// NCCH
		};
		int romType;

		// What stuff do we have?
		enum HeadersPresent {
			HEADER_NONE	= 0,

			// The following headers are not exclusive,
			// so one or more can be present.
			HEADER_SMDH	= (1 << 0),	// Includes header and icon.

			// The following headers are mutually exclusive.
			HEADER_3DSX	= (1 << 1),
			HEADER_CIA	= (1 << 2),
			HEADER_TMD	= (1 << 3),	// ticket, tmd_header
			HEADER_NCSD	= (1 << 4),	// ncsd_header, cinfo_header
		};
		uint32_t headers_loaded;	// HeadersPresent

		// Non-exclusive headers.
		// NOTE: These must be byteswapped on access.
		struct {
			N3DS_SMDH_Header_t header;
			N3DS_SMDH_Icon_t icon;
		} smdh;

		// Media unit shift.
		// This is usually 9 (512 bytes), though NCSD images
		// can have larger shifts.
		uint8_t media_unit_shift;

		// Mutually-exclusive headers.
		// NOTE: These must be byteswapped on access.
		union {
			N3DS_3DSX_Header_t hb3dsx_header;
			struct {
				N3DS_CIA_Header_t cia_header;
				N3DS_Ticket_t ticket;
				N3DS_TMD_Header_t tmd_header;
				// Content start address.
				uint32_t content_start_addr;
			};
			struct {
				N3DS_NCSD_Header_NoSig_t ncsd_header;
				N3DS_NCSD_Card_Info_Header_t cinfo_header;
			};
		} mxh;

		// Content chunk records. (CIA only)
		// Loaded by loadTicketAndTMD().
		unsigned int content_count;
		unique_ptr<N3DS_Content_Chunk_Record_t[]> content_chunks;

		// TODO: Move the pointers to the union?
		// That requires careful memory management...

	private:
		// Primary NCCH reader.
		// NOTE: Do NOT access this directly!
		// Use loadNCCH() instead.
		NCCHReader *ncch_reader;

	public:
		// File readers for DSiWare CIAs.
		IDiscReader *srlReader;	// uses this->file
		PartitionFile *srlFile;	// uses srlReader
		NintendoDS *srlData;	// NintendoDS object.

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
		 * Load the SMDH section.
		 * @return 0 on success; non-zero on error.
		 */
		int loadSMDH(void);

		/**
		 * Load the specified NCCH header.
		 * @param idx			[in] Content/partition index.
		 * @param pOutNcchReader	[out] Output variable for the NCCHReader.
		 * @return 0 on success; negative POSIX error code on error.
		 * NOTE: Caller must check NCCHReader::isOpen().
		 */
		int loadNCCH(int idx, NCCHReader **pOutNcchReader);

		/**
		 * Create an NCCHReader for the primary content.
		 * An NCCH reader is created as this->ncch_reader.
		 * @return this->ncch_reader on success; nullptr on error.
		 * NOTE: Caller must check NCCHReader::isOpen().
		 */
		NCCHReader *loadNCCH(void);

		/**
		 * Get the NCCH header from the primary content.
		 * This uses loadNCCH() to get the NCCH reader.
		 * @return NCCH header, or nullptr on error.
		 */
		const N3DS_NCCH_Header_NoSig_t *loadNCCHHeader(void);

		/**
		 * Load the ticket and TMD header. (CIA only)
		 * The ticket is loaded into mxh.ticket.
		 * The TMD header is loaded into mxh.tmd_header.
		 * @return 0 on success; non-zero on error.
		 */
		int loadTicketAndTMD(void);

		/**
		 * Load the ROM image's icon.
		 * @param idx Image index. (0 == 24x24; 1 == 48x48)
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(int idx = 1);

		/**
		 * Add the title ID, product code, and logo fields.
		 * Called by loadFieldData().
		 * @param showContentType If true, show the content type.
		 */
		void addTitleIdAndProductCodeFields(bool showContentType);

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

		/**
		 * Convert a Nintendo 3DS version number field to a string.
		 * @param version Version number.
		 * @return String.
		 */
		static inline string n3dsVersionToString(uint16_t version);

		/**
		 * Add the Permissions fields. (part of ExHeader)
		 * A separate tab should be created by the caller first.
		 * @param pNcchExHeader NCCH ExHeader.
		 */
		void addFields_permissions(const N3DS_NCCH_ExHeader_t *pNcchExHeader);
};

/** Nintendo3DSPrivate **/

Nintendo3DSPrivate::Nintendo3DSPrivate(Nintendo3DS *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_TYPE_UNKNOWN)
	, headers_loaded(0)
	, media_unit_shift(9)	// default is 9 (512 bytes)
	, content_count(0)
	, ncch_reader(nullptr)
	, srlReader(nullptr)
	, srlFile(nullptr)
	, srlData(nullptr)
{
	// Clear img_icon.
	img_icon[0] = nullptr;
	img_icon[1] = nullptr;

	// Clear the various headers.
	memset(&smdh, 0, sizeof(smdh));
	memset(&mxh, 0, sizeof(mxh));
}

Nintendo3DSPrivate::~Nintendo3DSPrivate()
{
	delete img_icon[0];
	delete img_icon[1];

	// NCCH reader.
	delete ncch_reader;

	// If this is a DSiWare SRL, these will be open.
	if (srlData) {
		srlData->unref();
	}
	delete srlFile;
	delete srlReader;
}

/**
 * Load the SMDH section.
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::loadSMDH(void)
{
	if (headers_loaded & HEADER_SMDH) {
		// SMDH section is already loaded.
		return 0;
	}

	switch (romType) {
		case ROM_TYPE_SMDH: {
			// SMDH section is the entire file.
			file->rewind();
			size_t size = file->read(&smdh, sizeof(smdh));
			if (size != sizeof(smdh)) {
				// Error reading the SMDH section.
				return -1;
			}

			// SMDH section has been read.
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

			// Read the SMDH section.
			size_t size = file->seekAndRead(le32_to_cpu(mxh.hb3dsx_header.smdh_offset), &smdh, sizeof(smdh));
			if (size != sizeof(smdh)) {
				// Seek and/or read error.
				return -4;
			}

			// SMDH section has been read.
			break;
		}

		case ROM_TYPE_CIA:
			// CIA file. SMDH may be located at the end
			// of the file in plaintext, or as part of
			// the executable in decrypted archives.

			// TODO: If a CIA has an SMDH in the archive itself
			// and as a meta at the end of the file, which does
			// the FBI program prefer?

			// NOTE: CIA header should have been loaded by the constructor.
			if (!(headers_loaded & HEADER_CIA)) {
				// CIA header wasn't loaded...
				return -5;
			}

			// Do we have a meta section?
			// FBI's meta section is 15,040 bytes, but the SMDH section
			// only takes up 14,016 bytes.
			if (le32_to_cpu(mxh.cia_header.meta_size) >= (uint32_t)sizeof(smdh)) {
				// Determine the SMDH starting address.
				uint32_t addr = toNext64(le32_to_cpu(mxh.cia_header.header_size)) +
						toNext64(le32_to_cpu(mxh.cia_header.cert_chain_size)) +
						toNext64(le32_to_cpu(mxh.cia_header.ticket_size)) +
						toNext64(le32_to_cpu(mxh.cia_header.tmd_size)) +
						toNext64(le32_to_cpu((uint32_t)mxh.cia_header.content_size)) +
						(uint32_t)sizeof(N3DS_CIA_Meta_Header_t);
				size_t size = file->seekAndRead(addr, &smdh, sizeof(smdh));
				if (size == sizeof(smdh)) {
					// SMDH section read.
					break;
				}
			}

			// Either there's no meta section, or the SMDH section
			// wasn't valid. Try loading from the ExeFS.
			// fall-through

		case ROM_TYPE_CCI:
		case ROM_TYPE_NCCH: {
			// CCI file, CIA file with no meta section, or NCCH file.
			// Open "exefs:/icon".
			NCCHReader *const ncch_reader = loadNCCH();
			if (!ncch_reader || !ncch_reader->isOpen()) {
				// Unable to open the primary NCCH.
				return -6;
			}

			unique_ptr<IRpFile> f_icon(ncch_reader->open(N3DS_NCCH_SECTION_EXEFS, "icon"));
			if (!f_icon) {
				// Failed to open "icon".
				return -7;
			} else if (f_icon->size() < (int64_t)sizeof(smdh)) {
				// Icon is too small.
				return -8;
			}

			// Load the SMDH section.
			size_t size = f_icon->read(&smdh, sizeof(smdh));
			if (size != sizeof(smdh)) {
				// Read error.
				return -9;
			}
			break;
		}

		default:
			// Unsupported...
			return -98;
	}

	// Verify the SMDH magic number.
	if (memcmp(smdh.header.magic, N3DS_SMDH_HEADER_MAGIC, sizeof(smdh.header.magic)) != 0) {
		// SMDH magic number is incorrect.
		return -99;
	}
	// Loaded the SMDH section.
	headers_loaded |= HEADER_SMDH;
	return 0;
}

/**
 * Load the specified NCCH header.
 * @param pOutNcchReader	[out] Output variable for the NCCHReader.
 * @return 0 on success; negative POSIX error code on error.
 * NOTE: Caller must check NCCHReader::isOpen().
 */
int Nintendo3DSPrivate::loadNCCH(int idx, NCCHReader **pOutNcchReader)
{
	assert(pOutNcchReader != nullptr);
	if (!pOutNcchReader)
		return -EINVAL;

	int64_t offset = 0;
	uint32_t length = 0;
	switch (romType) {
		case ROM_TYPE_CIA: {
			if (!(headers_loaded & HEADER_CIA)) {
				// CIA header is not loaded...
				return -EIO;
			}

			// Load the ticket and TMD header.
			if (loadTicketAndTMD() != 0) {
				// Unable to load the ticket and TMD header.
				return -EIO;
			}

			// Check if the content index is valid.
			if ((unsigned int)idx >= content_count) {
				// Content index is out of range.
				return -ENOENT;
			}

			// Determine the content start position.
			// Need to add all content chunk sizes, algined to 64 bytes.
			for (unsigned int i = 0; i < content_count; i++) {
				if (be16_to_cpu(content_chunks[i].index) == idx) {
					// Found the content chunk.
					length = (uint32_t)(be64_to_cpu(content_chunks[i].size));
					break;
				}
				// Next chunk.
				offset += toNext64(be64_to_cpu(content_chunks[i].size));
			}
			if (length == 0) {
				// Content chunk not found.
				return -ENOENT;
			}

			// Add the content start address.
			offset += mxh.content_start_addr;
			break;
		}

		case ROM_TYPE_CCI: {
			if (!(headers_loaded & HEADER_NCSD)) {
				// NCSD header is not loaded...
				return -EIO;
			}

			// The NCCH header is located at the beginning of the partition.
			// (Add 0x100 to skip the signature.)
			assert(idx >= 0 && idx < 8);
			if (idx < 0 || idx >= 8) {
				// Invalid partition index.
				return -ENOENT;
			}

			// Get the partition offset and length.
			offset = (int64_t)(le32_to_cpu(mxh.ncsd_header.partitions[idx].offset)) << media_unit_shift;
			length = le32_to_cpu(mxh.ncsd_header.partitions[idx].length) << media_unit_shift;
			// TODO: Validate length.
			// Make sure the partition starts after the card info header.
			if (offset <= 0x2000) {
				// Invalid partition offset.
				return -EIO;
			}
			break;
		}

		case ROM_TYPE_NCCH: {
			// NCCH file. Only one content.
			if (idx != 0) {
				// Invalid content index.
				return -ENOENT;
			}
			offset = 0;
			length = (uint32_t)file->size();
			break;
		}

		default:
			// Unsupported...
			return -ENOTSUP;
	}

	// Is this encrypted using CIA title key encryption?
	CIAReader *ciaReader = nullptr;
	if (romType == ROM_TYPE_CIA && idx < (int)content_count) {
		// Check if this content is encrypted.
		// If it is, we'll need to create a CIAReader.
		N3DS_Ticket_t *ticket = nullptr;
		const N3DS_Content_Chunk_Record_t *content_chunk = &content_chunks[0];
		for (unsigned int i = 0; i < content_count; i++, content_chunk++) {
			const uint16_t content_index = be16_to_cpu(content_chunk->index);
			if (content_index == idx) {
				// Found the content index.
				if (content_chunk->type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED)) {
					// Content is encrypted.
					ticket = &mxh.ticket;
				}
				break;
			}
		}

		if (ticket) {
			// Create a CIAReader.
			ciaReader = new CIAReader(file, offset, length, ticket, idx);
		}
	}

	// Create the NCCHReader.
	// NOTE: We're not checking isOpen() here.
	// That should be checked by the caller.
	if (ciaReader && ciaReader->isOpen()) {
		// This is an encrypted CIA.
		// NOTE: NCCHReader will take ownership of the CIAReader,
		// so we don't have to manage it ourselves.
		// NOTE 2: CIAReader handles the offset, so we need to
		// tell NCCHReader that the offset is 0.
		*pOutNcchReader = new NCCHReader(ciaReader, media_unit_shift, 0, length);
	} else {
		// Anything else is read directly.
		*pOutNcchReader = new NCCHReader(file, media_unit_shift, offset, length);
	}
	return 0;
}

/**
 * Create an NCCHReader for the primary content.
 * An NCCH reader is created as this->ncch_reader.
 * @return this->ncch_reader on success; nullptr on error.
 * NOTE: Caller must check NCCHReader::isOpen().
 */
NCCHReader *Nintendo3DSPrivate::loadNCCH(void)
{
	if (this->ncch_reader) {
		// NCCH reader has already been created.
		return this->ncch_reader;
	}

	unsigned int content_idx = 0;
	if (romType == ROM_TYPE_CIA) {
		// Use the boot content index.
		if ((headers_loaded & Nintendo3DSPrivate::HEADER_TMD) || loadTicketAndTMD() == 0) {
			content_idx = be16_to_cpu(mxh.tmd_header.boot_content);
		}
	}

	// TODO: For CCIs, verify that the copy in the
	// Card Info Header matches the actual partition?
	// NOTE: We're not checking isOpen() here.
	// That should be checked by the caller.
	loadNCCH(content_idx, &this->ncch_reader);
	return this->ncch_reader;
}

/**
 * Get the NCCH header from the primary content.
 * This uses loadNCCH() to get the NCCH reader.
 * @return NCCH header, or nullptr on error.
 */
inline const N3DS_NCCH_Header_NoSig_t *Nintendo3DSPrivate::loadNCCHHeader(void)
{
	const NCCHReader *const ncch = loadNCCH();
	return (ncch && ncch->isOpen() ? ncch->ncchHeader() : nullptr);
}

/**
 * Load the ticket and TMD header. (CIA only)
 * The ticket is loaded into mxh.ticket.
 * The TMD header is loaded into mxh.tmd_header.
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::loadTicketAndTMD(void)
{
	if (headers_loaded & HEADER_TMD) {
		// Ticket and TMD header are already loaded.
		return 0;
	} else if (romType != ROM_TYPE_CIA) {
		// Ticket and TMD are only available in CIA files.
		return -1;
	}

	/** Read the ticket. **/

	// Determine the ticket starting address
	// and read the signature type.
	const uint32_t ticket_start = toNext64(le32_to_cpu(mxh.cia_header.header_size)) +
			toNext64(le32_to_cpu(mxh.cia_header.cert_chain_size));
	uint32_t addr = ticket_start;
	uint32_t signature_type;
	size_t size = file->seekAndRead(addr, &signature_type, sizeof(signature_type));
	if (size != sizeof(signature_type)) {
		// Seek and/or read error.
		return -2;
	}
	signature_type = be32_to_cpu(signature_type);

	// Verify the signature type.
	if ((signature_type & 0xFFFFFFF8) != 0x00010000) {
		// Invalid signature type.
		return -3;
	}

	// Skip over the signature and padding.
	static const unsigned int sig_len_tbl[8] = {
		0x200 + 0x3C,	// N3DS_SIGTYPE_RSA_4096_SHA1
		0x100 + 0x3C,	// N3DS_SIGTYPE_RSA_2048_SHA1,
		0x3C  + 0x40,	// N3DS_SIGTYPE_EC_SHA1

		0x200 + 0x3C,	// N3DS_SIGTYPE_RSA_4096_SHA256
		0x100 + 0x3C,	// N3DS_SIGTYPE_RSA_2048_SHA256,
		0x3C  + 0x40,	// N3DS_SIGTYPE_ECDSA_SHA256

		0,		// invalid
		0,		// invalid
	};

	uint32_t sig_len = sig_len_tbl[signature_type & 0x07];
	if (sig_len == 0) {
		// Invalid signature type.
		return -3;
	}

	// Make sure the ticket is large enough.
	const uint32_t ticket_size = le32_to_cpu(mxh.cia_header.ticket_size);
	if (ticket_size < (sizeof(N3DS_Ticket_t) + sig_len)) {
		// Ticket is too small.
		return -4;
	}

	// Read the ticket.
	addr += sizeof(signature_type) + sig_len;
	size = file->seekAndRead(addr, &mxh.ticket, sizeof(mxh.ticket));
	if (size != sizeof(mxh.ticket)) {
		// Seek and/or read error.
		return -5;
	}

	/** Read the TMD. **/

	// Determine the TMD starting address.
	const uint32_t tmd_start = ticket_start +
			toNext64(le32_to_cpu(mxh.cia_header.ticket_size));
	addr = tmd_start;
	size = file->seekAndRead(addr, &signature_type, sizeof(signature_type));
	if (size != sizeof(signature_type)) {
		// Seek and/or read error.
		return -6;
	}
	signature_type = be32_to_cpu(signature_type);

	// Verify the signature type.
	if ((signature_type & 0xFFFFFFF8) != 0x00010000) {
		// Invalid signature type.
		return -7;
	}

	// Skip over the signature and padding.
	sig_len = sig_len_tbl[signature_type & 0x07];
	if (sig_len == 0) {
		// Invalid signature type.
		return -7;
	}

	// Make sure the TMD is large enough.
	const uint32_t tmd_size = le32_to_cpu(mxh.cia_header.tmd_size);
	if (tmd_size < (sizeof(N3DS_TMD_t) + sig_len)) {
		// TMD is too small.
		return -8;
	}

	// Read the TMD.
	addr += sizeof(signature_type) + sig_len;
	size = file->seekAndRead(addr, &mxh.tmd_header, sizeof(mxh.tmd_header));
	if (size != sizeof(mxh.tmd_header)) {
		// Seek and/or read error.
		return -9;
	}

	// Load the content chunk records.
	content_count = be16_to_cpu(mxh.tmd_header.content_count);
	if (content_count > 255) {
		// TODO: Do any titles have more than 255 contents?
		// Restricting to 255 maximum for now.
		content_count = 255;
	}
	content_chunks.reset(new N3DS_Content_Chunk_Record_t[content_count]);
	const size_t content_chunks_size = content_count * sizeof(N3DS_Content_Chunk_Record_t);

	addr += sizeof(N3DS_TMD_t);
	size = file->seekAndRead(addr, content_chunks.get(), content_chunks_size);
	if (size != content_chunks_size) {
		// Seek and/or read error.
		content_count = 0;
		content_chunks.reset(nullptr);
		return -10;
	}

	// Store the content start address.
	mxh.content_start_addr = tmd_start + toNext64(tmd_size);

	// Check if the CIA is DSiWare.
	// NOTE: "WarioWare Touched!" has a manual, but no other
	// DSiWare titles that I've seen do.
	if (content_count <= 2 && !this->srlData) {
		const int64_t offset = mxh.content_start_addr;
		const uint32_t length = (uint32_t)be64_to_cpu(content_chunks[0].size);
		if (length >= 0x8000) {
			// Attempt to open the SRL as if it's a new file.
			// TODO: IRpFile implementation with offset/length, so we don't
			// have to use both DiscReader and PartitionFile.

			// Check if this content is encrypted.
			// If it is, we'll need to create a CIAReader.
			IDiscReader *srlReader = nullptr;
			if (content_chunks[0].type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED)) {
				// Content is encrypted.
				srlReader = new CIAReader(this->file, offset, length,
					&mxh.ticket, be16_to_cpu(content_chunks[0].index));
			} else {
				// Content is NOT encrypted.
				// Use a plain old DiscReader.
				srlReader = new DiscReader(this->file, offset, length);
			}

			// TODO: Make IDiscReader derive from IRpFile.
			// May need to add reference counting to IRpFile...
			PartitionFile *srlFile = nullptr;
			NintendoDS *srlData = nullptr;
			if (srlReader->isOpen()) {
				srlFile = new PartitionFile(srlReader, 0, length);
				if (srlFile->isOpen()) {
					// Create the NintendoDS object.
					srlData = new NintendoDS(srlFile, true);
				}
			}

			if (srlData && srlData->isOpen() && srlData->isValid()) {
				// SRL opened successfully.
				this->srlReader = srlReader;
				this->srlFile = srlFile;
				this->srlData = srlData;
			} else {
				// Failed to open the SRL.
				if (srlData) {
					srlData->unref();
				}
				delete srlFile;
				delete srlReader;
			}
		}
	}

	// Loaded the TMD header.
	headers_loaded |= HEADER_TMD;
	return 0;
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

	// Make sure the SMDH section is loaded.
	if (!(headers_loaded & HEADER_SMDH)) {
		// Load the SMDH section.
		if (loadSMDH() != 0) {
			// Error loading the SMDH section.
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
				smdh.icon.small, sizeof(smdh.icon.small));
			break;
		case 1:
			// Large icon. (48x48)
			img_icon[1] = ImageDecoder::fromN3DSTiledRGB565(
				N3DS_SMDH_ICON_LARGE_W, N3DS_SMDH_ICON_LARGE_H,
				smdh.icon.large, sizeof(smdh.icon.large));
			break;
		default:
			// Invalid icon index.
			assert(!"Invalid 3DS icon index.");
			return nullptr;
	}

	return img_icon[idx];
}

/**
 * Add the title ID and product code fields.
 * Called by loadFieldData().
 * @param showContentType If true, show the content type.
 */
void Nintendo3DSPrivate::addTitleIdAndProductCodeFields(bool showContentType)
{
	// Title ID.
	// If using NCSD, use the Media ID.
	// If using CIA/TMD, use the TMD Title ID.
	// Otherwise, use the primary NCCH Title ID.

	// NCCH header.
	NCCHReader *const ncch = loadNCCH();
	const N3DS_NCCH_Header_NoSig_t *const ncch_header =
		(ncch && ncch->isOpen() ? ncch->ncchHeader() : nullptr);

	const char *tid_desc = nullptr;
	uint32_t tid_hi, tid_lo;
	if (romType == Nintendo3DSPrivate::ROM_TYPE_CCI &&
	    headers_loaded & Nintendo3DSPrivate::HEADER_NCSD)
	{
		tid_desc = "Media ID";
		tid_lo = le32_to_cpu(mxh.ncsd_header.media_id.lo);
		tid_hi = le32_to_cpu(mxh.ncsd_header.media_id.hi);
	} else if ((headers_loaded & Nintendo3DSPrivate::HEADER_TMD) || loadTicketAndTMD() == 0) {
		tid_desc = "Title ID";
		tid_hi = be32_to_cpu(mxh.tmd_header.title_id.hi);
		tid_lo = be32_to_cpu(mxh.tmd_header.title_id.lo);
	} else if (ncch_header) {
		tid_desc = "Title ID";
		tid_lo = le32_to_cpu(ncch_header->program_id.lo);
		tid_hi = le32_to_cpu(ncch_header->program_id.hi);
	}

	if (tid_desc) {
		fields->addField_string(tid_desc, rp_sprintf("%08X-%08X", tid_hi, tid_lo));
	}

	if (ncch && ncch->isOpen()) {
		// Product code.
		if (ncch_header) {
			fields->addField_string(C_("Nintendo3DS", "Product Code"),
				latin1_to_utf8(ncch_header->product_code, sizeof(ncch_header->product_code)));
		}

		// Content type.
		// This is normally shown in the CIA content table.
		if (showContentType) {
			const char *const content_type = ncch->contentType();
			// TODO: Remove context from "Unknown" and "Invalid" strings.
			fields->addField_string(C_("Nintendo3DS", "Content Type"),
				(content_type ? content_type : C_("Nintendo3DS", "Unknown")));
		}

		// Logo.
		// NOTE: All known official logo binaries are 8 KB.
		// The original and new "Homebrew" logos are also 8 KB.
		uint32_t crc = 0;
		unique_ptr<IRpFile> f_logo(ncch->openLogo());
		const int64_t szFile = (f_logo ? f_logo->size() : 0);
		if (szFile == 8192) {
			// Calculate the CRC32.
			unique_ptr<uint8_t[]> buf(new uint8_t[(unsigned int)szFile]);
			size_t size = f_logo->read(buf.get(), (unsigned int)szFile);
			if (size == (unsigned int)szFile) {
				crc = crc32(0, buf.get(), (unsigned int)szFile);
			}
		} else if (szFile > 0) {
			// Some other custom logo.
			crc = 1;
		}

		const char *logo_name;
		switch (crc) {
			case 0:
				// No logo.
				logo_name = nullptr;
				break;
			default:
				// Custom logo.
				logo_name = C_("Nintendo3DS|Logo", "Custom");
				break;

			// Official logos.
			// NOTE: Not translatable!
			case 0xCFD0EB8B:
				logo_name = "Nintendo";
				break;
			case 0x1093522B:
				logo_name = "Licensed by Nintendo";
				break;
			case 0x4FA8771C:
				logo_name = "Distributed by Nintendo";
				break;
			case 0x7F68B548:
				logo_name = "iQue";
				break;
			case 0xD8907ED7:
				logo_name = "iQue (System)";
				break;

			// Homebrew logos.
			case 0x343A79D9:
				// "Old" static Homebrew logo.
				// Included with `makerom`.
				logo_name = "Homebrew (static)";
				break;
			case 0xF257BD67:
				// "New" animated Homebrew logo.
				// Uses the Homebrew Launcher theme.
				// Reference: https://gbatemp.net/threads/release-default-homebrew-custom-logo-bin.457611/
				logo_name = "Homebrew (animated)";
				break;
		}

		if (logo_name) {
			fields->addField_string(C_("Nintendo3DS", "Logo"), logo_name);
		}
	}
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

/**
 * Convert a Nintendo 3DS version number field to a string.
 * @param version Version number.
 * @return String.
 */
inline string Nintendo3DSPrivate::n3dsVersionToString(uint16_t version)
{
	// Reference: https://3dbrew.org/wiki/Titles
	return rp_sprintf("%u.%u.%u",
		(version >> 10),
		(version >>  4) & 0x1F,
		(version & 0x0F));
}

/**
 * Add the Permissions fields. (part of ExHeader)
 * A separate tab should be created by the caller first.
 * @param pNcchExHeader NCCH ExHeader.
 */
void Nintendo3DSPrivate::addFields_permissions(const N3DS_NCCH_ExHeader_t *pNcchExHeader)
{
#ifdef _WIN32
	// Windows: 6 visible rows per RFT_LISTDATA.
	static const int rows_visible = 6;
#else
	// Linux: 4 visible rows per RFT_LISTDATA.
	static const int rows_visible = 4;
#endif

	// FS access.
	static const char *const perm_fs_access[] = {
		"CategorySysApplication",
		"CategoryHardwareCheck",
		"CategoryFileSystemTool",
		"Debug",
		"TwlCardBackup",
		"TwlNandData",
		"Boss",
		"DirectSdmc",
		"Core",
		"CtrNandRo",
		"CtrNandRw",
		"CtrNandRoWrite",
		"CategorySystemSettings",
		"Cardboard",
		"ExportImportIvs",
		"DirectSdmcWrite",
		"SwitchCleanup",
		"SaveDataMove",
		"Shop",
		"Shell",
		"CategoryHomeMenu",
		"SeedDB",
	};

	// Convert to vector<vector<string> > for RFT_LISTDATA.
	auto vv_fs = new std::vector<std::vector<string> >();
	vv_fs->resize(ARRAY_SIZE(perm_fs_access));
	for (int i = ARRAY_SIZE(perm_fs_access)-1; i >= 0; i--) {
		auto &data_row = vv_fs->at(i);
		data_row.push_back(perm_fs_access[i]);
	}

	fields->addField_listData(C_("Nintendo3DS", "FS Access"), nullptr, vv_fs,
		rows_visible, RomFields::RFT_LISTDATA_CHECKBOXES,
		(uint32_t)le64_to_cpu(pNcchExHeader->aci.arm11_local.storage.fs_access));

	// ARM9 access.
	static const char *const perm_arm9_access[] = {
		"FsMountNand",
		"FsMountNandRoWrite",
		"FsMountTwln",
		"FsMountWnand",
		"FsMountCardSpi",
		"UseSdif3",
		"CreateSeed",
		"UseCardSpi",
		"SDApplication",
		"FsMountSdmcWrite",	// implied by DirectSdmc
	};

	// TODO: Other descriptor versions?
	// v2 is standard; may be v3 on 9.3.0-X.
	assert(pNcchExHeader->aci.arm9.descriptor_version == 2 ||
	       pNcchExHeader->aci.arm9.descriptor_version == 3);
	if (pNcchExHeader->aci.arm9.descriptor_version == 2 ||
	    pNcchExHeader->aci.arm9.descriptor_version == 3)
	{
		// Convert to vector<vector<string> > for RFT_LISTDATA.
		auto vv_arm9 = new std::vector<std::vector<string> >();
		vv_arm9->resize(ARRAY_SIZE(perm_arm9_access));
		for (int i = ARRAY_SIZE(perm_arm9_access)-1; i >= 0; i--) {
			auto &data_row = vv_arm9->at(i);
			data_row.push_back(perm_arm9_access[i]);
		}

		fields->addField_listData(C_("Nintendo3DS", "ARM9 Access"), nullptr, vv_arm9,
			rows_visible, RomFields::RFT_LISTDATA_CHECKBOXES,
			(uint32_t)le64_to_cpu(pNcchExHeader->aci.arm9.descriptors));
	}

	// Services. Each service is a maximum of 8 characters.
	// The field is NULL-padded, though if the service name
	// is 8 characters long, there won't be any NULLs.
	// TODO: How to determine 32 or 34? (descriptor version?)
	auto vv_svc = new std::vector<std::vector<string> >();
	vv_svc->reserve(ARRAY_SIZE(pNcchExHeader->aci.arm11_local.services));
	const char *svc = &pNcchExHeader->aci.arm11_local.services[0][0];
	for (int i = 0; i < ARRAY_SIZE(pNcchExHeader->aci.arm11_local.services); i++, svc += 8) {
		if (svc[0] == 0) {
			// End of service list.
			break;
		}

		// Add the service.
		// TODO: Service descriptions?
		vv_svc->resize(vv_svc->size()+1);
		auto &data_row = vv_svc->at(vv_svc->size()-1);
		data_row.push_back(latin1_to_utf8(svc, 8));
	}

	if (likely(!vv_svc->empty())) {
		fields->addField_listData(C_("Nintendo3DS", "Services"), nullptr, vv_svc, rows_visible, 0);
	} else {
		// No services.
		delete vv_svc;
	}
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
	d->className = "Nintendo3DS";
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
	const string filename = file->filename();
	info.ext = FileSystem::file_ext(filename);
	info.szFile = d->file->size();
	d->romType = isRomSupported_static(&info);

	// Determine what kind of file this is.
	// NOTE: SMDH header and icon will be loaded on demand.
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
			// Save the NCSD and Card Info headers for later.
			memcpy(&d->mxh.ncsd_header, &header[N3DS_NCSD_NOSIG_HEADER_ADDRESS], sizeof(d->mxh.ncsd_header));
			memcpy(&d->mxh.cinfo_header, &header[N3DS_NCSD_CARD_INFO_HEADER_ADDRESS], sizeof(d->mxh.cinfo_header));

			// NCSD may have a larger media unit shift.
			// FIXME: Handle invalid shift values?
			d->media_unit_shift = 9 + d->mxh.ncsd_header.cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_UNIT_SIZE];

			d->headers_loaded |= Nintendo3DSPrivate::HEADER_NCSD;
			d->fileType = FTYPE_ROM_IMAGE;
			break;

		case Nintendo3DSPrivate::ROM_TYPE_eMMC:
			// Save the NCSD header for later.
			memcpy(&d->mxh.ncsd_header, &header[N3DS_NCSD_NOSIG_HEADER_ADDRESS], sizeof(d->mxh.ncsd_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_NCSD;
			d->fileType = FTYPE_EMMC_DUMP;
			break;

		case Nintendo3DSPrivate::ROM_TYPE_NCCH:
			// NCCH reader will be created when loadNCCH() is called.
			// TODO: Better type.
			d->fileType = FTYPE_TITLE_CONTENTS;
			break;

		default:
			// Unknown ROM format.
			d->romType = Nintendo3DSPrivate::ROM_TYPE_UNKNOWN;
			return;
	}

	d->isValid = true;
}

/**
 * Close the opened file.
 */
void Nintendo3DS::close(void)
{
	RP_D(Nintendo3DS);
	if (d->srlData) {
		// Close the SRL.
		d->srlData->close();
	}
	super::close();
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
	    !strcasecmp(info->ext, ".cia"))
	{
		// Verify the header parameters.
		const N3DS_CIA_Header_t *const cia_header =
			reinterpret_cast<const N3DS_CIA_Header_t*>(info->header.pData);
		if (cia_header->header_size == cpu_to_le32((uint32_t)sizeof(N3DS_CIA_Header_t)) &&
		    cia_header->type == cpu_to_le16(0) &&
		    cia_header->version == cpu_to_le16(0))
		{
			// Validate the various sizes.
			// TODO: Add some fuzz to some of these?
			if (cia_header->cert_chain_size == cpu_to_le32(N3DS_CERT_CHAIN_SIZE) &&
			    le32_to_cpu(cia_header->ticket_size) % 4 == 0 &&
			    (cia_header->ticket_size == cpu_to_le32(sizeof(N3DS_Ticket_t) + 0x4 + 0x200 + 0x3C) ||
			     cia_header->ticket_size == cpu_to_le32(sizeof(N3DS_Ticket_t) + 0x4 + 0x100 + 0x3C) ||
			     cia_header->ticket_size == cpu_to_le32(sizeof(N3DS_Ticket_t) + 0x4 +  0x3C + 0x40)) &&
			    le32_to_cpu(cia_header->tmd_size) % 4 == 0 &&
			    (le32_to_cpu(cia_header->tmd_size) >= cpu_to_le32(sizeof(N3DS_TMD_Header_t) + 0x4 + 0x3C + 0x40 + sizeof(N3DS_Content_Info_Record_t)*64 + sizeof(N3DS_Content_Chunk_Record_t)) &&
			     le32_to_cpu(cia_header->tmd_size) <= cpu_to_le32(sizeof(N3DS_TMD_Header_t) + 0x4 + 0x200 + 0x3C + sizeof(N3DS_Content_Info_Record_t)*64 + sizeof(N3DS_Content_Chunk_Record_t)*64)) &&
			    (cia_header->meta_size == cpu_to_le32(0) ||
			     (le32_to_cpu(cia_header->meta_size) % 4 == 0 &&
			      le32_to_cpu(cia_header->meta_size) >= (sizeof(N3DS_SMDH_Header_t) + sizeof(N3DS_SMDH_Icon_t)))))
			{
				// Sizes appear to be valid.
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
	const N3DS_NCSD_Header_NoSig_t *const ncsd_header =
		reinterpret_cast<const N3DS_NCSD_Header_NoSig_t*>(
			&info->header.pData[N3DS_NCSD_NOSIG_HEADER_ADDRESS]);
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

	// Check for NCCH.
	const N3DS_NCCH_Header_t *const ncch_header =
		reinterpret_cast<const N3DS_NCCH_Header_t*>(info->header.pData);
	if (!memcmp(ncch_header->hdr.magic, N3DS_NCCH_HEADER_MAGIC, sizeof(ncch_header->hdr.magic))) {
		// Found the NCCH magic.
		// TODO: Other checks?
		return Nintendo3DSPrivate::ROM_TYPE_NCCH;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Nintendo3DS::systemName(unsigned int type) const
{
	RP_D(const Nintendo3DS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	// TODO: *New* Nintendo 3DS for N3DS-exclusive titles.
	static const char *const sysNames[4] = {
		"Nintendo 3DS", "Nintendo 3DS", "3DS", nullptr
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Nintendo3DS::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".smdh",	// SMDH (icon) file.
		".3dsx",	// Homebrew application.
		".3ds",		// ROM image. (NOTE: Conflicts with 3DS Max.)
		".3dz",		// ROM image. (with private header for Gateway 3DS)
		".cci",		// ROM image.
		".cia",		// CTR installable archive.
		".ncch",	// NCCH file.
		".app",		// NCCH file. (NOTE: May conflict with others...)
		".cxi",		// CTR Executable Image (NCCH)
		".cfa",		// CTR File Archive (NCCH)
		".csu",		// CTR System Update (CCI)

		nullptr
	};
	return exts;
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
	RP_D(const Nintendo3DS);
	if (d->romType == Nintendo3DSPrivate::ROM_TYPE_CIA) {
		// TMD needs to be loaded so we can check if it's a DSiWare SRL.
		if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
			const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
		}
		if (d->srlData) {
			// This is a DSiWare SRL.
			// Get the image information from the underlying SRL.
			return d->srlData->supportedImageTypes();
		}
	}

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
#ifdef HAVE_JPEG
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
#endif /* HAVE_JPEG */
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

	RP_D(const Nintendo3DS);
	if (d->romType == Nintendo3DSPrivate::ROM_TYPE_CIA) {
		// TMD needs to be loaded so we can check if it's a DSiWare SRL.
		if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
			const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
		}
		if (d->srlData) {
			// This is a DSiWare SRL.
			// Get the image information from the underlying SRL.
			return d->srlData->imgpf(imageType);
		}
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

	// TODO: Disambiguate the various NCCHReader pointers.
	// TODO: Split up into smaller functions?
	const char *const s_unknown = C_("Nintendo3DS", "Unknown");

	// Maximum of 20 fields.
	// Tested with several CCI, CIA, and NCCH files.
	d->fields->reserve(20);

	// Reserve at least 2 tabs.
	d->fields->reserveTabs(2);

	// Have we shown a warning yet?
	bool shownWarning = false;

	// Load headers if we don't already have them.
	if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_SMDH)) {
		d->loadSMDH();
	}
	if ((d->romType == Nintendo3DSPrivate::ROM_TYPE_CIA) &&
	    !(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD))
	{
		d->loadTicketAndTMD();
	}

	// Get the primary NCCH.
	// If this fails, and the file type is NCSD or CIA,
	// it usually means there's a missing key.
	const NCCHReader *const ncch = d->loadNCCH();
	// Check for potential encryption key errors.
	if (d->romType == Nintendo3DSPrivate::ROM_TYPE_CCI ||
	    d->romType == Nintendo3DSPrivate::ROM_TYPE_CIA ||
	    d->romType == Nintendo3DSPrivate::ROM_TYPE_NCCH)
	{
		KeyManager::VerifyResult res = (ncch
			? ncch->verifyResult()
			: KeyManager::VERIFY_UNKNOWN);
		if (!d->srlData && res != KeyManager::VERIFY_OK) {
			// Missing encryption keys.
			if (!shownWarning) {
				const char *err = KeyManager::verifyResultToString(res);
				if (!err) {
					err = C_("Nintendo3DS", "Unknown error. (THIS IS A BUG!)");
				}
				d->fields->addField_string("Warning", err, RomFields::STRF_WARNING);
				shownWarning = true;
			}
		}
	}

	// Load and parse the SMDH header.
	bool haveSeparateSMDHTab = true;
	if (d->headers_loaded & Nintendo3DSPrivate::HEADER_SMDH) {
		d->fields->setTabName(0, "SMDH");
		// Will we end up having a separate SMDH tab?
		if (!(d->headers_loaded & (Nintendo3DSPrivate::HEADER_NCSD | Nintendo3DSPrivate::HEADER_TMD))) {
			// There will only be a single tab.
			// Add the title ID and product code fields here.
			// (Include the content type, if available.)
			haveSeparateSMDHTab = false;
			d->addTitleIdAndProductCodeFields(true);
		}

		// Get the system language.
		// TODO: Verify against the game's region code?
		int lang = NintendoLanguage::getN3DSLanguage();

		// Check that the field is valid.
		if (d->smdh.header.titles[lang].desc_short[0] == cpu_to_le16(0)) {
			// Not valid. Check English.
			if (d->smdh.header.titles[N3DS_LANG_ENGLISH].desc_short[0] != cpu_to_le16(0)) {
				// English is valid.
				lang = N3DS_LANG_ENGLISH;
			} else {
				// Not valid. Check Japanese.
				if (d->smdh.header.titles[N3DS_LANG_JAPANESE].desc_short[0] != cpu_to_le16(0)) {
					// Japanese is valid.
					lang = N3DS_LANG_JAPANESE;
				} else {
					// Not valid...
					// TODO: Check other languages?
					lang = -1;
				}
			}
		}

		if (lang >= 0 && lang < ARRAY_SIZE(d->smdh.header.titles)) {
			d->fields->addField_string(C_("Nintendo3DS", "Title"), utf16le_to_utf8(
				d->smdh.header.titles[1].desc_short, ARRAY_SIZE(d->smdh.header.titles[lang].desc_short)));
			d->fields->addField_string(C_("Nintendo3DS", "Full Title"), utf16le_to_utf8(
				d->smdh.header.titles[1].desc_long, ARRAY_SIZE(d->smdh.header.titles[lang].desc_long)));
			d->fields->addField_string(C_("Nintendo3DS", "Publisher"), utf16le_to_utf8(
				d->smdh.header.titles[1].publisher, ARRAY_SIZE(d->smdh.header.titles[lang].publisher)));
		}

		// Region code.
		// Maps directly to the SMDH field.
		static const char *const n3ds_region_bitfield_names[] = {
			NOP_C_("Region", "Japan"),
			NOP_C_("Region", "USA"),
			NOP_C_("Region", "Europe"),
			NOP_C_("Region", "Australia"),
			NOP_C_("Region", "China"),
			NOP_C_("Region", "South Korea"),
			NOP_C_("Region", "Taiwan"),
		};
		vector<string> *const v_n3ds_region_bitfield_names = RomFields::strArrayToVector_i18n(
			"Region", n3ds_region_bitfield_names, ARRAY_SIZE(n3ds_region_bitfield_names));
		d->fields->addField_bitfield(C_("Nintendo3DS", "Region Code"),
			v_n3ds_region_bitfield_names, 3, le32_to_cpu(d->smdh.header.settings.region_code));

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
			const uint8_t n3ds_rating = d->smdh.header.settings.ratings[i];
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
		d->fields->addField_ageRatings(C_("Nintendo3DS", "Age Rating"), age_ratings);
	} else if (d->srlData) {
		// DSiWare SRL.
		const RomFields *srl_fields = d->srlData->fields();
		if (srl_fields) {
			d->fields->setTabName(0, C_("Nintendo3DS", "DSiWare"));

			// Will we end up having a separate DSiWare tab?
			if (!(d->headers_loaded & (Nintendo3DSPrivate::HEADER_NCSD | Nintendo3DSPrivate::HEADER_TMD))) {
				// There will only be a single tab.
				// Add the title ID and product code fields here.
				// (Include the content type, if available.)
				haveSeparateSMDHTab = false;
				d->addTitleIdAndProductCodeFields(true);
			}

			// Add the DSiWare fields.
			d->fields->addFields_romFields(srl_fields, 0);
		}
	} else {
		// Single tab.
		// Add the title ID and product code fields here.
		// (Include the content type, if available.)
		haveSeparateSMDHTab = false;
		d->addTitleIdAndProductCodeFields(true);
	}

	// Is the NCSD header loaded?
	if (d->headers_loaded & Nintendo3DSPrivate::HEADER_NCSD) {
		// Display the NCSD header.
		if (haveSeparateSMDHTab) {
			d->fields->addTab("NCSD");
			// Add the title ID and product code fields here.
			// (Content type is listed in the NCSD partition table.)
			d->addTitleIdAndProductCodeFields(false);
		} else {
			d->fields->setTabName(0, "NCSD");
		}

		if (!ncch || ncch->verifyResult() != KeyManager::VERIFY_OK) {
			// Missing encryption keys.
			// TODO: This warning probably isn't needed,
			// since it's handled above...
			if (!shownWarning) {
				KeyManager::VerifyResult res = (ncch
					? ncch->verifyResult()
					: KeyManager::VERIFY_UNKNOWN);
				const char *err = KeyManager::verifyResultToString(res);
				if (!err) {
					err = C_("Nintendo3DS", "Unknown error. (THIS IS A BUG!)");
				}
				d->fields->addField_string("Warning", err, RomFields::STRF_WARNING);
				shownWarning = true;
			}
		}

		// TODO: Add more fields?
		const N3DS_NCSD_Header_NoSig_t *const ncsd_header = &d->mxh.ncsd_header;

		// Is this eMMC?
		const bool emmc = (d->romType == Nintendo3DSPrivate::ROM_TYPE_eMMC);
		const bool new3ds = (ncsd_header->emmc_part_tbl.crypt_type[4] == 3);

		// Partition type names.
		// TODO: Translate?
		static const char *const partition_types[2][8] = {
			// CCI
			{"Game", "Manual", "Download Play",
			 nullptr, nullptr, nullptr,
			 "N3DS Update", "O3DS Update"},
			// eMMC
			{"TWL NAND", "AGB SAVE",
			 "FIRM0", "FIRM1", "CTR NAND",
			 nullptr, nullptr, nullptr},
		};

		// eMMC keyslots.
		static const uint8_t emmc_keyslots[2][8] = {
			// Old3DS keyslots.
			{0x03, 0x07, 0x06, 0x06, 0x04, 0x00, 0x00, 0x00},
			// New3DS keyslots.
			{0x03, 0x07, 0x06, 0x06, 0x05, 0x00, 0x00, 0x00},
		};

		const char *const *pt_types;
		const uint8_t *keyslots = nullptr;
		vector<string> *v_partitions_names = new vector<string>();
		if (!emmc) {
			// CCI (3DS cartridge dump)

			// Partition type names.
			pt_types = partition_types[0];

			// Columns for the partition table.
			static const char *const cci_partitions_names[] = {
				NOP_C_("Nintendo3DS|CCI", "#"),
				NOP_C_("Nintendo3DS|CCI", "Type"),
				NOP_C_("Nintendo3DS|CCI", "Encryption"),
				NOP_C_("Nintendo3DS|CCI", "Version"),
				NOP_C_("Nintendo3DS|CCI", "Size"),
			};
			v_partitions_names = RomFields::strArrayToVector_i18n(
				"Nintendo3DS|CCI", cci_partitions_names, ARRAY_SIZE(cci_partitions_names));
		} else {
			// eMMC (NAND dump)

			// eMMC type.
			d->fields->addField_string(C_("Nintendo3DS|eMMC", "Type"),
				(new3ds ? "New3DS / New2DS" : "Old3DS / 2DS"));

			// Partition type names.
			// TODO: Show TWL NAND partitions?
			pt_types = partition_types[1];

			// Keyslots.
			keyslots = emmc_keyslots[new3ds];

			// Columns for the partition table.
			static const char *const emmc_partitions_names[] = {
				NOP_C_("Nintendo3DS|eMMC", "#"),
				NOP_C_("Nintendo3DS|eMMC", "Type"),
				NOP_C_("Nintendo3DS|eMMC", "Keyslot"),
				NOP_C_("Nintendo3DS|eMMC", "Size"),
			};
			v_partitions_names = RomFields::strArrayToVector_i18n(
				"Nintendo3DS|eMMC", emmc_partitions_names, ARRAY_SIZE(emmc_partitions_names));
		}

		if (d->romType == Nintendo3DSPrivate::ROM_TYPE_CCI) {
			// CCI-specific fields.
			const N3DS_NCSD_Card_Info_Header_t *const cinfo_header = &d->mxh.cinfo_header;

			// TODO: Check if platform != 1 on New3DS-only cartridges.

			// Card type.
			static const char *const media_type_tbl[4] = {
				"Inner Device",
				"Card1",
				"Card2",
				"Extended Device",
			};
			const uint8_t media_type = ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_TYPE_INDEX];
			if (media_type < ARRAY_SIZE(media_type_tbl)) {
				d->fields->addField_string(C_("Nintendo3DS", "Media Type"),
					media_type_tbl[media_type]);
			} else {
				d->fields->addField_string(C_("Nintendo3DS", "Media Type"),
					rp_sprintf(C_("Nintendo3DS", "Unknown (0x%02X)"), media_type));
			}

			if (ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_TYPE_INDEX] == N3DS_NCSD_MEDIA_TYPE_CARD2) {
				// Card2 writable address.
				d->fields->addField_string_numeric(C_("Nintendo3DS", "Card2 RW Address"),
					le32_to_cpu(cinfo_header->card2_writable_address),
					RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);
			}

			// Card device.
			// NOTE: Either the SDK3 or SDK2 field is set,
			// depending on how old the title is. Check the
			// SDK3 field first.
			uint8_t card_dev_id = ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK3];
			if (card_dev_id < N3DS_NCSD_CARD_DEVICE_MIN ||
			    card_dev_id > N3DS_NCSD_CARD_DEVICE_MAX)
			{
				// SDK3 field is invalid. Use SDK2.
				card_dev_id = ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK3];
			}

			static const char *const card_dev_tbl[4] = {
				nullptr,
				NOP_C_("Nintendo3DS|CDev", "NOR Flash"),
				NOP_C_("Nintendo3DS|CDev", "None"),
				NOP_C_("Nintendo3DS|CDev", "Bluetooth"),
			};
			if (card_dev_id >= 1 && card_dev_id < ARRAY_SIZE(card_dev_tbl)) {
				d->fields->addField_string(C_("Nintendo3DS", "Card Device"),
					dpgettext_expr(RP_I18N_DOMAIN, "Nintendo3DS|CDev", card_dev_tbl[card_dev_id]));
			} else {
				d->fields->addField_string(C_("Nintendo3DS", "Card Device"),
					rp_sprintf_p(C_("Nintendo3DS|CDev", "Unknown (SDK2=0x%1$02X, SDK3=0x%2$02X)"),
						ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK2],
						ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK3]));
			}

			// Card revision.
			d->fields->addField_string_numeric(C_("Nintendo3DS", "Card Revision"),
				le32_to_cpu(cinfo_header->card_revision),
				RomFields::FB_DEC, 2);

			// TODO: Show "title version"?
		}

		// Partition table.
		// TODO: Show the ListView on a separate row?
		auto partitions = new std::vector<std::vector<string> >();
		partitions->reserve(8);

		// Process the partition table.
		for (unsigned int i = 0; i < 8; i++) {
			const uint32_t length = le32_to_cpu(ncsd_header->partitions[i].length);
			if (length == 0)
				continue;

			// Make sure the partition exists first.
			NCCHReader *pNcch = nullptr;
			int ret = d->loadNCCH(i, &pNcch);
			if (ret == -ENOENT)
				continue;

			const int vidx = (int)partitions->size();
			partitions->resize(vidx+1);
			auto &data_row = partitions->at(vidx);

			// Partition number.
			data_row.push_back(rp_sprintf("%u", i));

			// Partition type.
			// TODO: Use the partition ID to determine the type?
			const char *type = (pt_types[i] ? pt_types[i] : s_unknown);
			data_row.push_back(type);

			if (!emmc) {
				const N3DS_NCCH_Header_NoSig_t *const part_ncch_header =
					(pNcch && pNcch->isOpen() ? pNcch->ncchHeader() : nullptr);
				if (part_ncch_header) {
					// Encryption.
					NCCHReader::CryptoType cryptoType = {nullptr, false, 0, false};
					int ret = NCCHReader::cryptoType_static(&cryptoType, part_ncch_header);
					if (ret != 0 || !cryptoType.encrypted || cryptoType.keyslot >= 0x40) {
						// Not encrypted, or not using a predefined keyslot.
						if (cryptoType.name) {
							data_row.push_back(latin1_to_utf8(cryptoType.name, -1));
						} else {
							data_row.push_back(s_unknown);
						}
					} else {
						data_row.push_back(rp_sprintf("%s %s (0x%02X)",
							(cryptoType.name ? cryptoType.name : s_unknown),
							(cryptoType.seed ? "+Seed" : ""),
							cryptoType.keyslot));
					}

					// Version.
					// Reference: https://3dbrew.org/wiki/Titles
					bool isUpdate;
					uint16_t version;
					if (i >= 6) {
						// System Update versions are in the partition ID.
						// TODO: Update region.
						isUpdate = true;
						version = le16_to_cpu(part_ncch_header->sysversion);
					} else {
						// Use the NCCH version.
						// NOTE: This doesn't seem to be accurate...
						isUpdate = false;
						version = le16_to_cpu(part_ncch_header->version);
					}

					if (isUpdate && version == 0x8000) {
						// Early titles have a system update with version 0x8000 (32.0.0).
						// This is usually 1.1.0, though some might be 1.0.0.
						data_row.push_back("1.x.x");
					} else {
						data_row.push_back(d->n3dsVersionToString(version));
					}
				} else {
					// Unable to load the NCCH header.
					data_row.push_back(s_unknown);	// Encryption
					data_row.push_back(s_unknown);	// Version
				}
			}

			if (keyslots) {
				// Keyslot.
				data_row.push_back(rp_sprintf("0x%02X", keyslots[i]));
			}

			// Partition size.
			const int64_t length_bytes = (int64_t)length << d->media_unit_shift;
			data_row.push_back(d->formatFileSize(length_bytes));

			delete pNcch;
		}

		// Add the partitions list data.
		d->fields->addField_listData(C_("Nintendo3DS", "Partitions"),
			v_partitions_names, partitions);
	}

	// Is the TMD header loaded?
	if (d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD) {
		// Display the TMD header.
		// NOTE: This is usually for CIAs only.
		if (haveSeparateSMDHTab) {
			d->fields->addTab("CIA");
			// Add the title ID and product code fields here.
			// (Content type is listed in the CIA contents table.)
			d->addTitleIdAndProductCodeFields(false);
		} else {
			d->fields->setTabName(0, "CIA");
		}

		// TODO: Add more fields?
		const N3DS_TMD_Header_t *const tmd_header = &d->mxh.tmd_header;

		// TODO: Required system version?

		// Version.
		const uint16_t version = be16_to_cpu(tmd_header->title_version);
		d->fields->addField_string(C_("Nintendo3DS", "Version"), d->n3dsVersionToString(version));

		// Issuer.
		// NOTE: We're using the Ticket Issuer in the TMD tab.
		// Retail Ticket will always have a Retail TMD,
		// but the issuer is technically different.
		// We're only printing "Ticket Issuer" if we can't
		// identify the issuer at all.
		const char *issuer;
		if (!strncmp(d->mxh.ticket.issuer, N3DS_TICKET_ISSUER_RETAIL, sizeof(d->mxh.ticket.issuer))) {
			// Retail issuer..
			issuer = C_("Nintendo3DS", "Retail");
		} else if (!strncmp(d->mxh.ticket.issuer, N3DS_TICKET_ISSUER_DEBUG, sizeof(d->mxh.ticket.issuer))) {
			// Debug issuer.
			issuer = C_("Nintendo3DS", "Debug");
		} else {
			// Unknown issuer.
			issuer = nullptr;
		}

		if (issuer) {
			d->fields->addField_string(C_("Nintendo3DS", "Issuer"), issuer);
		} else {
			// Print the ticket issuer as-is.
			d->fields->addField_string(C_("Nintendo3DS", "Ticket Issuer"),
				latin1_to_utf8(d->mxh.ticket.issuer, sizeof(d->mxh.ticket.issuer)));
		}

		// Demo use limit.
		if (d->mxh.ticket.limits[0] == cpu_to_be32(4)) {
			// Title has use limits.
			d->fields->addField_string_numeric(C_("Nintendo3DS", "Demo Use Limit"),
				be32_to_cpu(d->mxh.ticket.limits[1]));
		}

		// Contents table.
		// TODO: Show the ListView on a separate row?
		auto contents = new std::vector<std::vector<string> >();
		contents->reserve(d->content_count);

		// Process the contents.
		// TODO: Content types?
		const N3DS_Content_Chunk_Record_t *content_chunk = &d->content_chunks[0];
		for (unsigned int i = 0; i < d->content_count; i++, content_chunk++) {
			// Make sure the content exists first.
			NCCHReader *pNcch = nullptr;
			int ret = d->loadNCCH(i, &pNcch);
			if (ret == -ENOENT)
				continue;

			const int vidx = (int)contents->size();
			contents->resize(vidx+1);
			auto &data_row = contents->at(vidx);

			// Content index.
			data_row.push_back(rp_sprintf("%u", i));

			// TODO: Use content_chunk->index?
			const N3DS_NCCH_Header_NoSig_t *content_ncch_header = nullptr;
			const char *noncch_cnt_type = nullptr;
			if (pNcch) {
				if (pNcch->isOpen()) {
					content_ncch_header = pNcch->ncchHeader();
				} else {
					// This might be a non-NCCH content that
					// we still recognize.
					noncch_cnt_type = pNcch->contentType();
				}
			}
			if (!content_ncch_header) {
				// Invalid content index, or this content isn't an NCCH.
				// TODO: Are there CIAs with discontiguous content indexes?
				// (Themes, DLC...)
				const char *crypto = nullptr;
				if (content_chunk->type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED)) {
					// CIA encryption.
					crypto = "CIA";
				}

				if (i == 0 && d->srlData) {
					// This is an SRL.
					if (!noncch_cnt_type) {
						noncch_cnt_type = "SRL";
					}
					// TODO: Do SRLs have encryption besides CIA encryption?
					if (!crypto) {
						crypto = "NoCrypto";
					}
				} else {
					// Something else...
					if (!noncch_cnt_type) {
						noncch_cnt_type = s_unknown;
					}
				}
				data_row.push_back(noncch_cnt_type);

				// Encryption.
				data_row.push_back(crypto ? crypto : s_unknown);
				// Version.
				data_row.push_back("");

				// Content size.
				if (i < d->content_count) {
					data_row.push_back(d->formatFileSize(be64_to_cpu(content_chunk->size)));
				} else {
					data_row.push_back("");
				}
				delete pNcch;
				continue;
			}

			// Content type.
			const char *content_type = pNcch->contentType();
			data_row.push_back(content_type ? content_type : s_unknown);

			// Encryption.
			NCCHReader::CryptoType cryptoType;
			bool isCIAcrypto = !!(content_chunk->type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED));
			ret = NCCHReader::cryptoType_static(&cryptoType, content_ncch_header);
			if (ret != 0) {
				// Unknown encryption.
				cryptoType.name = nullptr;
				cryptoType.encrypted = false;
			}
			if (!cryptoType.name && isCIAcrypto) {
				// Prevent "CIA+Unknown".
				cryptoType.name = "CIA";
				cryptoType.encrypted = false;
				isCIAcrypto = false;
			}

			if (!cryptoType.encrypted || cryptoType.keyslot >= 0x40) {
				// Not encrypted, or not using a predefined keyslot.
				if (cryptoType.name) {
					data_row.push_back(latin1_to_utf8(cryptoType.name, -1));
				} else {
					data_row.push_back(s_unknown);
				}
			} else {
				// Encrypted.
				data_row.push_back(rp_sprintf("%s%s%s (0x%02X)",
					(isCIAcrypto ? "CIA+" : ""),
					(cryptoType.name ? cryptoType.name : s_unknown),
					(cryptoType.seed ? "+Seed" : ""),
					cryptoType.keyslot));
			}

			// Version. [FIXME: Might not be right...]
			const uint16_t version = le16_to_cpu(content_ncch_header->version);
			data_row.push_back(d->n3dsVersionToString(version));

			// Content size.
			data_row.push_back(d->formatFileSize(pNcch->partition_size()));

			delete pNcch;
		}

		// Add the contents table.
		static const char *const contents_names[] = {
			NOP_C_("Nintendo3DS|CtNames", "#"),
			NOP_C_("Nintendo3DS|CtNames", "Type"),
			NOP_C_("Nintendo3DS|CtNames", "Encryption"),
			NOP_C_("Nintendo3DS|CtNames", "Version"),
			NOP_C_("Nintendo3DS|CtNames", "Size"),
		};
		vector<string> *const v_contents_names = RomFields::strArrayToVector_i18n(
			"Nintendo3DS|CtNames", contents_names, ARRAY_SIZE(contents_names));
		d->fields->addField_listData(C_("Nintendo3DS", "Contents"), v_contents_names, contents);
	}

	// Get the NCCH Extended Header.
	const N3DS_NCCH_ExHeader_t *const ncch_exheader =
		(ncch && ncch->isOpen() ? ncch->ncchExHeader() : nullptr);
	if (ncch_exheader) {
		// Display the NCCH Extended Header.
		// TODO: Add more fields?
		d->fields->addTab("ExHeader");

		// Process name.
		d->fields->addField_string(C_("Nintendo3DS", "Process Name"),
			latin1_to_utf8(ncch_exheader->sci.title, sizeof(ncch_exheader->sci.title)));

		// Application type. (resource limit category)
		static const char *const appl_type_tbl[4] = {
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_APPLICATION
			NOP_C_("Nintendo3DS|ApplType", "Application"),
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_SYS_APPLET
			NOP_C_("Nintendo3DS|ApplType", "System Applet"),
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_LIB_APPLET
			NOP_C_("Nintendo3DS|ApplType", "Library Applet"),
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_OTHER
			NOP_C_("Nintendo3DS|ApplType", "SysModule"),
		};
		const uint8_t appl_type = ncch_exheader->aci.arm11_local.res_limit_category;
		if (appl_type < ARRAY_SIZE(appl_type_tbl)) {
			d->fields->addField_string(C_("Nintendo3DS", "Type"),
				dpgettext_expr(RP_I18N_DOMAIN, "Nintendo3DS|ApplType", appl_type_tbl[appl_type]));
		} else {
			d->fields->addField_string(C_("Nintendo3DS", "Type"),
				rp_sprintf(C_("Nintendo3DS", "Invalid (0x%02X)"), appl_type));
		}

		// Flags.
		static const char *const exheader_flags_names[] = {
			"CompressExefsCode", "SDApplication"
		};
		vector<string> *const v_exheader_flags_names = RomFields::strArrayToVector(
			exheader_flags_names, ARRAY_SIZE(exheader_flags_names));
		d->fields->addField_bitfield("Flags",
			v_exheader_flags_names, 0, le32_to_cpu(ncch_exheader->sci.flags));

		// TODO: Figure out what "Core Version" is.

		// System Mode.
		// TODO: Localize this? ("MB" -> "Mo" in French.)
		static const char *const old3ds_sys_mode_tbl[6] = {
			"Prod (64 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Prod
			nullptr,
			"Dev1 (96 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev1
			"Dev2 (80 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev2
			"Dev3 (72 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev3
			"Dev4 (32 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev4
		};
		const uint8_t old3ds_sys_mode = (ncch_exheader->aci.arm11_local.flags[2] &
			N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Mask) >> 4;
		if (old3ds_sys_mode < ARRAY_SIZE(old3ds_sys_mode_tbl) &&
		    old3ds_sys_mode_tbl[old3ds_sys_mode] != nullptr)
		{
			d->fields->addField_string(C_("Nintendo3DS", "Old3DS Sys Mode"),
				old3ds_sys_mode_tbl[old3ds_sys_mode]);
		} else {
			d->fields->addField_string(C_("Nintendo3DS", "Old3DS Sys Mode"),
				rp_sprintf(C_("Nintendo3DS", "Invalid (0x%02X)"), old3ds_sys_mode));
		}

		// New3DS System Mode.
		// TODO: Localize this? ("MB" -> "Mo" in French.)
		static const char *const new3ds_sys_mode_tbl[4] = {
			"Legacy (64 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Legacy
			"Prod (124 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Prod
			"Dev1 (178 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Dev1
			"Dev2 (124 MB)",	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Dev2
		};
		const uint8_t new3ds_sys_mode = ncch_exheader->aci.arm11_local.flags[1] &
			N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Mask;
		if (new3ds_sys_mode < ARRAY_SIZE(new3ds_sys_mode_tbl)) {
			d->fields->addField_string(C_("Nintendo3DS", "New3DS Sys Mode"),
				new3ds_sys_mode_tbl[new3ds_sys_mode]);
		} else {
			d->fields->addField_string(C_("Nintendo3DS", "New3DS Sys Mode"),
				rp_sprintf(C_("Nintendo3DS", "Invalid (0x%02X)"), new3ds_sys_mode));
		}

		// New3DS CPU Mode.
		static const char *const new3ds_cpu_mode_names[] = {
			NOP_C_("Nintendo3DS|N3DSCPUMode", "L2 Cache"),
			NOP_C_("Nintendo3DS|N3DSCPUMode", "804 MHz"),
		};
		vector<string> *const v_new3ds_cpu_mode_names = RomFields::strArrayToVector_i18n(
			"Nintendo3DS|N3DSCPUMode", new3ds_cpu_mode_names, ARRAY_SIZE(new3ds_cpu_mode_names));
		d->fields->addField_bitfield("New3DS CPU Mode",
			v_new3ds_cpu_mode_names, 0, ncch_exheader->aci.arm11_local.flags[0]);

		// TODO: Ideal CPU and affinity mask.
		// TODO: core_version is probably specified for e.g. AGB.
		// Indicate that somehow.

		// Permissions. These are technically part of the
		// ExHeader, but we're using a separate tab because
		// there's a lot of them.
		d->fields->addTab(C_("Nintendo3DS", "Permissions"));
		d->addFields_permissions(ncch_exheader);
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

	RP_D(Nintendo3DS);
	if (d->romType == Nintendo3DSPrivate::ROM_TYPE_CIA) {
		// TMD needs to be loaded so we can check if it's a DSiWare SRL.
		if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
			d->loadTicketAndTMD();
		}
		if (d->srlData) {
			// This is a DSiWare SRL.
			// Get the image from the underlying SRL.
			const rp_image *image = d->srlData->image(imageType);
			*pImage = image;
			return (image ? 0 : -EIO);
		}
	}

	// NOTE: Assuming icon index 1. (48x48)
	const int idx = 1;

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
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
const IconAnimData *Nintendo3DS::iconAnimData(void) const
{
	// NOTE: Nintendo 3DS icons cannot be animated.
	// Nintendo DSi icons can be animated, so this is
	// only used if we're looking at a DSiWare SRL
	// packaged as a CIA.
	RP_D(const Nintendo3DS);
	if (d->srlData) {
		return d->srlData->iconAnimData();
	}
	return nullptr;
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

	RP_D(const Nintendo3DS);
	if (d->romType == Nintendo3DSPrivate::ROM_TYPE_CIA) {
		// TMD needs to be loaded so we can check if it's a DSiWare SRL.
		if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
			const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
		}
		if (d->srlData) {
			// This is a DSiWare SRL.
			// Get the image URLs from the underlying SRL.
			return d->srlData->extURLs(imageType, pExtURLs, size);
		}
	}

	// Make sure the NCCH header is loaded.
	const N3DS_NCCH_Header_NoSig_t *const ncch_header =
		const_cast<Nintendo3DSPrivate*>(d)->loadNCCHHeader();
	if (!ncch_header) {
		// Unable to load the NCCH header.
		// Cannot create URLs.
		return -ENOENT;
	}

	// If using NCSD, use the Media ID.
	// Otherwise, use the primary Title ID.
	uint32_t tid_hi, tid_lo;
	if (d->headers_loaded & Nintendo3DSPrivate::HEADER_NCSD) {
		tid_lo = le32_to_cpu(d->mxh.ncsd_header.media_id.lo);
		tid_hi = le32_to_cpu(d->mxh.ncsd_header.media_id.hi);
	} else if (ncch_header) {
		tid_lo = le32_to_cpu(ncch_header->program_id.lo);
		tid_hi = le32_to_cpu(ncch_header->program_id.hi);
	} else {
		// Unlikely, but cppcheck complained.
		tid_lo = 0;
		tid_hi = 0;
	}

	// Validate the title ID.
	// Reference: https://3dbrew.org/wiki/Titles
	if (tid_hi != 0x00040000 || tid_lo < 0x00030000 || tid_lo >= 0x0F800000) {
		// This is probably not a retail application
		// because one of the following conditions is met:
		// - TitleID High is not 0x00040000
		// - TitleID Low unique ID is  <   0x300 (system)
		// - TitleID Low unique ID is >= 0xF8000 (eval/proto/dev)
		return -ENOENT;
	}

	// Validate the product code.
	if (memcmp(ncch_header->product_code, "CTR-", 4) != 0 &&
	    memcmp(ncch_header->product_code, "KTR-", 4) != 0)
	{
		// Not a valid product code for GameTDB.
		return -ENOENT;
	}

	if (ncch_header->product_code[5] != '-' ||
	    ncch_header->product_code[10] != 0)
	{
		// Missing hyphen, or longer than 10 characters.
		return -ENOENT;
	}

	// Check the product type.
	// TODO: Enable demos, DLC, and updates?
	switch (ncch_header->product_code[4]) {
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
	const char *id4 = &ncch_header->product_code[6];
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
			? le32_to_cpu(d->smdh.header.settings.region_code)
			: 0;
	vector<const char*> tdb_regions = d->n3dsRegionToGameTDB(smdhRegion, id4[3]);

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	const ImageSizeDef *szdefs_dl[2];
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
