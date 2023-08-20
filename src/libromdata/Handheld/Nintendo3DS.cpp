/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS.hpp: Nintendo 3DS ROM reader.                               *
 * Handles CCI/3DS, CIA, and SMDH files.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "Nintendo3DS.hpp"
#include "Nintendo3DS_p.hpp"
#include "n3ds_structs.h"

// Other rom-properties libraries
#include "librpbase/config/Config.hpp"
#include "librpbase/Achievements.hpp"
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// For sections delegated to other RomData subclasses.
#include "Nintendo3DS_SMDH.hpp"
#include "NintendoDS.hpp"

// NCCH and CIA readers.
#include "disc/NCCHReader.hpp"
#include "disc/CIAReader.hpp"

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::vector;

// zlib for crc32()
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

ROMDATA_IMPL(Nintendo3DS)
ROMDATA_IMPL_IMG_SIZES(Nintendo3DS)

/** Nintendo3DSPrivate **/

/* RomDataInfo */
const char *const Nintendo3DSPrivate::exts[] = {
	".3dsx",	// Homebrew application.
	".3ds",		// ROM image (NOTE: Conflicts with 3DS Max.)
	".3dz",		// ROM image (with private header for Gateway 3DS)
	".cci",		// ROM image
	".cia",		// CTR importable archive
	".ncch",	// NCCH file
	".app",		// NCCH file (NOTE: May conflict with others...)
	".cxi",		// CTR Executable Image (NCCH)
	".cfa",		// CTR File Archive (NCCH)
	".csu",		// CTR System Update (CCI)

	nullptr
};
const char *const Nintendo3DSPrivate::mimeTypes[] = {
	// NOTE: Ordering matches N3DS_RomType.

	// Unofficial MIME types.
	"application/x-nintendo-3ds-executable",	// on fd.o
	"application/x-nintendo-3ds-rom",		// on fd.o
	"application/x-nintendo-3ds-emmc",		// NOT on fd.o
	"application/x-nintendo-3ds-cia",		// NOT on fd.o
	"application/x-nintendo-3ds-ncch",		// NOT on fd.o

	// Unofficial MIME types.
	// NOT used by N3DS_RomType.
	// TODO: Add a specific type for CXI?
	"application/x-nintendo-3ds-cxi",		// NOT on fd.o

	// Unofficial MIME types from Citra.
	// NOT used by N3DS_RomType.
	"application/x-ctr-3dsx",
	"application/x-ctr-cci",
	"application/x-ctr-cia",
	"application/x-ctr-cxi",

	nullptr
};
const RomDataInfo Nintendo3DSPrivate::romDataInfo = {
	"Nintendo3DS", exts, mimeTypes
};

Nintendo3DSPrivate::Nintendo3DSPrivate(IRpFile *file)
	: super(file, &romDataInfo)
	, romType(RomType::Unknown)
	, headers_loaded(0)
	, media_unit_shift(9)	// default is 9 (512 bytes)
	, ncch_reader(nullptr)
	, mainContent(nullptr)
{
	// Clear the various structs.
	memset(&mxh, 0, sizeof(mxh));
	memset(&perm, 0, sizeof(perm));
}

Nintendo3DSPrivate::~Nintendo3DSPrivate()
{
	UNREF(mainContent);
	UNREF(ncch_reader);
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

	static const size_t N3DS_SMDH_Section_Size =
		sizeof(N3DS_SMDH_Header_t) + sizeof(N3DS_SMDH_Icon_t);

	SubFile *smdhFile = nullptr;
	switch (romType) {
		default:
			// Unsupported...
			return -1;

		case RomType::_3DSX: {
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

			// Open the SMDH section.
			smdhFile = new SubFile(this->file, le32_to_cpu(mxh.hb3dsx_header.smdh_offset), N3DS_SMDH_Section_Size);
			break;
		}

		case RomType::CIA:
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
			if (le32_to_cpu(mxh.cia_header.meta_size) >= (uint32_t)N3DS_SMDH_Section_Size) {
				// Determine the SMDH starting address.
				const uint32_t addr =
					toNext64(le32_to_cpu(mxh.cia_header.header_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.cert_chain_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.ticket_size)) +
					toNext64(le32_to_cpu(mxh.cia_header.tmd_size)) +
					toNext64(static_cast<uint32_t>(le64_to_cpu(mxh.cia_header.content_size))) +
					(uint32_t)sizeof(N3DS_CIA_Meta_Header_t);

				// Open the SMDH section.
				// TODO: Verify that this works.
				smdhFile = new SubFile(this->file, addr, N3DS_SMDH_Section_Size);
				break;
			}

			// Either there's no meta section, or the SMDH section
			// wasn't valid. Try loading from the ExeFS.
			// fall-through

		case RomType::CCI:
		case RomType::NCCH: {
			// CCI file, CIA file with no meta section, or NCCH file.
			// Open "exefs:/icon".
			NCCHReader *const ncch_reader = loadNCCH();
			if (!ncch_reader || !ncch_reader->isOpen()) {
				// Unable to open the primary NCCH section.
				return -6;
			}

			IRpFile *const ncch_f_icon = ncch_reader->open(N3DS_NCCH_SECTION_EXEFS, "icon");
			if (!ncch_f_icon) {
				// Failed to open "icon".
				return -7;
			} else if (ncch_f_icon->size() < (off64_t)N3DS_SMDH_Section_Size) {
				// Icon is too small.
				ncch_f_icon->unref();
				return -8;
			}

			// Create the SMDH subfile.
			smdhFile = new SubFile(ncch_f_icon, 0, N3DS_SMDH_Section_Size);
			ncch_f_icon->unref();
			break;
		}
	}

	if (!smdhFile || !smdhFile->isOpen()) {
		// Unable to open the SMDH subfile.
		UNREF(smdhFile);
		return -9;
	}

	// Open the SMDH RomData subclass.
	Nintendo3DS_SMDH *const smdhData = new Nintendo3DS_SMDH(smdhFile);
	smdhFile->unref();
	if (!smdhData->isOpen()) {
		// Unable to open the SMDH file.
		smdhData->unref();
		return -11;
	}

	// Loaded the SMDH section.
	headers_loaded |= HEADER_SMDH;
	this->mainContent = smdhData;
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

	off64_t offset = 0;
	uint32_t length = 0;
	switch (romType) {
		case RomType::CIA: {
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
			if (static_cast<unsigned int>(idx) >= content_chunks.size()) {
				// Content index is out of range.
				return -ENOENT;
			}

			// Determine the content start position.
			// Need to add all content chunk sizes, algined to 64 bytes.
			for (const N3DS_Content_Chunk_Record_t &p : content_chunks) {
				const uint32_t cur_size = static_cast<uint32_t>(be64_to_cpu(p.size));
				if (be16_to_cpu(p.index) == idx) {
					// Found the content chunk.
					length = cur_size;
					break;
				}
				// Next chunk.
				offset += toNext64(cur_size);
			}
			if (length == 0) {
				// Content chunk not found.
				return -ENOENT;
			}

			// Add the content start address.
			offset += mxh.content_start_addr;
			break;
		}

		case RomType::CCI: {
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
			offset = static_cast<off64_t>(le32_to_cpu(mxh.ncsd_header.partitions[idx].offset)) << media_unit_shift;
			length = le32_to_cpu(mxh.ncsd_header.partitions[idx].length) << media_unit_shift;
			// TODO: Validate length.
			// Make sure the partition starts after the card info header.
			if (offset <= 0x2000) {
				// Invalid partition offset.
				return -EIO;
			}
			break;
		}

		case RomType::NCCH: {
			// NCCH file. Only one content.
			if (idx != 0) {
				// Invalid content index.
				return -ENOENT;
			}
			offset = 0;
			length = static_cast<uint32_t>(file->size());
			break;
		}

		default:
			// Unsupported...
			return -ENOTSUP;
	}

	// Is this encrypted using CIA title key encryption?
	CIAReader *ciaReader = nullptr;
	if (romType == RomType::CIA && idx < (int)content_chunks.size()) {
		// Check if this content is encrypted.
		// If it is, we'll need to create a CIAReader.
		N3DS_Ticket_t *ticket = nullptr;
		for (const N3DS_Content_Chunk_Record_t &p : content_chunks) {
			const uint16_t content_index = be16_to_cpu(p.index);
			if (content_index == idx) {
				// Found the content index.
				if (p.type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED)) {
					// Content is encrypted.
					ticket = &mxh.ticket;
				}
				break;
			}
		}

		if (ticket) {
			// Create a CIAReader.
			ciaReader = new CIAReader(file, offset, length, ticket, idx);
			if (!ciaReader->isOpen()) {
				// Unable to open the CIAReader.
				UNREF_AND_NULL_NOCHK(ciaReader);
			}
		}
	}

	// Create the NCCHReader.
	// NOTE: We're not checking isOpen() here.
	// That should be checked by the caller.
	if (ciaReader && ciaReader->isOpen()) {
		// This is an encrypted CIA.
		// NOTE 2: CIAReader handles the offset, so we need to
		// tell NCCHReader that the offset is 0.
		*pOutNcchReader = new NCCHReader(ciaReader, media_unit_shift, 0, length);
	} else {
		// Anything else is read directly.
		*pOutNcchReader = new NCCHReader(file, media_unit_shift, offset, length);
	}

	// We don't need to keep a reference to the CIAReader.
	UNREF(ciaReader);
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
	if (romType == RomType::CIA) {
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
	} else if (romType != RomType::CIA) {
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
	unsigned int content_count = be16_to_cpu(mxh.tmd_header.content_count);
	if (content_count > 255) {
		// TODO: Do any titles have more than 255 contents?
		// Restricting to 255 maximum for now.
		content_count = 255;
	}
	content_chunks.resize(content_count);
	const size_t content_chunks_size = content_count * sizeof(N3DS_Content_Chunk_Record_t);

	addr += sizeof(N3DS_TMD_t);
	size = file->seekAndRead(addr, content_chunks.data(), content_chunks_size);
	if (size != content_chunks_size) {
		// Seek and/or read error.
		content_chunks.clear();
		return -10;
	}

	// Store the content start address.
	mxh.content_start_addr = tmd_start + toNext64(tmd_size);

	// Loaded the TMD header.
	headers_loaded |= HEADER_TMD;

	// Check if the CIA is DSiWare.
	// NOTE: "WarioWare Touched!" has a manual, but no other
	// DSiWare titles that I've seen do.
	if (content_count <= 2 && !(headers_loaded & HEADER_SMDH) && !this->mainContent) {
		openSRL();
	}

	return 0;
}

/**
 * Open the SRL if it isn't already opened.
 * This operation only works for CIAs that contain an SRL.
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::openSRL(void)
{
	if (romType != RomType::CIA || content_chunks.empty()) {
		return -ENOENT;
	} else if (this->mainContent) {
		// Something's already loaded.
		if (this->mainContent->isOpen()) {
			// File is still open.
			// Return 0 if it's an SRL; -ENOENT otherwise.
			return (!(headers_loaded & HEADER_SMDH) ? 0 : -ENOENT);
		}
		// File is no longer open.
		// unref() and reopen it.
		UNREF_AND_NULL(this->mainContent);
	}

	if (!this->file || !this->file->isOpen()) {
		// Can't open the SRL.
		return -EIO;
	}

	const N3DS_Content_Chunk_Record_t *const chunk0 = &content_chunks[0];
	const off64_t offset = mxh.content_start_addr;
	const uint32_t length = static_cast<uint32_t>(be64_to_cpu(chunk0->size));
	if (length < 0x8000) {
		return -ENOENT;
	}

	// Attempt to open the SRL as if it's a new file.
	// TODO: IRpFile implementation with offset/length, so we don't
	// have to use both DiscReader and PartitionFile.

	// Check if this content is encrypted.
	// If it is, we'll need to create a CIAReader.
	IDiscReader *srlReader = nullptr;
	if (chunk0->type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED)) {
		// Content is encrypted.
		srlReader = new CIAReader(this->file, offset, length,
			&mxh.ticket, be16_to_cpu(chunk0->index));
	} else {
		// Content is NOT encrypted.
		// Use a plain old DiscReader.
		srlReader = new DiscReader(this->file, offset, length);
	}
	if (!srlReader->isOpen()) {
		// Unable to open the SRL reader.
		srlReader->unref();
		return -EIO;
	}

	// TODO: Make IDiscReader derive from IRpFile.
	// May need to add reference counting to IRpFile...
	NintendoDS *srlData = nullptr;
	PartitionFile *const srlFile = new PartitionFile(srlReader, 0, length);
	srlReader->unref();
	if (srlFile->isOpen()) {
		// Create the NintendoDS object.
		srlData = new NintendoDS(srlFile, true);
	}
	srlFile->unref();

	if (srlData && srlData->isOpen() && srlData->isValid()) {
		// SRL opened successfully.
		this->mainContent = srlData;
	} else {
		// Failed to open the SRL.
		UNREF(srlData);
	}

	return (this->mainContent != nullptr ? 0 : -EIO);
}

/**
 * Get the SMDH region code.
 * @return SMDH region code, or 0 if it could not be obtained.
 */
uint32_t Nintendo3DSPrivate::getSMDHRegionCode(void)
{
	uint32_t smdhRegion = 0;
	if ((headers_loaded & Nintendo3DSPrivate::HEADER_SMDH) || loadSMDH() == 0) {
		// SMDH section loaded.
		Nintendo3DS_SMDH *const smdh = dynamic_cast<Nintendo3DS_SMDH*>(this->mainContent);
		if (smdh) {
			smdhRegion = smdh->getRegionCode();
		}
	}
	return smdhRegion;
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

	// The program ID will also be retrieved from the NCCH header
	// and will be printed if it doesn't match the title ID.

	// NCCH header.
	NCCHReader *const ncch = loadNCCH();
	const N3DS_NCCH_Header_NoSig_t *const ncch_header =
		(ncch && ncch->isOpen() ? ncch->ncchHeader() : nullptr);

	const char *tid_desc = nullptr;
	uint32_t tid_hi, tid_lo;
	if (romType == Nintendo3DSPrivate::RomType::CCI &&
	    headers_loaded & Nintendo3DSPrivate::HEADER_NCSD)
	{
		tid_desc = C_("Nintendo3DS", "Media ID");
		tid_lo = le32_to_cpu(mxh.ncsd_header.media_id.lo);
		tid_hi = le32_to_cpu(mxh.ncsd_header.media_id.hi);
	} else if ((headers_loaded & Nintendo3DSPrivate::HEADER_TMD) || loadTicketAndTMD() == 0) {
		tid_desc = C_("Nintendo", "Title ID");
		tid_hi = be32_to_cpu(mxh.tmd_header.title_id.hi);
		tid_lo = be32_to_cpu(mxh.tmd_header.title_id.lo);
	} else if (ncch_header) {
		tid_desc = C_("Nintendo", "Title ID");
		tid_lo = le32_to_cpu(ncch_header->title_id.lo);
		tid_hi = le32_to_cpu(ncch_header->title_id.hi);
	}

	if (tid_desc) {
		fields.addField_string(tid_desc, rp_sprintf("%08X-%08X", tid_hi, tid_lo));
	}

	if (!ncch_header) {
		// Unable to open the NCCH header.
		return;
	}

	// Program ID, if different from title ID.
	if (ncch_header->program_id.id != ncch_header->title_id.id) {
		const uint32_t pid_lo = le32_to_cpu(ncch_header->program_id.lo);
		const uint32_t pid_hi = le32_to_cpu(ncch_header->program_id.hi);
		fields.addField_string(C_("Nintendo3DS", "Program ID"),
			rp_sprintf("%08X-%08X", pid_hi, pid_lo));
	}

	// Product code.
	fields.addField_string(C_("Nintendo3DS", "Product Code"),
		latin1_to_utf8(ncch_header->product_code, sizeof(ncch_header->product_code)));

	// Content type.
	// This is normally shown in the CIA content table.
	if (showContentType) {
		const char *const content_type = ncch->contentType();
		// TODO: Remove context from "Unknown" and "Invalid" strings.
		fields.addField_string(C_("Nintendo3DS", "Content Type"),
			(content_type ? content_type : C_("RomData", "Unknown")));

#ifdef ENABLE_DECRYPTION
		// Only show the encryption type if a TMD isn't available.
		if (loadTicketAndTMD() != 0) {
			fields.addField_string(C_("Nintendo3DS", "Issuer"),
				ncch->isDebug()
					? C_("Nintendo3DS", "Debug")
					: C_("Nintendo3DS", "Retail"));
		}
#endif /* ENABLE_DECRYPTION */

		// Encryption.
		const char *const s_encryption = C_("Nintendo3DS", "Encryption");
		const char *const s_unknown = C_("RomData", "Unknown");
		NCCHReader::CryptoType cryptoType = {nullptr, false, 0, false};
		int ret = NCCHReader::cryptoType_static(&cryptoType, ncch_header);
		if (ret != 0 || !cryptoType.encrypted || cryptoType.keyslot >= 0x40) {
			// Not encrypted, or not using a predefined keyslot.
			fields.addField_string(s_encryption,
				cryptoType.name
					? latin1_to_utf8(cryptoType.name, -1)
					: s_unknown);
		} else {
			fields.addField_string(s_encryption,
				rp_sprintf("%s%s (0x%02X)",
					(cryptoType.name ? cryptoType.name : s_unknown),
					(cryptoType.seed ? "+Seed" : ""),
					cryptoType.keyslot));
		}
	}

	// Logo.
	// NOTE: All known official logo binaries are 8 KB.
	// The original and new "Homebrew" logos are also 8 KB.
	uint32_t crc = 0;
	IRpFile *const f_logo = ncch->openLogo();
	if (f_logo) {
		const off64_t szFile = f_logo->size();
		if (szFile == 8192) {
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
			// Delay load verification.
			// TODO: Only if linked with /DELAYLOAD?
			bool has_zlib = true;
			if (DelayLoad_test_get_crc_table() != 0) {
				// Delay load failed.
				// Can't calculate the CRC32.
				has_zlib = false;
			}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
			// zlib isn't in a DLL, but we need to ensure that the
			// CRC table is initialized anyway.
			static const bool has_zlib = true;
			get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

			if (has_zlib) {
				// Calculate the CRC32.
				unique_ptr<uint8_t[]> buf(new uint8_t[static_cast<unsigned int>(szFile)]);
				size_t size = f_logo->read(buf.get(), static_cast<unsigned int>(szFile));
				if (size == static_cast<unsigned int>(szFile)) {
					crc = crc32(0, buf.get(), static_cast<unsigned int>(szFile));
				}
			}
		} else if (szFile > 0) {
			// Some other custom logo.
			crc = 1;
		}
		f_logo->unref();
	}

	struct logo_crc_tbl_t {
		uint32_t crc;
		const char *name;
	};
	static const logo_crc_tbl_t logo_crc_tbl[] = {
		// Official logos
		// NOTE: Not translatable!
		{0xCFD0EB8BU,	"Nintendo"},
		{0x1093522BU,	"Licensed by Nintendo"},
		{0x4FA8771CU,	"Distributed by Nintendo"},
		{0x7F68B548U,	"iQue"},
		{0xD8907ED7U,	"iQue (System)"},

		// Homebrew logos
		// TODO: Make them translatable?

		// "Old" static Homebrew logo.
		// Included with `makerom`.
		{0x343A79D9U,	"Homebrew (static)"},

		// "New" animated Homebrew logo.
		// Uses the Homebrew Launcher theme.
		// Reference: https://gbatemp.net/threads/release-default-homebrew-custom-logo-bin.457611/
		{0xF257BD67U,	"Homebrew (animated)"},
	};

	// If CRC is zero, we don't have a valid logo section.
	// Otherwise, search for a matching logo.
	const char *logo_name = nullptr;
	if (crc != 0) {
		// Search for a matching logo.
		static const logo_crc_tbl_t *const p_logo_crc_tbl_end = &logo_crc_tbl[ARRAY_SIZE(logo_crc_tbl)];
		auto iter = std::find_if(logo_crc_tbl, p_logo_crc_tbl_end,
			[crc](const logo_crc_tbl_t &p) noexcept -> bool {
				return (p.crc == crc);
			});
		if (iter != p_logo_crc_tbl_end) {
			// Found a matching logo.
			logo_name = iter->name;
		}

		if (!logo_name) {
			// Custom logo.
			logo_name = C_("Nintendo3DS|Logo", "Custom");
		}
	}

	if (logo_name) {
		fields.addField_string(C_("Nintendo3DS", "Logo"), logo_name);
	}
}

/**
 * Convert a Nintendo 3DS region value to a GameTDB language code.
 * @param smdhRegion Nintendo 3DS region. (from SMDH)
 * @param idRegion Game ID region.
 *
 * NOTE: Mulitple GameTDB language codes may be returned, including:
 * - User-specified fallback language code for PAL.
 * - General fallback language code.
 *
 * @return GameTDB language code(s), or empty vector if the region value is invalid.
 * NOTE: The language code may need to be converted to uppercase!
 */
vector<uint16_t> Nintendo3DSPrivate::n3dsRegionToGameTDB(uint32_t smdhRegion, char idRegion)
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
	vector<uint16_t> ret;
	ret.reserve(3);

	int fallback_region = 0;
	switch (smdhRegion) {
		case N3DS_REGION_JAPAN:
			ret.push_back('JA');
			return ret;
		case N3DS_REGION_USA:
			ret.push_back('US');
			return ret;
		case N3DS_REGION_EUROPE:
		case N3DS_REGION_EUROPE | N3DS_REGION_AUSTRALIA:
			// Process the game ID and use 'EN' as a fallback.
			fallback_region = 1;
			break;
		case N3DS_REGION_AUSTRALIA:
			// Process the game ID and use 'AU','EN' as fallbacks.
			fallback_region = 2;
			break;
		case N3DS_REGION_CHINA:
			// NOTE: GameTDB only has 'ZH' for boxart, not 'ZHCN' or 'ZHTW'.
			ret.push_back('ZH');
			ret.push_back('JA');
			ret.push_back('EN');
			return ret;
		case N3DS_REGION_SOUTH_KOREA:
			ret.push_back('KO');
			ret.push_back('JA');
			ret.push_back('EN');
			return ret;
		case N3DS_REGION_TAIWAN:
			// NOTE: GameTDB only has 'ZH' for boxart, not 'ZHCN' or 'ZHTW'.
			ret.push_back('ZH');
			ret.push_back('JA');
			ret.push_back('EN');
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
			ret.push_back('US');
			break;
		case 'J':	// Japan
			ret.push_back('JA');
			break;
		case 'P':	// PAL
		case 'X':	// Multi-language release
		case 'Y':	// Multi-language release
		case 'L':	// Japanese import to PAL regions
		case 'M':	// Japanese import to PAL regions
		default: {
			// Generic PAL release.
			// Use the user-specified fallback.
			const Config *const config = Config::instance();
			const uint32_t lc = config->palLanguageForGameTDB();
			if (lc != 0 && lc < 65536) {
				ret.emplace_back(static_cast<uint16_t>(lc));
				// Don't add English again if that's what the
				// user-specified fallback language is.
				if (lc != 'en' && lc != 'EN') {
					fallback_region = 1;
				}
			} else {
				// Invalid. Use 'EN'.
				fallback_region = 1;
			}
			break;
		}

		// European regions.
		case 'D':	// Germany
			ret.push_back('DE');
			break;
		case 'F':	// France
			ret.push_back('FR');
			break;
		case 'H':	// Netherlands
			ret.push_back('NL');
			break;
		case 'I':	// Italy
			ret.push_back('IT');
			break;
		case 'R':	// Russia
			ret.push_back('RU');
			break;
		case 'S':	// Spain
			ret.push_back('ES');
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
			ret.push_back('EN');
			break;
		case 2:
			// Australia
			ret.push_back('AU');
			ret.push_back('EN');
			break;

		case 3:
			// TODO: Check the host system region.
			// For now, assuming US.
			ret.push_back('US');
			break;

		case 0:	// None
		default:
			break;
	}

	return ret;
}

/**
 * Load the permissions values. (from ExHeader)
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::loadPermissions(void)
{
	if (perm.isLoaded) {
		// Permissions have already been loaded.
		return 0;
	}

	const NCCHReader *const ncch = loadNCCH();
	if (!ncch || !ncch->isOpen()) {
		// Can't open the primary NCCH.
		return -1;
	}

	// Get the NCCH Header.
	// TODO: With signature?
	const N3DS_NCCH_Header_NoSig_t *const ncch_header = ncch->ncchHeader();
	if (!ncch_header) {
		// Can't get the header.
		return -2;
	}

	// Get the NCCH Extended Header.
	const N3DS_NCCH_ExHeader_t *const ncch_exheader = ncch->ncchExHeader();
	if (!ncch_exheader) {
		// Can't get the ExHeader.
		return -3;
	}

	// Save the permissions.
	perm.fsAccess = static_cast<uint32_t>(le64_to_cpu(ncch_exheader->aci.arm11_local.storage.fs_access));

	// TODO: Other descriptor versions?
	// v2 is standard; may be v3 on 9.3.0-X.
	// FIXME: Some pre-release images have version 0.
	perm.ioAccess = static_cast<uint32_t>(le32_to_cpu(ncch_exheader->aci.arm9.descriptors));
	perm.ioAccessVersion = ncch_exheader->aci.arm9.descriptor_version;

	// Save a pointer to the services array.
	perm.services = ncch_exheader->aci.arm11_local.services;

	// TODO: Ignore permissions on system titles.
	// TODO: Check permissions on retail games and compare to this list.
	static const uint32_t fsAccess_dangerous =
		// mset has CategorySystemApplication set.
		N3DS_NCCH_EXHEADER_ACI_FsAccess_CategorySystemApplication |
		// TinyFormat has CategoryFilesystemTool set.
		N3DS_NCCH_EXHEADER_ACI_FsAccess_CategoryFilesystemTool |
		N3DS_NCCH_EXHEADER_ACI_FsAccess_CtrNandRo |
		N3DS_NCCH_EXHEADER_ACI_FsAccess_CtrNandRw |
		N3DS_NCCH_EXHEADER_ACI_FsAccess_CtrNandRoWrite |
		// mset has CategorySystemSettings set.
		N3DS_NCCH_EXHEADER_ACI_FsAccess_CategorySystemSettings;
	static const uint32_t ioAccess_dangerous =
		N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountNand |
		N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountNandRoWrite |
		N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountTwln |
		N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountWnand |
		N3DS_NCCH_EXHEADER_ACI_IoAccess_UseSdif3;

	// Check for "dangerous" permissions.
	if ((perm.fsAccess & fsAccess_dangerous) ||
	    (perm.ioAccess & ioAccess_dangerous))
	{
		// One or more "dangerous" permissions are set.
		// TODO: Also highlight "dangerous" permissions in the ROM Properties tab.
		perm.isDangerous = true;
	}

	// We're done here.
	return 0;
}

/**
 * Add the Permissions fields. (part of ExHeader)
 * A separate tab should be created by the caller first.
 * @param pNcchExHeader NCCH ExHeader.
 * @return 0 on success; non-zero on error.
 */
int Nintendo3DSPrivate::addFields_permissions(void)
{
	int ret = loadPermissions();
	if (ret != 0) {
		// Unable to load permissions.
		return ret;
	}

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
	auto vv_fs = new RomFields::ListData_t(ARRAY_SIZE(perm_fs_access));
	for (int i = ARRAY_SIZE(perm_fs_access)-1; i >= 0; i--) {
		auto &data_row = vv_fs->at(i);
		data_row.emplace_back(perm_fs_access[i]);
	}

	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_CHECKBOXES, rows_visible);
	params.headers = nullptr;
	params.data.single = vv_fs;
	params.mxd.checkboxes = perm.fsAccess;
	fields.addField_listData(C_("Nintendo3DS", "FS Access"), &params);

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
	// FIXME: Some pre-release images have version 0.
	if (perm.ioAccessVersion == 2 ||
	    perm.ioAccessVersion == 3)
	{
		// Convert to RomFields::ListData_t for RFT_LISTDATA.
		auto vv_arm9 = new RomFields::ListData_t(ARRAY_SIZE(perm_arm9_access));
		for (int i = ARRAY_SIZE(perm_arm9_access)-1; i >= 0; i--) {
			auto &data_row = vv_arm9->at(i);
			data_row.emplace_back(perm_arm9_access[i]);
		}

		params.data.single = vv_arm9;
		params.mxd.checkboxes = perm.ioAccess;
		fields.addField_listData(C_("Nintendo3DS", "ARM9 Access"), &params);
	}

	// Services. Each service is a maximum of 8 characters.
	// The field is NULL-padded, though if the service name
	// is 8 characters long, there won't be any NULLs.
	// TODO: How to determine 32 or 34? (descriptor version?)
	auto vv_svc = new RomFields::ListData_t();
	vv_svc->reserve(N3DS_SERVICE_MAX);
	const char *svc = perm.services[0];
	for (unsigned int i = 0; i < N3DS_SERVICE_MAX; i++, svc += N3DS_SERVICE_LEN) {
		if (svc[0] == 0) {
			// End of service list.
			break;
		}

		// Add the service.
		// TODO: Service descriptions?
		vv_svc->resize(vv_svc->size()+1);
		auto &data_row = vv_svc->at(vv_svc->size()-1);
		data_row.emplace_back(latin1_to_utf8(svc, N3DS_SERVICE_LEN));
	}

	if (likely(!vv_svc->empty())) {
		params.flags = 0;
		params.data.single = vv_svc;
		fields.addField_listData(C_("Nintendo3DS", "Services"), &params);
	} else {
		// No services.
		delete vv_svc;
	}

	return 0;
}

/** Nintendo3DS **/

/**
 * Read a Nintendo 3DS ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
Nintendo3DS::Nintendo3DS(IRpFile *file)
	: super(new Nintendo3DSPrivate(file))
{
	// This class handles several different types of files,
	// so we'll initialize d->fileType later.
	RP_D(Nintendo3DS);
	d->fileType = FileType::Unknown;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	uint8_t header[0x2020];	// large enough for CIA headers
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this ROM image is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(header), header},
		FileSystem::file_ext(filename),	// ext
		d->file->size()			// szFile (not needed for NGPC)
	};
	d->romType = static_cast<Nintendo3DSPrivate::RomType>(isRomSupported_static(&info));

	// Determine what kind of file this is.
	// NOTE: SMDH header and icon will be loaded on demand.
	switch (d->romType) {
		case Nintendo3DSPrivate::RomType::_3DSX:
			// Save the 3DSX header for later.
			memcpy(&d->mxh.hb3dsx_header, header, sizeof(d->mxh.hb3dsx_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_3DSX;
			d->fileType = FileType::Homebrew;
			break;

		case Nintendo3DSPrivate::RomType::CIA:
			// Save the CIA header for later.
			memcpy(&d->mxh.cia_header, header, sizeof(d->mxh.cia_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_CIA;
			d->fileType = FileType::ApplicationPackage;
			break;

		case Nintendo3DSPrivate::RomType::CCI:
			// Save the NCSD and Card Info headers for later.
			memcpy(&d->mxh.ncsd_header, &header[N3DS_NCSD_NOSIG_HEADER_ADDRESS], sizeof(d->mxh.ncsd_header));
			memcpy(&d->mxh.cinfo_header, &header[N3DS_NCSD_CARD_INFO_HEADER_ADDRESS], sizeof(d->mxh.cinfo_header));

			// NCSD may have a larger media unit shift.
			// FIXME: Handle invalid shift values?
			d->media_unit_shift = 9 + d->mxh.ncsd_header.cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_UNIT_SIZE];

			d->headers_loaded |= Nintendo3DSPrivate::HEADER_NCSD;
			d->fileType = FileType::ROM_Image;
			break;

		case Nintendo3DSPrivate::RomType::eMMC:
			// Save the NCSD header for later.
			memcpy(&d->mxh.ncsd_header, &header[N3DS_NCSD_NOSIG_HEADER_ADDRESS], sizeof(d->mxh.ncsd_header));
			d->headers_loaded |= Nintendo3DSPrivate::HEADER_NCSD;
			d->fileType = FileType::eMMC_Dump;
			break;

		case Nintendo3DSPrivate::RomType::NCCH:
			// NCCH reader will be created when loadNCCH() is called.
			// TODO: Better type.
			d->fileType = FileType::ContainerFile;
			break;

		default:
			// Unknown ROM format.
			d->romType = Nintendo3DSPrivate::RomType::Unknown;
			UNREF_AND_NULL_NOCHK(d->file);
			return;
	}

	// Set the MIME type.
	d->mimeType = d->mimeTypes[(int)d->romType];

	// File is valid.
	d->isValid = true;
}

/**
 * Close the opened file.
 */
void Nintendo3DS::close(void)
{
	RP_D(Nintendo3DS);

	// Close any child RomData subclasses.
	if (d->mainContent) {
		d->mainContent->close();
	}

	// Call the superclass function.
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
		return static_cast<int>(Nintendo3DSPrivate::RomType::Unknown);
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
			// NOTE: 3dbrew lists up to 64 contents; some DLC packages
			// have significantly more, so we'll allow pu to 256.
			// TODO: Read the TMD content count value and calculate the expected
			// TMD size based on that.
			if (cia_header->cert_chain_size == cpu_to_le32(N3DS_CERT_CHAIN_SIZE) &&
			    le32_to_cpu(cia_header->ticket_size) % 4 == 0 &&
			    (cia_header->ticket_size == cpu_to_le32(sizeof(N3DS_Ticket_t) + 0x4 + 0x200 + 0x3C) ||
			     cia_header->ticket_size == cpu_to_le32(sizeof(N3DS_Ticket_t) + 0x4 + 0x100 + 0x3C) ||
			     cia_header->ticket_size == cpu_to_le32(sizeof(N3DS_Ticket_t) + 0x4 +  0x3C + 0x40)) &&
			    le32_to_cpu(cia_header->tmd_size) % 4 == 0 &&
			    (le32_to_cpu(cia_header->tmd_size) >= (sizeof(N3DS_TMD_Header_t) + 0x4 + 0x3C + 0x40 + sizeof(N3DS_Content_Info_Record_t)*64 + sizeof(N3DS_Content_Chunk_Record_t)) &&
			     le32_to_cpu(cia_header->tmd_size) <= (sizeof(N3DS_TMD_Header_t) + 0x4 + 0x200 + 0x3C + sizeof(N3DS_Content_Info_Record_t)*256 + sizeof(N3DS_Content_Chunk_Record_t)*256)) &&
			    (cia_header->meta_size == cpu_to_le32(0) ||	// no meta
			     cia_header->meta_size == cpu_to_le32(8) ||	// CVer
			     (le32_to_cpu(cia_header->meta_size) % 4 == 0 &&
			      le32_to_cpu(cia_header->meta_size) >= (sizeof(N3DS_SMDH_Header_t) + sizeof(N3DS_SMDH_Icon_t)))))
			{
				// Sizes appear to be valid.
				return static_cast<int>(Nintendo3DSPrivate::RomType::CIA);
			}
		}
	}

	// Check for 3DSX.
	const N3DS_3DSX_Header_t *const _3dsx_header =
		reinterpret_cast<const N3DS_3DSX_Header_t*>(info->header.pData);
	if (_3dsx_header->magic == cpu_to_be32(N3DS_3DSX_HEADER_MAGIC)) {
		// We have a 3DSX file.
		// NOTE: sizeof(N3DS_3DSX_Header_t) includes the
		// extended header, but that's fine, since a .3DSX
		// file with just the standard header and nothing
		// else is rather useless.
		return static_cast<int>(Nintendo3DSPrivate::RomType::_3DSX);
	}

	// Check for CCI/eMMC.
	const N3DS_NCSD_Header_NoSig_t *const ncsd_header =
		reinterpret_cast<const N3DS_NCSD_Header_NoSig_t*>(
			&info->header.pData[N3DS_NCSD_NOSIG_HEADER_ADDRESS]);
	if (ncsd_header->magic == cpu_to_be32(N3DS_NCSD_HEADER_MAGIC)) {
		// TODO: Validate the NCSD image size field?

		// Check if this is an eMMC dump or a CCI image.
		// This is done by checking the eMMC-specific crypt type section.
		// (All zeroes for CCI; minor variance between Old3DS and New3DS.)
		static const uint8_t crypt_cci[8]      = {0,0,0,0,0,0,0,0};
		static const uint8_t crypt_emmc_old[8] = {1,2,2,2,2,0,0,0};
		static const uint8_t crypt_emmc_new[8] = {1,2,2,2,3,0,0,0};
		if (!memcmp(ncsd_header->emmc_part_tbl.crypt_type, crypt_cci, sizeof(crypt_cci))) {
			// CCI image.
			return static_cast<int>(Nintendo3DSPrivate::RomType::CCI);
		} else if (!memcmp(ncsd_header->emmc_part_tbl.crypt_type, crypt_emmc_old, sizeof(crypt_emmc_old)) ||
			   !memcmp(ncsd_header->emmc_part_tbl.crypt_type, crypt_emmc_new, sizeof(crypt_emmc_new))) {
			// eMMC dump.
			// NOTE: Not differentiating between Old3DS and New3DS here.
			return static_cast<int>(Nintendo3DSPrivate::RomType::eMMC);
		} else {
			// Not valid.
			return static_cast<int>(Nintendo3DSPrivate::RomType::Unknown);
		}
	}

	// Check for NCCH.
	const N3DS_NCCH_Header_t *const ncch_header =
		reinterpret_cast<const N3DS_NCCH_Header_t*>(info->header.pData);
	if (ncch_header->hdr.magic == cpu_to_be32(N3DS_NCCH_HEADER_MAGIC)) {
		// Found the NCCH magic.
		// TODO: Other checks?
		return static_cast<int>(Nintendo3DSPrivate::RomType::NCCH);
	}

	// Not supported.
	return static_cast<int>(Nintendo3DSPrivate::RomType::Unknown);
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

	// Nintendo 3DS has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Nintendo3DS::systemName() array index optimization needs to be updated.");

	// 3DS system offset is OR'd with type.
	type &= SYSNAME_TYPE_MASK;

	// Product code.
	// Used to determine if it's *New* Nintendo 3DS exclusive.
	// (KTR instead of CTR)
	NCCHReader *const ncch = const_cast<Nintendo3DSPrivate*>(d)->loadNCCH();
	const N3DS_NCCH_Header_NoSig_t *const ncch_header =
		(ncch && ncch->isOpen() ? ncch->ncchHeader() : nullptr);
	if (ncch_header && ncch_header->product_code[0] == 'K') {
		// *New* Nintendo 3DS exclusive.
		type |= (1U << 2);
	}

	// "iQue" is only used if the localized system name is requested
	// *and* the ROM's region code is China only.
	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
		// SMDH contains a region code bitfield.
		const uint32_t smdhRegion = const_cast<Nintendo3DSPrivate*>(d)->getSMDHRegionCode();
		if (smdhRegion == N3DS_REGION_CHINA) {
			// Chinese exclusive.
			type |= (1U << 3);
		}
	}

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: *New* Nintendo 3DS
	// Bit 3: iQue
	static const char *const sysNames[4*4] = {
		"Nintendo 3DS", "Nintendo 3DS", "3DS", nullptr,
		"*New* Nintendo 3DS", "*New* Nintendo 3DS", "N3DS", nullptr,

		// iQue
		// NOTE: *New* iQue 3DS wasn't actually released...
		"iQue 3DS", "iQue 3DS", "3DS", nullptr,
		"*New* iQue 3DS", "*New* iQue 3DS", "N3DS", nullptr,
	};

	return sysNames[type];
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
	if (d->romType == Nintendo3DSPrivate::RomType::CIA) {
		// TMD needs to be loaded so we can check if it's a DSiWare SRL.
		if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
			const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
		}
		// Is it in fact DSiWare?
		NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
		if (srl) {
			// This is a DSiWare SRL.
			// Get the image information from the underlying SRL.
			return d->mainContent->supportedImageTypes();
		}
	}

	return supportedImageTypes_static();
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Nintendo3DS::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON: {
			// Technically handled by Nintendo3DS_SMDH,
			// but we'll return it here anyway.
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
	return vector<ImageSizeDef>();
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
	ASSERT_imgpf(imageType);

	RP_D(const Nintendo3DS);
	if (d->romType == Nintendo3DSPrivate::RomType::CIA) {
		// TMD needs to be loaded so we can check if it's a DSiWare SRL.
		if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
			const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
		}
		// Is it in fact DSiWare?
		NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
		if (srl) {
			// This is a DSiWare SRL.
			// Get the image information from the underlying SRL.
			return d->mainContent->imgpf(imageType);
		}
	}

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
			// Use nearest-neighbor scaling.
			ret = IMGPF_RESCALE_NEAREST;
			break;
		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Nintendo3DS::loadFieldData(void)
{
	RP_D(Nintendo3DS);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM type.
		return -EIO;
	}

	// TODO: Disambiguate the various NCCHReader pointers.
	// TODO: Split up into smaller functions?
	const char *const s_unknown = C_("RomData", "Unknown");

	// Maximum of 22 fields.
	// Tested with several CCI, CIA, and NCCH files.
	d->fields.reserve(22);

	// Reserve at least 4 tabs:
	// SMDH, NCSD/CIA, ExHeader, Permissions
	d->fields.reserveTabs(4);

	// Have we shown a warning yet?
	bool shownWarning = false;

	// Load headers if we don't already have them.
	if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_SMDH)) {
		d->loadSMDH();
	}
	if ((d->romType == Nintendo3DSPrivate::RomType::CIA) &&
	    !(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD))
	{
		d->loadTicketAndTMD();
	}

	// Get the primary NCCH.
	// If this fails, and the file type is NCSD or CIA,
	// it usually means there's a missing key.
	const NCCHReader *const ncch = d->loadNCCH();

	// Check for potential encryption key errors.
	if (d->romType == Nintendo3DSPrivate::RomType::CCI ||
	    d->romType == Nintendo3DSPrivate::RomType::CIA ||
	    d->romType == Nintendo3DSPrivate::RomType::NCCH)
	{
		if (!ncch) {
			// Unable to open the primary NCCH section.
			if (!shownWarning) {
				d->fields.addField_string(C_("RomData", "Warning"),
					C_("Nintendo3DS", "Unable to open the primary NCCH section."),
					RomFields::STRF_WARNING);
				shownWarning = true;
			}
		} else {
			// SMDH presence indicates this is *not* a DSiWare SRL.
			KeyManager::VerifyResult res = ncch->verifyResult();
			if ((d->headers_loaded & Nintendo3DSPrivate::HEADER_SMDH) &&
			     res != KeyManager::VerifyResult::OK)
			{
				// Missing encryption keys.
				if (!shownWarning) {
					const char *err = KeyManager::verifyResultToString(res);
					if (!err) {
						err = C_("Nintendo3DS", "Unknown error. (THIS IS A BUG!)");
					}
					d->fields.addField_string(C_("RomData", "Warning"),
						err, RomFields::STRF_WARNING);
					shownWarning = true;
				}
			}
		}
	}

	// Load and parse the SMDH header.
	bool haveSeparateSMDHTab = true;
	if (d->headers_loaded & Nintendo3DSPrivate::HEADER_SMDH) {
		d->fields.setTabName(0, "SMDH");
		// Will we end up having a separate SMDH tab?
		if (!(d->headers_loaded & (Nintendo3DSPrivate::HEADER_NCSD | Nintendo3DSPrivate::HEADER_TMD))) {
			// There will only be a single tab.
			// Add the title ID and product code fields here.
			// (Include the content type, if available.)
			haveSeparateSMDHTab = false;
			d->addTitleIdAndProductCodeFields(true);
		}

		// Add the SMDH fields from the Nintendo3DS_SMDH object.
		assert(d->mainContent != nullptr);
		const RomFields *const smdh_fields = d->mainContent->fields();
		assert(smdh_fields != nullptr);
		if (smdh_fields) {
			// Add the SMDH fields.
			d->fields.addFields_romFields(smdh_fields, 0);
		}
	} else if (d->mainContent) {
		// DSiWare SRL.
		const RomFields *const srl_fields = d->mainContent->fields();
		assert(srl_fields != nullptr);
		if (srl_fields) {
			d->fields.setTabName(0, C_("Nintendo3DS", "DSiWare"));

			// Will we end up having a separate DSiWare tab?
			if (!(d->headers_loaded & (Nintendo3DSPrivate::HEADER_NCSD | Nintendo3DSPrivate::HEADER_TMD))) {
				// There will only be a single tab.
				// Add the title ID and product code fields here.
				// (Include the content type, if available.)
				haveSeparateSMDHTab = false;
				d->addTitleIdAndProductCodeFields(true);
			}

			// Do we have additional tabs?
			// TODO: Combine "DSiWare" (tab 0) and "DSi" (tab 1)?
			const int subtab_count = srl_fields->tabCount();
			if (subtab_count > 1) {
				for (int subtab = 1; subtab < subtab_count; subtab++) {
					d->fields.setTabName(subtab, srl_fields->tabName(subtab));
				}
			}

			// Add the DSiWare fields.
			d->fields.addFields_romFields(srl_fields, 0);
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
		bool addTID = false;
		if (haveSeparateSMDHTab) {
			d->fields.addTab("NCSD");
			addTID = true;
		} else {
			d->fields.setTabName(0, "NCSD");
		}

		if (!ncch) {
			// Unable to open the primary NCCH section.
			if (!shownWarning) {
				d->fields.addField_string(C_("RomData", "Warning"),
					C_("Nintendo3DS", "Unable to open the primary NCCH section."),
					RomFields::STRF_WARNING);
				shownWarning = true;
			}
		} else if (ncch->verifyResult() != KeyManager::VerifyResult::OK) {
			// Missing encryption keys.
			// TODO: This warning probably isn't needed,
			// since it's handled above...
			if (!shownWarning) {
				KeyManager::VerifyResult res = (ncch
					? ncch->verifyResult()
					: KeyManager::VerifyResult::Unknown);
				const char *err = KeyManager::verifyResultToString(res);
				if (!err) {
					err = C_("Nintendo3DS", "Unknown error. (THIS IS A BUG!)");
				}
				d->fields.addField_string(C_("RomData", "Warning"),
					err, RomFields::STRF_WARNING);
				shownWarning = true;
			}
		} else if (ncch->isForceNoCrypto()) {
			// NCSD is decrypted but has incorrect encryption flags.
			// TODO: Show in the SMDH tab if it's visible?
			if (!shownWarning) {
				d->fields.addField_string(C_("RomData", "Warning"),
					C_("Nintendo3DS", "NCCH encryption flags are incorrect. Use GodMode9 to fix."),
                                       RomFields::STRF_WARNING);
				shownWarning = true;
			}
		}

		if (addTID) {
			// Add the title ID and product code fields here.
			// (Content type is listed in the NCSD partition table.)
			d->addTitleIdAndProductCodeFields(false);
		}

		// TODO: Add more fields?
		const N3DS_NCSD_Header_NoSig_t *const ncsd_header = &d->mxh.ncsd_header;

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
		vector<string> *v_partitions_names;
		if (d->romType != Nintendo3DSPrivate::RomType::eMMC) {
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
			// Old3DS uses encryption type 2 for CTR NAND.
			// New3DS uses encryption type 3 for CTR NAND.
			const bool new3ds = (ncsd_header->emmc_part_tbl.crypt_type[4] == 3);
			d->fields.addField_string(C_("Nintendo3DS|eMMC", "Type"),
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

		if (d->romType == Nintendo3DSPrivate::RomType::CCI) {
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
			const char *const media_type_title = C_("Nintendo3DS", "Media Type");
			if (media_type < ARRAY_SIZE(media_type_tbl)) {
				d->fields.addField_string(media_type_title,
					media_type_tbl[media_type]);
			} else {
				d->fields.addField_string(media_type_title,
					rp_sprintf(C_("RomData", "Unknown (0x%02X)"), media_type));
			}

			if (ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_TYPE_INDEX] == N3DS_NCSD_MEDIA_TYPE_CARD2) {
				// Card2 writable address.
				d->fields.addField_string_numeric(C_("Nintendo3DS", "Card2 RW Address"),
					le32_to_cpu(cinfo_header->card2_writable_address),
					RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
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
			const char *const card_device_title = C_("Nintendo3DS", "Card Device");
			if (card_dev_id >= 1 && card_dev_id < ARRAY_SIZE(card_dev_tbl)) {
				d->fields.addField_string(card_device_title,
					dpgettext_expr(RP_I18N_DOMAIN, "Nintendo3DS|CDev", card_dev_tbl[card_dev_id]));
			} else {
				d->fields.addField_string(card_device_title,
					rp_sprintf_p(C_("Nintendo3DS|CDev", "Unknown (SDK2=0x%1$02X, SDK3=0x%2$02X)"),
						ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK2],
						ncsd_header->cci.partition_flags[N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK3]));
			}

			// Card revision.
			d->fields.addField_string_numeric(C_("Nintendo3DS", "Card Revision"),
				le32_to_cpu(cinfo_header->card_revision),
				RomFields::Base::Dec, 2);

			// TODO: Show "title version"?

#ifdef ENABLE_DECRYPTION
			// Also show encryption type.
			// TODO: Show a warning if `ncch` is NULL?
			if (ncch) {
				d->fields.addField_string(C_("Nintendo3DS", "Issuer"),
					ncch->isDebug()
						? C_("Nintendo3DS", "Debug")
						: C_("Nintendo3DS", "Retail"));
			}
#endif /* ENABLE_DECRYPTION */
		}

		// Partition table.
		auto vv_partitions = new RomFields::ListData_t();
		vv_partitions->reserve(8);

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

			const size_t vidx = vv_partitions->size();
			vv_partitions->resize(vidx+1);
			auto &data_row = vv_partitions->at(vidx);
			data_row.reserve(5);

			// Partition number.
			data_row.emplace_back(rp_sprintf("%u", i));

			// Partition type.
			// TODO: Use the partition ID to determine the type?
			const char *const s_ptype = (pt_types[i] ? pt_types[i] : s_unknown);
			data_row.emplace_back(s_ptype);

			if (d->romType != Nintendo3DSPrivate::RomType::eMMC) {
				const N3DS_NCCH_Header_NoSig_t *const part_ncch_header =
					(pNcch && pNcch->isOpen() ? pNcch->ncchHeader() : nullptr);
				if (part_ncch_header) {
					// Encryption.
					NCCHReader::CryptoType cryptoType = {nullptr, false, 0, false};
					ret = NCCHReader::cryptoType_static(&cryptoType, part_ncch_header);
					if (ret != 0 || !cryptoType.encrypted || cryptoType.keyslot >= 0x40) {
						// Not encrypted, or not using a predefined keyslot.
						if (cryptoType.name) {
							data_row.emplace_back(latin1_to_utf8(cryptoType.name, -1));
						} else {
							data_row.emplace_back(s_unknown);
						}
					} else {
						// TODO: Show an error if this should be NoCrypto.
						// This is detected for the main NCCH in the initial
						// NCSD check, but not here...
						data_row.emplace_back(rp_sprintf("%s%s (0x%02X)",
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
						data_row.emplace_back("1.x.x");
					} else {
						data_row.emplace_back(d->n3dsVersionToString(version));
					}
				} else {
					// Unable to load the NCCH header.
					data_row.emplace_back(s_unknown);	// Encryption
					data_row.emplace_back(s_unknown);	// Version
				}
			}

			if (keyslots) {
				// Keyslot.
				data_row.emplace_back(rp_sprintf("0x%02X", keyslots[i]));
			}

			// Partition size.
			const off64_t length_bytes = static_cast<off64_t>(length) << d->media_unit_shift;
			data_row.emplace_back(LibRpText::formatFileSize(length_bytes));

			UNREF(pNcch);
		}

		// Add the partitions list data.
		RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 0);
		params.headers = v_partitions_names;
		params.data.single = vv_partitions;
		d->fields.addField_listData(C_("Nintendo3DS", "Partitions"), &params);
	}

	// Is the TMD header loaded?
	if (d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD) {
		// Display the TMD header.
		// NOTE: This is usually for CIAs only.
		if (haveSeparateSMDHTab) {
			d->fields.addTab("CIA");
			// Add the title ID and product code fields here.
			// (Content type is listed in the CIA contents table.)
			d->addTitleIdAndProductCodeFields(false);
		} else {
			d->fields.setTabName(0, "CIA");
		}

		// TODO: Add more fields?
		const N3DS_TMD_Header_t *const tmd_header = &d->mxh.tmd_header;

		// TODO: Required system version?

		// Version.
		d->fields.addField_string(C_("Nintendo3DS", "Version"),
			d->n3dsVersionToString(be16_to_cpu(tmd_header->title_version)));

		// Issuer.
		// NOTE: We're using the Ticket Issuer in the TMD tab.
		// TODO: Verify that Ticket and TMD issuers match?
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

		const char *const issuer_title = C_("Nintendo3DS", "Issuer");
		if (issuer) {
			// tr: Ticket issuer. (retail or debug)
			d->fields.addField_string(issuer_title, issuer);
		} else {
			// Unknown issuer. Print it as-is.
			d->fields.addField_string(issuer_title,
				latin1_to_utf8(d->mxh.ticket.issuer, sizeof(d->mxh.ticket.issuer)));
		}

		// Demo use limit.
		if (d->mxh.ticket.limits[0] == cpu_to_be32(4)) {
			// Title has use limits.
			d->fields.addField_string_numeric(C_("Nintendo3DS", "Demo Use Limit"),
				be32_to_cpu(d->mxh.ticket.limits[1]));
		}

		// Console ID.
		// NOTE: Technically part of the ticket.
		// NOTE: Not including the "0x" hex prefix.
		d->fields.addField_string(C_("Nintendo3DS", "Console ID"),
			rp_sprintf("%08X", be32_to_cpu(d->mxh.ticket.console_id)),
			RomFields::STRF_MONOSPACE);

		// Contents table.
		auto vv_contents = new RomFields::ListData_t();
		vv_contents->reserve(d->content_chunks.size());

		// Process the contents.
		// TODO: Content types?
		unsigned int i = 0;
		const auto content_chunks_cend = d->content_chunks.cend();
		for (auto iter = d->content_chunks.cbegin();
		     iter != content_chunks_cend; ++iter, ++i)
		{
			// Make sure the content exists first.
			NCCHReader *pNcch = nullptr;
			int ret = d->loadNCCH(i, &pNcch);
			if (ret == -ENOENT)
				continue;

			const size_t vidx = vv_contents->size();
			vv_contents->resize(vidx+1);
			auto &data_row = vv_contents->at(vidx);
			data_row.reserve(5);

			// Content index
			data_row.emplace_back(rp_sprintf("%u", i));

			// TODO: Use content_chunk->index?
			const N3DS_NCCH_Header_NoSig_t *content_ncch_header = nullptr;
			const char *content_type = nullptr;
			if (pNcch) {
				if (pNcch->isOpen()) {
					content_ncch_header = pNcch->ncchHeader();
				}
				// Get the content type regardless of whether or not
				// the NCCH is open, since it might be a non-NCCH
				// content that we still recognize.
				content_type = pNcch->contentType();
			}
			if (!content_ncch_header) {
				// Invalid content index, or this content isn't an NCCH.
				// TODO: Are there CIAs with discontiguous content indexes?
				// (Themes, DLC...)
				const char *crypto = nullptr;
				if (iter->type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED)) {
					// CIA encryption
					crypto = "CIA";
				}

				if (i == 0 && d->mainContent) {
					// This is an SRL.
					if (!content_type) {
						content_type = "SRL";
					}
					// TODO: Do SRLs have encryption besides CIA encryption?
					if (!crypto) {
						crypto = "NoCrypto";
					}
				} else {
					// Something else...
					if (!content_type) {
						content_type = s_unknown;
					}
				}
				data_row.emplace_back(content_type);

				// Encryption
				data_row.emplace_back(crypto ? crypto : s_unknown);
				// Version
				data_row.emplace_back();

				// Content size
				data_row.emplace_back(LibRpText::formatFileSize(be64_to_cpu(iter->size)));
				UNREF(pNcch);
				continue;
			}

			// Content type
			data_row.emplace_back(content_type ? content_type : s_unknown);

			// Encryption
			NCCHReader::CryptoType cryptoType;
			bool isCIAcrypto = !!(iter->type & cpu_to_be16(N3DS_CONTENT_CHUNK_ENCRYPTED));
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
					data_row.emplace_back(latin1_to_utf8(cryptoType.name, -1));
				} else {
					data_row.emplace_back(s_unknown);
				}
			} else {
				// Encrypted.
				data_row.emplace_back(rp_sprintf("%s%s%s (0x%02X)",
					(isCIAcrypto ? "CIA+" : ""),
					(cryptoType.name ? cryptoType.name : s_unknown),
					(cryptoType.seed ? "+Seed" : ""),
					cryptoType.keyslot));
			}

			// Version [FIXME: Might not be right...]
			data_row.emplace_back(d->n3dsVersionToString(
				le16_to_cpu(content_ncch_header->version)));

			// Content size
			data_row.emplace_back(LibRpText::formatFileSize(pNcch->partition_size()));

			UNREF(pNcch);
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

		RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 0);
		params.headers = v_contents_names;
		params.data.single = vv_contents;
		d->fields.addField_listData(C_("Nintendo3DS", "Contents"), &params);
	}

	// Get the NCCH Extended Header.
	const N3DS_NCCH_ExHeader_t *const ncch_exheader =
		(ncch && ncch->isOpen() ? ncch->ncchExHeader() : nullptr);
	if (ncch_exheader) {
		// Display the NCCH Extended Header.
		// TODO: Add more fields?
		d->fields.addTab("ExHeader");

		// Process name.
		d->fields.addField_string(C_("Nintendo3DS", "Process Name"),
			latin1_to_utf8(ncch_exheader->sci.title, sizeof(ncch_exheader->sci.title)));

		// Application type. (resource limit category)
		static const char appl_type_tbl[4][16] = {
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_APPLICATION
			NOP_C_("Nintendo3DS|ApplType", "Application"),
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_SYS_APPLET
			NOP_C_("Nintendo3DS|ApplType", "System Applet"),
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_LIB_APPLET
			NOP_C_("Nintendo3DS|ApplType", "Library Applet"),
			// tr: N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_OTHER
			NOP_C_("Nintendo3DS|ApplType", "SysModule"),
		};
		const char *const type_title = C_("Nintendo3DS", "Type");
		const uint8_t appl_type = ncch_exheader->aci.arm11_local.res_limit_category;
		if (appl_type < ARRAY_SIZE(appl_type_tbl)) {
			d->fields.addField_string(type_title,
				dpgettext_expr(RP_I18N_DOMAIN, "Nintendo3DS|ApplType", appl_type_tbl[appl_type]));
		} else {
			d->fields.addField_string(type_title,
				rp_sprintf(C_("Nintendo3DS", "Invalid (0x%02X)"), appl_type));
		}

		// Flags.
		static const char *const exheader_flags_names[] = {
			"CompressExefsCode", "SDApplication"
		};
		vector<string> *const v_exheader_flags_names = RomFields::strArrayToVector(
			exheader_flags_names, ARRAY_SIZE(exheader_flags_names));
		d->fields.addField_bitfield("Flags",
			v_exheader_flags_names, 0, ncch_exheader->sci.flags);

		// TODO: Figure out what "Core Version" is.

		// System Mode struct.
		typedef struct _ModeTbl_t {
			char name[7];	// Mode name.
			uint8_t mb;	// RAM allocation, in megabytes.
		} ModeTbl_t;
		ASSERT_STRUCT(ModeTbl_t, 8);

		// Old3DS System Mode.
		// NOTE: Mode names are NOT translatable!
		static const ModeTbl_t old3ds_sys_mode_tbl[6] = {
			{"Prod", 64},	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Prod
			{"", 0},
			{"Dev1", 96},	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev1
			{"Dev2", 80},	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev2
			{"Dev3", 72},	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev3
			{"Dev4", 32},	// N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev4
		};
		const char *const old3ds_sys_mode_title = C_("Nintendo3DS", "Old3DS Sys Mode");
		const uint8_t old3ds_sys_mode = (ncch_exheader->aci.arm11_local.flags[2] &
			N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Mask) >> 4;
		if (old3ds_sys_mode < ARRAY_SIZE(old3ds_sys_mode_tbl) &&
		    old3ds_sys_mode_tbl[old3ds_sys_mode].name[0] != '\0')
		{
			const auto &ptbl = &old3ds_sys_mode_tbl[old3ds_sys_mode];
			d->fields.addField_string(old3ds_sys_mode_title,
				// tr: %1$s == Old3DS system mode; %2$u == RAM allocation, in megabytes
				rp_sprintf_p(C_("Nintendo3DS", "%1$s (%2$u MiB)"), ptbl->name, ptbl->mb));
		} else {
			d->fields.addField_string(old3ds_sys_mode_title,
				rp_sprintf(C_("Nintendo3DS", "Invalid (0x%02X)"), old3ds_sys_mode));
		}

		// New3DS System Mode.
		// NOTE: Mode names are NOT translatable!
		static const ModeTbl_t new3ds_sys_mode_tbl[4] = {
			{"Legacy", 64},	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Legacy
			{"Prod",  124},	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Prod
			{"Dev1",  178},	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Dev1
			{"Dev2",  124},	// N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Dev2
		};
		const char *const new3ds_sys_mode_title = C_("Nintendo3DS", "New3DS Sys Mode");
		const uint8_t new3ds_sys_mode = ncch_exheader->aci.arm11_local.flags[1] &
			N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Mask;
		if (new3ds_sys_mode < ARRAY_SIZE(new3ds_sys_mode_tbl)) {
			const auto &ptbl = &new3ds_sys_mode_tbl[new3ds_sys_mode];
			d->fields.addField_string(new3ds_sys_mode_title,
				// tr: %1$s == New3DS system mode; %2$u == RAM allocation, in megabytes
				rp_sprintf_p(C_("Nintendo3DS", "%1$s (%2$u MiB)"), ptbl->name, ptbl->mb));
		} else {
			d->fields.addField_string(new3ds_sys_mode_title,
				rp_sprintf(C_("Nintendo3DS", "Invalid (0x%02X)"), new3ds_sys_mode));
		}

		// New3DS CPU Mode.
		static const char *const new3ds_cpu_mode_names[] = {
			NOP_C_("Nintendo3DS|N3DSCPUMode", "L2 Cache"),
			NOP_C_("Nintendo3DS|N3DSCPUMode", "804 MHz"),
		};
		vector<string> *const v_new3ds_cpu_mode_names = RomFields::strArrayToVector_i18n(
			"Nintendo3DS|N3DSCPUMode", new3ds_cpu_mode_names, ARRAY_SIZE(new3ds_cpu_mode_names));
		d->fields.addField_bitfield("New3DS CPU Mode",
			v_new3ds_cpu_mode_names, 0, ncch_exheader->aci.arm11_local.flags[0]);

		// TODO: Ideal CPU and affinity mask.
		// TODO: core_version is probably specified for e.g. AGB.
		// Indicate that somehow.

		// Permissions. These are technically part of the
		// ExHeader, but we're using a separate tab because
		// there's a lot of them.
		d->fields.addTab(C_("Nintendo3DS", "Permissions"));
		d->addFields_permissions();
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Nintendo3DS::loadMetaData(void)
{
	RP_D(Nintendo3DS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Check for SMDH.
	int ret = const_cast<Nintendo3DSPrivate*>(d)->loadSMDH();
	if (ret != 0) {
		// Check for DSiWare.
		ret = const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
	}

	if (ret == 0 && d->mainContent != nullptr) {
		// Add the metadata.
		d->metaData = new RomMetaData();
		d->metaData->addMetaData_metaData(d->mainContent->metaData());
	}

	// Finished reading the metadata.
	return (d->metaData ? static_cast<int>(d->metaData->count()) : 0);
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
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(Nintendo3DS);
	if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	switch (d->romType) {
		default:
		case Nintendo3DSPrivate::RomType::Unknown:
		case Nintendo3DSPrivate::RomType::eMMC:
			// Cannot get external images for eMMC and unknown ROM types.
			*pImage = nullptr;
			return -ENOENT;

		case Nintendo3DSPrivate::RomType::CIA:
			// TMD needs to be loaded so we can check if it's a DSiWare SRL.
			if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
				d->loadTicketAndTMD();
			}
			break;

		case Nintendo3DSPrivate::RomType::_3DSX:
		case Nintendo3DSPrivate::RomType::CCI:
		case Nintendo3DSPrivate::RomType::NCCH:
			// Internal images are available.
			break;
	}

	// TODO: Specify the icon index.

	// Make sure the SMDH section is loaded.
	d->loadSMDH();

	if (!d->mainContent) {
		// No main content...
		*pImage = nullptr;
		return -ENOENT;
	}

	// Get the icon from the main content.
	const rp_image *image = d->mainContent->image(imageType);
	*pImage = image;
	return (image ? 0 : -EIO);
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * The retrieved IconAnimData must be ref()'d by the caller if the
 * caller stores it instead of using it immediately.
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
	if (d->mainContent) {
		return d->mainContent->iconAnimData();
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
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const Nintendo3DS);
	if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	switch (d->romType) {
		default:
		case Nintendo3DSPrivate::RomType::Unknown:
		case Nintendo3DSPrivate::RomType::eMMC:
		case Nintendo3DSPrivate::RomType::_3DSX:
			// Cannot get external images for eMMC, 3DSX, and unknown ROM types.
			return -ENOENT;

		case Nintendo3DSPrivate::RomType::CIA: {
			// TMD needs to be loaded so we can check if it's a DSiWare SRL.
			if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
				const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
			}
			// Check for a DSiWare SRL.
			NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
			if (srl) {
				// This is a DSiWare SRL.
				// Get the image URLs from the underlying SRL.
				return srl->extURLs(imageType, pExtURLs, size);
			}
			// Assume it's a regular 3DS CIA which has external images.
			break;
		}

		case Nintendo3DSPrivate::RomType::CCI:
		case Nintendo3DSPrivate::RomType::NCCH:
			// External images are available.
			break;
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
	// NOTE: We're checking for NULL termination above.
	const char *const id4 = &ncch_header->product_code[6];
	for (int i = 3; i >= 0; i--) {
		if (!ISPRINT(id4[i])) {
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

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *const sizeDef = d->selectBestSize(sizeDefs, size);
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

	// SMDH contains a region code bitfield.
	const uint32_t smdhRegion = const_cast<Nintendo3DSPrivate*>(d)->getSMDHRegionCode();

	// Determine the GameTDB language code(s).
	const vector<uint16_t> tdb_lc = d->n3dsRegionToGameTDB(smdhRegion, id4[3]);

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
	pExtURLs->resize(szdef_count * tdb_lc.size());
	auto extURL_iter = pExtURLs->begin();
	const auto tdb_lc_cend = tdb_lc.cend();
	for (unsigned int i = 0; i < szdef_count; i++) {
		// Current image type.
		char imageTypeName[16];
		snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
			 imageTypeName_base, (szdefs_dl[i]->name ? szdefs_dl[i]->name : ""));

		// Add the images.
		for (auto tdb_iter = tdb_lc.cbegin();
		     tdb_iter != tdb_lc_cend; ++tdb_iter, ++extURL_iter)
		{
			const string lc_str = SystemRegion::lcToStringUpper(*tdb_iter);
			extURL_iter->url = d->getURL_GameTDB("3ds", imageTypeName, lc_str.c_str(), id4, ext);
			extURL_iter->cache_key = d->getCacheKey_GameTDB("3ds", imageTypeName, lc_str.c_str(), id4, ext);
			extURL_iter->width = szdefs_dl[i]->width;
			extURL_iter->height = szdefs_dl[i]->height;
			extURL_iter->high_res = (szdefs_dl[i]->index >= 2);
		}
	}

	// All URLs added.
	return 0;
}

/**
 * Does this ROM image have "dangerous" permissions?
 *
 * @return True if the ROM image has "dangerous" permissions; false if not.
 */
bool Nintendo3DS::hasDangerousPermissions(void) const
{
	RP_D(const Nintendo3DS);

	// Check for DSiWare.
	int ret = const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
	if (ret == 0) {
		// Is it in fact DSiWare?
		NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
		if (srl) {
			// DSiWare: Check DSi permissions.
			return d->mainContent->hasDangerousPermissions();
		}
	}

	// Load permissions.
	ret = const_cast<Nintendo3DSPrivate*>(d)->loadPermissions();
	if (ret != 0) {
		// Can't load permissions.
		return false;
	}

	return d->perm.isDangerous;
}

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int Nintendo3DS::checkViewedAchievements(void) const
{
	RP_D(const Nintendo3DS);
	if (!d->isValid) {
		// ROM is not valid.
		return 0;
	}

#ifdef ENABLE_DECRYPTION
	// NCCH header.
	NCCHReader *const ncch = const_cast<Nintendo3DSPrivate*>(d)->loadNCCH();
	if (!ncch) {
		// Cannot load the NCCH.
		return 0;
	}

	Achievements *const pAch = Achievements::instance();
	int ret = 0;

	// If a TMD is present, check the TMD issuer first.
	if (const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD() == 0) {
		if (!strncmp(d->mxh.ticket.issuer, N3DS_TICKET_ISSUER_DEBUG, sizeof(d->mxh.ticket.issuer))) {
			// Debug issuer.
			pAch->unlock(Achievements::ID::ViewedDebugCryptedFile);
			ret++;
		}
	} else {
		// Check the NCCH encryption.
		if (ncch->isDebug()) {
			// Debug encryption.
			pAch->unlock(Achievements::ID::ViewedDebugCryptedFile);
			ret++;
		}
	}

	return ret;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not available. Cannot check.
	return 0;
#endif /* ENABLE_DECRYPTION */
}

}
