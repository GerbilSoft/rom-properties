/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * BRSTM.cpp: Nintendo Wii BRSTM audio reader.                             *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "BRSTM.hpp"
#include "brstm_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;

namespace LibRomData {

class BRSTMPrivate final : public RomDataPrivate
{
public:
	explicit BRSTMPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(BRSTMPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// BRSTM headers
	// NOTE: Uses the endianness specified by the byte-order mark.
	BRSTM_Header brstmHeader;
	BRSTM_HEAD_Chunk1 headChunk1;

	// Is byteswapping needed?
	bool needsByteswap;

	/**
	 * Byteswap a uint16_t value from BRSTM to CPU.
	 * @param x Value to swap.
	 * @return Swapped value.
	 */
	inline uint16_t brstm16_to_cpu(uint16_t x)
	{
		return (needsByteswap ? __swab16(x) : x);
	}

	/**
	 * Byteswap a uint32_t value from BRSTM to CPU.
	 * @param x Value to swap.
	 * @return Swapped value.
	 */
	inline uint32_t brstm32_to_cpu(uint32_t x)
	{
		return (needsByteswap ? __swab32(x) : x);
	}
};

ROMDATA_IMPL(BRSTM)

/** BRSTMPrivate **/

/* RomDataInfo */
const char *const BRSTMPrivate::exts[] = {
	".brstm",

	nullptr
};
const char *const BRSTMPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"audio/x-brstm",

	nullptr
};
const RomDataInfo BRSTMPrivate::romDataInfo = {
	"BRSTM", exts, mimeTypes
};

BRSTMPrivate::BRSTMPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, needsByteswap(false)
{
	// Clear the BRSTM header structs.
	memset(&brstmHeader, 0, sizeof(brstmHeader));
	memset(&headChunk1, 0, sizeof(headChunk1));
}

/** BRSTM **/

/**
 * Read a Nintendo Wii BRSTM audio file.
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
BRSTM::BRSTM(const IRpFilePtr &file)
	: super(new BRSTMPrivate(file))
{
	RP_D(BRSTM);
	d->mimeType = "audio/x-brstm";	// unofficial, not on fd.o
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the BRSTM header.
	d->file->rewind();
	size_t size = d->file->read(&d->brstmHeader, sizeof(d->brstmHeader));
	if (size != sizeof(d->brstmHeader)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->brstmHeader), reinterpret_cast<const uint8_t*>(&d->brstmHeader)},
		nullptr,	// ext (not needed for BRSTM)
		0		// szFile (not needed for BRSTM)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Is byteswapping needed?
	d->needsByteswap = (d->brstmHeader.bom == BRSTM_BOM_SWAP);

	// Get the HEAD header.
	BRSTM_HEAD_Header headHeader;
	const uint32_t head_offset = d->brstm32_to_cpu(d->brstmHeader.head.offset);
	const uint32_t head_size = d->brstm32_to_cpu(d->brstmHeader.head.size);
	if (head_offset == 0 || head_size < sizeof(headHeader)) {
		// Invalid HEAD chunk.
		d->isValid = false;
		d->file.reset();
		return;
	}
	size = d->file->seekAndRead(head_offset, &headHeader, sizeof(headHeader));
	if (size != sizeof(headHeader)) {
		// Seek and/or read error.
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Verify the HEAD header.
	if (headHeader.magic != cpu_to_be32(BRSTM_HEAD_MAGIC)) {
		// Incorrect magic number.
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Get the HEAD chunk, part 1.
	// NOTE: Offset is relative to head_offset+8.
	const uint32_t head1_offset = d->brstm32_to_cpu(headHeader.head1_offset);
	if (head1_offset < sizeof(headHeader) - 8) {
		// Invalid offset.
		d->isValid = false;
		d->file.reset();
		return;
	}
	size = d->file->seekAndRead(head_offset + 8 + head1_offset, &d->headChunk1, sizeof(d->headChunk1));
	if (size != sizeof(d->headChunk1)) {
		// Seek and/or read error.
		d->isValid = false;
		d->file.reset();
		return;
	}

	// TODO: Verify headChunk1, or assume it's valid?
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int BRSTM::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(BRSTM_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const BRSTM_Header *const brstmHeader =
		reinterpret_cast<const BRSTM_Header*>(info->header.pData);

	// Check the BRSTM magic number.
	if (brstmHeader->magic != cpu_to_be32(BRSTM_MAGIC)) {
		// Not the BRSTM magic number.
		return -1;
	}

	// Check the byte-order mark.
	bool needsByteswap;
	switch (brstmHeader->bom) {
		case BRSTM_BOM_HOST:
			// Host-endian.
			needsByteswap = false;
			break;
		case BRSTM_BOM_SWAP:
			// Swapped-endian.
			needsByteswap = true;
			break;
		default:
			// Invalid.
			return -1;
	}

	// TODO: Check the version number, file size, and header size?

	// Check the chunks.
	// HEAD and DATA must both be present.
	const uint16_t chunk_count = (needsByteswap
		? __swab16(brstmHeader->chunk_count)
		: brstmHeader->chunk_count);
	if (chunk_count < 2) {
		// Not enough chunks.
		return -1;
	}

	// HEAD and DATA offset and sizes must both be non-zero.
	// No byteswapping is needed here.
	if (brstmHeader->head.offset == 0 ||
	    brstmHeader->head.size == 0 ||
	    brstmHeader->data.offset == 0 ||
	    brstmHeader->data.size == 0)
	{
		// Missing a required chunk.
		return -1;
	}

	// This is a BRSTM file.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *BRSTM::systemName(unsigned int type) const
{
	RP_D(const BRSTM);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// BRSTM has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"BRSTM::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Nintendo Wii", "Wii", "Wii", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int BRSTM::loadFieldData(void)
{
	RP_D(BRSTM);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// BRSTM headers
	const BRSTM_Header *const brstmHeader = &d->brstmHeader;
	const BRSTM_HEAD_Chunk1 *const headChunk1 = &d->headChunk1;
	d->fields.reserve(9);	// Maximum of 9 fields.

	// Type
	// NOTE: This is for consistency with BCSTM, and it's needed
	// because we don't show the format in systemName().
	// TODO: Add more formats?
	d->fields.addField_string(C_("RomData", "Type"), "BRSTM");

	// Version
	d->fields.addField_string(C_("RomData", "Version"),
		rp_sprintf("%u.%u", brstmHeader->version_major, brstmHeader->version_minor));

	// Endianness
	d->fields.addField_string(C_("RomData", "Endianness"),
		(brstmHeader->bom == cpu_to_be16(BRSTM_BOM_HOST))
			? C_("RomData", "Big-Endian")
			: C_("RomData", "Little-Endian"));

	// Codec
	static const array<const char*, 3> codec_tbl = {{
		NOP_C_("BRSTM|Codec", "Signed 8-bit PCM"),
		NOP_C_("BRSTM|Codec", "Signed 16-bit PCM"),
		"4-bit THP ADPCM",
	}};
	const char *const codec_title = C_("BRSTM", "Codec");
	if (headChunk1->codec < codec_tbl.size()) {
		d->fields.addField_string(codec_title,
			pgettext_expr("BRSTM|Codec", codec_tbl[headChunk1->codec]));
	} else {
		d->fields.addField_string(codec_title,
			rp_sprintf(C_("RomData", "Unknown (%u)"), headChunk1->codec));
	}

	// Number of channels
	d->fields.addField_string_numeric(C_("RomData|Audio", "Channels"), headChunk1->channel_count);

	// Sample rate and sample count
	const uint16_t sample_rate = d->brstm16_to_cpu(headChunk1->sample_rate);
	const uint32_t sample_count = d->brstm32_to_cpu(headChunk1->sample_count);

	// Sample rate
	d->fields.addField_string(C_("RomData|Audio", "Sample Rate"),
		rp_sprintf(C_("RomData", "%u Hz"), sample_rate));

	// Length (non-looping)
	d->fields.addField_string(C_("RomData|Audio", "Length"),
		formatSampleAsTime(sample_count, sample_rate));

	// Looping
	d->fields.addField_string(C_("BRSTM", "Looping"),
		(headChunk1->loop_flag ? C_("RomData", "Yes") : C_("RomData", "No")));
	if (headChunk1->loop_flag) {
		d->fields.addField_string(C_("BRSTM", "Loop Start"),
			formatSampleAsTime(d->brstm32_to_cpu(headChunk1->loop_start), sample_rate));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int BRSTM::loadMetaData(void)
{
	RP_D(BRSTM);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// BRSTM header chunk 1
	const BRSTM_HEAD_Chunk1 *const headChunk1 = &d->headChunk1;
	d->metaData.reserve(3);	// Maximum of 3 metadata properties.

	// Number of channels
	d->metaData.addMetaData_integer(Property::Channels, headChunk1->channel_count);

	// Sample rate and sample count
	const uint16_t sample_rate = d->brstm16_to_cpu(headChunk1->sample_rate);
	const uint32_t sample_count = d->brstm32_to_cpu(headChunk1->sample_count);

	// Sample rate
	d->metaData.addMetaData_integer(Property::SampleRate, sample_rate);

	// Length, in milliseconds (non-looping)
	d->metaData.addMetaData_integer(Property::Duration,
		convSampleToMs(sample_count, sample_rate));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

} // namespace LibRomData
