/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiTMD.hpp: Nintendo Wii (and Wii U) title metadata reader.             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiTMD.hpp"
#include "wii_structs.h"
#include "wiiu_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

class WiiTMDPrivate final : public RomDataPrivate
{
public:
	WiiTMDPrivate(const IRpFilePtr &file);
	~WiiTMDPrivate();

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiTMDPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// TMD header
	RVL_TMD_Header tmdHeader;

	// TMD v1: CMD group header
	WUP_CMD_GroupHeader *cmdGroupHeader;

public:
	/**
	 * Load the CMD group header. (TMD v1)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadCmdGroupHeader(void);
};

ROMDATA_IMPL(WiiTMD)

/** WiiTMDPrivate **/

/* RomDataInfo */
const char *const WiiTMDPrivate::exts[] = {
	".tmd",

	nullptr
};
const char *const WiiTMDPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-tmd",

	nullptr
};
const RomDataInfo WiiTMDPrivate::romDataInfo = {
	"WiiTMD", exts, mimeTypes
};

WiiTMDPrivate::WiiTMDPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, cmdGroupHeader(nullptr)
{
	// Clear the TMD header struct.
	memset(&tmdHeader, 0, sizeof(tmdHeader));
}

WiiTMDPrivate::~WiiTMDPrivate()
{
	delete cmdGroupHeader;
}

/**
 * Load the CMD group header. (TMD v1)
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiTMDPrivate::loadCmdGroupHeader(void)
{
	if (this->cmdGroupHeader) {
		// CMD group header is already loaded.
		return 0;
	}

	// File must be open.
	if (!isValid || !file || !file->isOpen())
		return -EIO;

	// This TMD must be v1.
	assert(tmdHeader.tmd_format_version == 1);
	if (tmdHeader.tmd_format_version != 1) {
		// Incorrect TMD version.
		return -EINVAL;
	}

	WUP_CMD_GroupHeader *const grpHdr = new WUP_CMD_GroupHeader;
	size_t size = file->seekAndRead(sizeof(RVL_TMD_Header), grpHdr, sizeof(*grpHdr));
	if (size != sizeof(*grpHdr)) {
		// Seek and/or read error.
		delete grpHdr;
		return -EIO;
	}

	this->cmdGroupHeader = grpHdr;
	return 0;
}

/** WiiTMD **/

/**
 * Read a Nintendo Wii (or Wii U) ticket file. (.tik)
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
WiiTMD::WiiTMD(const IRpFilePtr &file)
	: super(new WiiTMDPrivate(file))
{
	RP_D(WiiTMD);
	d->mimeType = WiiTMDPrivate::mimeTypes[0];	// unofficial
	d->fileType = FileType::MetadataFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ticket. (either v0 or v1, depending on how much was read)
	d->file->rewind();
	size_t size = d->file->read(&d->tmdHeader, sizeof(d->tmdHeader));
	if (size != sizeof(_RVL_TMD_Header)) {
		// Ticket is too small.
		d->file.reset();
		return;
	}

	// Check if this ticket is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(d->tmdHeader), reinterpret_cast<const uint8_t*>(&d->tmdHeader)},
		FileSystem::file_ext(filename),	// ext
		d->file->size()			// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiTMD::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->ext || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(RVL_TMD_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// NOTE: File extension must match.
	bool ok = false;
	for (const char *const *ext = WiiTMDPrivate::exts;
	     *ext != nullptr; ext++)
	{
		if (!strcasecmp(info->ext, *ext)) {
			// File extension is supported.
			ok = true;
			break;
		}
	}
	if (!ok) {
		// File extension doesn't match.
		return -1;
	}

	// Compare the TMD version to the file size.
	const RVL_TMD_Header *const tmdHeader = reinterpret_cast<const RVL_TMD_Header*>(info->header.pData);
	switch (tmdHeader->tmd_format_version) {
		default:
			// Unsupported ticket version.
			return -1;
		case 0:
			// TODO: Calculate the actual CMD size.
			if (info->szFile < static_cast<off64_t>(
				sizeof(RVL_TMD_Header) +
				sizeof(RVL_Content_Entry)))
			{
				// Incorrect file size.
				return -1;
			}
			break;
		case 1:
			// TODO: Calculate the actual CMD size.
			if (info->szFile < static_cast<off64_t>(
				sizeof(RVL_TMD_Header) +
				sizeof(WUP_CMD_GroupHeader) +
				sizeof(WUP_CMD_GroupEntry) +
				sizeof(WUP_Content_Entry)))
			{
				// Incorrect file size.
				// TODO: Allow larger tickets?
				return -1;
			}
			break;
	}

	// Validate the ticket signature format.
	switch (be32_to_cpu(tmdHeader->signature_type)) {
		default:
			// Unsupported signature format.
			return -1;
		case RVL_CERT_SIGTYPE_RSA2048_SHA1:
			// RSA-2048 with SHA-1 (Wii, DSi)
			break;
		case WUP_CERT_SIGTYPE_RSA2048_SHA256:
		case WUP_CERT_SIGTYPE_RSA2048_SHA256 | WUP_CERT_SIGTYPE_FLAG_DISC:
			// RSA-2048 with SHA-256 (Wii U, 3DS)
			// NOTE: Requires TMD format v1 or later.
			if (tmdHeader->tmd_format_version < 1)
				return -1;
			break;
	}

	// Certificate issuer must start with "Root-".
	if (memcmp(tmdHeader->signature_issuer, "Root-", 5) != 0) {
		// Incorrect issuer.
		return -1;
	}

	// This appears to be a valid Nintendo title metadata.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiTMD::systemName(unsigned int type) const
{
	RP_D(const WiiTMD);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Abbreviation might be different... (Japan uses AGB?)
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiTMD::systemName() array index optimization needs to be updated.");

	// Use the title ID to determine the system.
	static const char *const sysNames[8][4] = {
		{"Nintendo Wii", "Wii", "Wii", nullptr},	// Wii IOS
		{"Nintendo Wii", "Wii", "Wii", nullptr},	// Wii
		{"GBA NetCard", "NetCard", "NetCard", nullptr},	// GBA NetCard
		{"Nintendo DSi", "DSi", "DSi", nullptr},	// DSi
		{"Nintendo 3DS", "3DS", "3DS", nullptr},	// 3DS
		{"Nintendo Wii U", "Wii U", "Wii U", nullptr},	// Wii U
		{nullptr, nullptr, nullptr, nullptr},		// unused
		{"Nintendo Wii U", "Wii U", "Wii U", nullptr},	// Wii U (vWii)
	};

	const unsigned int sysID = be16_to_cpu(d->tmdHeader.title_id.sysID);
	return (likely(sysID < ARRAY_SIZE(sysNames)))
		? sysNames[sysID][type & SYSNAME_TYPE_MASK]
		: nullptr;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiTMD::loadFieldData(void)
{
	RP_D(WiiTMD);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// TMD isn't valid.
		return -EIO;
	}

	// TMD header is read in the constructor.
	const RVL_TMD_Header *const tmdHeader = &d->tmdHeader;
	d->fields.reserve(4);	// Maximum of 4 fields.

	// Title ID
	char s_title_id[24];
	snprintf(s_title_id, sizeof(s_title_id), "%08X-%08X",
		be32_to_cpu(tmdHeader->title_id.hi),
		be32_to_cpu(tmdHeader->title_id.lo));
	d->fields.addField_string(C_("Nintendo", "Title ID"), s_title_id, RomFields::STRF_MONOSPACE);

	// Issuer
	d->fields.addField_string(C_("Nintendo", "Issuer"),
		latin1_to_utf8(tmdHeader->signature_issuer, sizeof(tmdHeader->signature_issuer)),
		RomFields::STRF_MONOSPACE | RomFields::STRF_TRIM_END);

	// Title version
	// TODO: Might be different on 3DS?
	const unsigned int title_version = be16_to_cpu(tmdHeader->title_version);
	d->fields.addField_string(C_("Nintendo", "Title Version"),
		rp_sprintf("%u.%u (v%u)", title_version >> 8, title_version & 0xFF, title_version));

	// OS version (if non-zero)
	const Nintendo_TitleID_BE_t os_tid = tmdHeader->sys_version;
	const unsigned int sysID = be16_to_cpu(os_tid.sysID);
	if (os_tid.id != 0) {
		// OS display depends on the system ID.
		char buf[24];
		buf[0] = '\0';

		switch (sysID) {
			default:
				break;

			case NINTENDO_SYSID_IOS: {
				// Wii (IOS)
				if (be32_to_cpu(os_tid.hi) != 1)
					break;

				// IOS slots
				const uint32_t tid_lo = be32_to_cpu(os_tid.lo);
				switch (tid_lo) {
					case 1:
						strcpy(buf, "boot2");
						break;
					case 2:
						// TODO: Localize this?
						strcpy(buf, "System Menu");
						break;
					case 256:
						strcpy(buf, "BC");
						break;
					case 257:
						strcpy(buf, "MIOS");
						break;
					case 512:
						strcpy(buf, "BC-NAND");
						break;
					case 513:
						strcpy(buf, "BC-WFS");
						break;
					default:
						if (tid_lo < 256) {
							snprintf(buf, sizeof(buf), "IOS%u", tid_lo);
						}
						break;
				}
				break;
			}

			case NINTENDO_SYSID_WUP: {
				// Wii U (IOSU)
				// TODO: Add pre-release versions.
				if (be32_to_cpu(os_tid.hi) != 0x00050010)
					break;

				const uint32_t tid_lo = be32_to_cpu(os_tid.lo);
				if ((tid_lo & 0xFFFF3F00) != 0x10000000) {
					// Not an IOSU title.
					// tid_lo should be:
					// - 0x100040xx for NDEBUG
					// - 0x100080xx for DEBUG
					break;
				}

				const unsigned int debug_flag = (tid_lo & 0xC000);
				if (debug_flag != 0x4000 && debug_flag != 0x8000) {
					// Incorrect debug flag.
					break;
				}

				snprintf(buf, sizeof(buf), "OSv%u %s", (tid_lo & 0xFF),
					(likely(debug_flag == 0x4000)) ? "NDEBUG" : "DEBUG");
				break;
			}
		}

		if (unlikely(buf[0] == '\0')) {
			// Print the OS title ID.
			snprintf(buf, sizeof(buf), "%08X-%08X",
				be32_to_cpu(os_tid.hi),
				be32_to_cpu(os_tid.lo));
		}
		d->fields.addField_string(C_("RomData", "OS Version"), buf);
	}

	// Access rights
	if (sysID == NINTENDO_SYSID_RVL || sysID == NINTENDO_SYSID_WUP) {
		vector<string> *const v_access_rights_hdr = new vector<string>();
		v_access_rights_hdr->reserve(2);
		v_access_rights_hdr->emplace_back("AHBPROT");
		v_access_rights_hdr->emplace_back(C_("Wii", "DVD Video"));
		d->fields.addField_bitfield(C_("Wii", "Access Rights"),
			v_access_rights_hdr, 0, be32_to_cpu(tmdHeader->access_rights));
	}

	// TODO: Region code, if available?

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int WiiTMD::loadMetaData(void)
{
	RP_D(WiiTMD);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// TMD isn't valid.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// TMD header is read in the constructor.
	const RVL_TMD_Header *const tmdHeader = &d->tmdHeader;

	// Title ID (using as Title)
	char s_title_id[24];
	snprintf(s_title_id, sizeof(s_title_id), "%08X-%08X",
		be32_to_cpu(tmdHeader->title_id.hi),
		be32_to_cpu(tmdHeader->title_id.lo));
	d->metaData->addMetaData_string(Property::Title, s_title_id);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/** TMD accessors **/

/**
 * Get the TMD format version.
 * @return TMD format version
 */
unsigned int WiiTMD::tmdFormatVersion(void) const
{
	RP_D(const WiiTMD);
	return (likely(d->isValid)) ? d->tmdHeader.tmd_format_version : 0;
}

/**
 * Get the TMD header.
 * @return TMD header
 */
const RVL_TMD_Header *WiiTMD::tmdHeader(void) const
{
	RP_D(const WiiTMD);
	return (likely(d->isValid)) ? &d->tmdHeader : nullptr;
}

/**
 * Get the number of content metadata groups. (for TMD v1)
 * @return Number of content metadata groups, or 0 on error.
 */
unsigned int WiiTMD::cmdGroupCountV1(void)
{
	RP_D(WiiTMD);

	// This TMD must be v1.
	assert(d->tmdHeader.tmd_format_version == 1);
	if (d->tmdHeader.tmd_format_version != 1) {
		// Incorrect TMD version.
		return 0;
	}

	// Make sure the CMD group header is loaded.
	if (d->loadCmdGroupHeader() != 0) {
		// Unable to load the CMD group header.
		return 0;
	}

	// Find the first CMD group that has zero entries.
	unsigned int idx;
	for (idx = 0; idx < ARRAY_SIZE(d->cmdGroupHeader->entries); idx++) {
		if (d->cmdGroupHeader->entries[idx].nbr_cont == 0) {
			// Found a zero entry.
			break;
		}
	}

	// idx is the index of the first entry with zero contents.
	// This is the total number of valid CMD groups.
	return idx;
}

/**
 * Get the contents table. (for TMD v1)
 * @param grpIdx CMD group index
 * @return Contents table, or empty rp::uvector<> on error.
 */
rp::uvector<WUP_Content_Entry> WiiTMD::contentsTableV1(unsigned int grpIdx)
{
	// NOTE: Not cached, so the file needs to remain open.
	RP_D(WiiTMD);
	assert(d->isValid);
	assert((bool)d->file);
	assert(d->file->isOpen());
	if (!d->isValid || !d->file || !d->file->isOpen()) {
		// Unable to read data from the file.
		return {};
	}

	// grpIdx must be [0,64).
	assert(grpIdx < ARRAY_SIZE(WUP_CMD_GroupHeader::entries));
	if (grpIdx >= ARRAY_SIZE(WUP_CMD_GroupHeader::entries)) {
		return {};
	}

	// This TMD must be v1.
	assert(d->tmdHeader.tmd_format_version == 1);
	if (d->tmdHeader.tmd_format_version != 1) {
		// Incorrect TMD version.
		return {};
	}

	// Make sure the CMD group header is loaded.
	if (d->loadCmdGroupHeader() != 0) {
		// Unable to load the CMD group header.
		return {};
	}

	static const off64_t contents_tbl_offset =
		sizeof(RVL_TMD_Header) +
		sizeof(WUP_CMD_GroupHeader);

	// Read the contents specified in the selected group.
	const unsigned int nbr_cont = be16_to_cpu(d->cmdGroupHeader->entries[grpIdx].nbr_cont);
	if (nbr_cont == 0) {
		// No contents?
		return {};
	}

	const off64_t addr = contents_tbl_offset + (be16_to_cpu(d->cmdGroupHeader->entries[grpIdx].offset) * sizeof(WUP_Content_Entry));

	rp::uvector<WUP_Content_Entry> contentsTbl;
	const size_t data_size = nbr_cont * sizeof(WUP_Content_Entry);
	contentsTbl.resize(data_size);
	size_t size = d->file->seekAndRead(addr, contentsTbl.data(), data_size);
	if (likely(size == data_size))
		return contentsTbl;
	return {};
}

}
