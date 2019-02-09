/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * BCSTM.cpp: Nintendo 3DS BCSTM audio reader.                             *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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

#include "BCSTM.hpp"
#include "librpbase/RomData_p.hpp"

#include "bcstm_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <string>
#include <sstream>
using std::ostringstream;
using std::string;

namespace LibRomData {

ROMDATA_IMPL(BCSTM)

class BCSTMPrivate : public RomDataPrivate
{
	public:
		BCSTMPrivate(BCSTM *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(BCSTMPrivate)

	public:
		// BCSTM headers.
		// NOTE: Uses the endianness specified by the byte-order mark.
		BCSTM_Header bcstmHeader;
		BCSTM_INFO_Block infoBlock;

		// Is byteswapping needed?
		bool needsByteswap;

		/**
		 * Byteswap a uint16_t value from BCSTM to CPU.
		 * @param x Value to swap.
		 * @return Swapped value.
		 */
		inline uint16_t bcstm16_to_cpu(uint16_t x)
		{
			return (needsByteswap ? __swab16(x) : x);
		}

		/**
		 * Byteswap a uint32_t value from BCSTM to CPU.
		 * @param x Value to swap.
		 * @return Swapped value.
		 */
		inline uint32_t bcstm32_to_cpu(uint32_t x)
		{
			return (needsByteswap ? __swab32(x) : x);
		}
};

/** BCSTMPrivate **/

BCSTMPrivate::BCSTMPrivate(BCSTM *q, IRpFile *file)
	: super(q, file)
	, needsByteswap(false)
{
	// Clear the BCSTM header structs.
	memset(&bcstmHeader, 0, sizeof(bcstmHeader));
	memset(&infoBlock, 0, sizeof(infoBlock));
}

/** BCSTM **/

/**
 * Read a Nintendo 3DS BCSTM audio file.
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
BCSTM::BCSTM(IRpFile *file)
	: super(new BCSTMPrivate(this, file))
{
	RP_D(BCSTM);
	d->className = "BCSTM";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the BCSTM header.
	d->file->rewind();
	size_t size = d->file->read(&d->bcstmHeader, sizeof(d->bcstmHeader));
	if (size != sizeof(d->bcstmHeader)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->bcstmHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->bcstmHeader);
	info.ext = nullptr;	// Not needed for BCSTM.
	info.szFile = 0;	// Not needed for BCSTM.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Is byteswapping needed?
	d->needsByteswap = (d->bcstmHeader.bom == BCSTM_BOM_SWAP);

	// Get the INFO block.
	const uint32_t info_offset = d->bcstm32_to_cpu(d->bcstmHeader.info.ref.offset);
	const uint32_t info_size = d->bcstm32_to_cpu(d->bcstmHeader.info.size);
	if (info_offset == 0 || info_offset == ~0 ||
	    info_size < sizeof(d->infoBlock))
	{
		// Invalid INFO block.
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
		return;
	}
	size = d->file->seekAndRead(info_offset, &d->infoBlock, sizeof(d->infoBlock));
	if (size != sizeof(d->infoBlock)) {
		// Seek and/or read error.
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Verify the INFO block.
	if (d->infoBlock.magic != cpu_to_be32(BCSTM_INFO_MAGIC)) {
		// Incorrect magic number.
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// TODO: Verify anything else in the info block?
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int BCSTM::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(BCSTM_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const BCSTM_Header *const bcstmHeader =
		reinterpret_cast<const BCSTM_Header*>(info->header.pData);

	// Check the BCSTM magic number.
	if (bcstmHeader->magic != cpu_to_be32(BCSTM_MAGIC)) {
		// Not the BCSTM magic number.
		return -1;
	}

	// Check the byte-order mark.
	bool needsByteswap;
	switch (bcstmHeader->bom) {
		case BCSTM_BOM_HOST:
			// Host-endian.
			needsByteswap = false;
			break;
		case BCSTM_BOM_SWAP:
			// Swapped-endian.
			needsByteswap = true;
			break;
		default:
			// Invalid.
			return -1;
	}

	// TODO: Check the version number, file size, and header size?

	// Check the block count.
	// INFO, SEEK, and DATA must all be present.
	const uint16_t block_count = (needsByteswap
		? __swab16(bcstmHeader->block_count)
		: bcstmHeader->block_count);
	if (block_count < 3) {
		// Not enough blocks.
		return -1;
	}

	// INFO, SEEK, and DATA offset and sizes must both be valid.
	// Invalid values: 0, ~0 (0xFFFFFFFF)
	// No byteswapping is needed here.
	if (bcstmHeader->info.ref.offset == 0 ||
	    bcstmHeader->info.ref.offset == ~0 ||
	    bcstmHeader->info.size == 0 ||
	    bcstmHeader->info.size == ~0 ||
	    bcstmHeader->seek.ref.offset == 0 ||
	    bcstmHeader->seek.ref.offset == ~0 ||
	    bcstmHeader->seek.size == 0 ||
	    bcstmHeader->seek.size == ~0 ||
	    bcstmHeader->data.ref.offset == 0 ||
	    bcstmHeader->data.ref.offset == ~0 ||
	    bcstmHeader->data.size == 0 ||
	    bcstmHeader->data.size == ~0)
	{
		// Missing a required block.
		return -1;
	}

	// This is a BCSTM file.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *BCSTM::systemName(unsigned int type) const
{
	RP_D(const BCSTM);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// BCSTM has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"BCSTM::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo 3DS BCSTM",
		"BCSTM",
		"BCSTM",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *BCSTM::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".bcstm",

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *BCSTM::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"audio/x-bcstm",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int BCSTM::loadFieldData(void)
{
	RP_D(BCSTM);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// BCSTM headers
	const BCSTM_Header *const bcstmHeader = &d->bcstmHeader;
	const BCSTM_Stream_Info *const streamInfo = &d->infoBlock.stream_info;
	d->fields->reserve(8);	// Maximum of 8 fields.

	// TODO: Show the version field?

	// Endianness
	d->fields->addField_string(C_("BCSTM", "Endianness"),
		(bcstmHeader->bom == cpu_to_be16(BCSTM_BOM_HOST))
			? C_("BCSTM", "Big-Endian")
			: C_("BCSTM", "Little-Endian"));

	// Codec
	const char *const codec_tbl[] = {
		NOP_C_("BCSTM|Codec", "Signed 8-bit PCM"),
		NOP_C_("BCSTM|Codec", "Signed 16-bit PCM"),
		NOP_C_("BCSTM|Codec", "DSP ADPCM"),
		NOP_C_("BCSTM|Codec", "IMA ADPCM"),
	};
	if (streamInfo->codec < ARRAY_SIZE(codec_tbl)) {
		d->fields->addField_string(C_("BCSTM", "Codec"),
			dpgettext_expr(RP_I18N_DOMAIN, "BCSTM|Codec", codec_tbl[streamInfo->codec]));
	} else {
		d->fields->addField_string(C_("BCSTM", "Codec"),
			rp_sprintf(C_("RomData", "Unknown (%u)"), streamInfo->codec));
	}

	// Number of channels
	d->fields->addField_string_numeric(C_("RomData|Audio", "Channels"), streamInfo->channel_count);

	// Sample rate and sample count
	const uint32_t sample_rate = d->bcstm32_to_cpu(streamInfo->sample_rate);
	// Sample count needs to be calculated based on
	// sample block count, number of samples per block,
	// and number of samples in the last block.
	const uint32_t sample_count =
		(d->bcstm32_to_cpu(streamInfo->sample_block_count - 1) *
		 d->bcstm32_to_cpu(streamInfo->sample_block_sample_count)) +
		d->bcstm32_to_cpu(streamInfo->last_sample_block_sample_count);

	// Sample rate
	// NOTE: Using ostringstream for localized numeric formatting.
	ostringstream oss;
	oss << sample_rate << " Hz";
	d->fields->addField_string(C_("RomData|Audio", "Sample Rate"), oss.str());

	// Length (non-looping)
	d->fields->addField_string(C_("RomData|Audio", "Length"),
		formatSampleAsTime(sample_count, sample_rate));

	// Looping
	d->fields->addField_string(C_("BCSTM", "Looping"),
		(streamInfo->loop_flag ? C_("RomData", "Yes") : C_("RomData", "No")));
	if (streamInfo->loop_flag) {
		d->fields->addField_string(C_("BCSTM", "Loop Start"),
			formatSampleAsTime(d->bcstm32_to_cpu(streamInfo->loop_start), sample_rate));
		d->fields->addField_string(C_("BCSTM", "Loop End"),
			formatSampleAsTime(d->bcstm32_to_cpu(streamInfo->loop_end), sample_rate));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int BCSTM::loadMetaData(void)
{
	RP_D(BCSTM);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// BCSTM stream info
	const BCSTM_Stream_Info *const streamInfo = &d->infoBlock.stream_info;

	// Number of channels
	d->metaData->addMetaData_integer(Property::Channels, streamInfo->channel_count);

	// Sample rate and sample count
	const uint32_t sample_rate = d->bcstm32_to_cpu(streamInfo->sample_rate);
	// Sample count needs to be calculated based on
	// sample block count, number of samples per block,
	// and number of samples in the last block.
	const uint32_t sample_count =
		(d->bcstm32_to_cpu(streamInfo->sample_block_count - 1) *
		 d->bcstm32_to_cpu(streamInfo->sample_block_sample_count)) +
		d->bcstm32_to_cpu(streamInfo->last_sample_block_sample_count);

	// Sample rate
	d->metaData->addMetaData_integer(Property::SampleRate, sample_rate);

	// Length, in milliseconds (non-looping)
	d->metaData->addMetaData_integer(Property::Duration,
		convSampleToMs(sample_count, sample_rate));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
