/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * BCSTM.cpp: Nintendo 3DS BCSTM and Nintendo Wii U BFSTM audio reader.    *
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
		// Audio format.
		enum AudioFormat {
			AUDIO_FORMAT_UNKNOWN = -1,	// Unknown file type.

			AUDIO_FORMAT_BCSTM = 0,
			AUDIO_FORMAT_BFSTM = 1,
			AUDIO_FORMAT_BCWAV = 2,

			AUDIO_FORMAT_MAX
		};
		int audioFormat;

	public:
		// BCSTM headers.
		// NOTE: Uses the endianness specified by the byte-order mark.
		BCSTM_Header bcstmHeader;
		union {
			BCSTM_INFO_Block cstm;
			BCWAV_INFO_Block cwav;
		} infoBlock;

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
	, audioFormat(AUDIO_FORMAT_UNKNOWN)
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
	d->audioFormat = isRomSupported_static(&info);

	if (d->audioFormat < 0) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Is byteswapping needed?
	d->needsByteswap = (d->bcstmHeader.bom == BCSTM_BOM_SWAP);

	// Get the INFO block.
	const uint32_t info_offset = d->bcstm32_to_cpu(d->bcstmHeader.info.ref.offset);
	const uint32_t info_size = d->bcstm32_to_cpu(d->bcstmHeader.info.size);
	const size_t req_size = (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCWAV
		? sizeof(d->infoBlock.cwav) : sizeof(d->infoBlock.cstm));
       if (info_offset == 0 || info_offset == ~0U ||
	    info_size < req_size)
	{
		// Invalid INFO block.
		d->audioFormat = BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
		d->file->unref();
		d->file = nullptr;
		return;
	}
	if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCWAV) {
		size = d->file->seekAndRead(info_offset,
			&d->infoBlock.cwav, sizeof(d->infoBlock.cwav));
	} else {
		size = d->file->seekAndRead(info_offset,
			&d->infoBlock.cstm, sizeof(d->infoBlock.cstm));
	}
	if (size != req_size) {
		// Seek and/or read error.
		d->audioFormat = BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Verify the INFO block.
	// NOTE: Both info blocks have the same position for
	// magic and size, so we don't have to check the fileType.
       if (d->infoBlock.cstm.magic != cpu_to_be32(BCSTM_INFO_MAGIC)) {
		// Incorrect magic number.
		d->audioFormat = BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// TODO: Verify anything else in the info block?
	d->isValid = true;
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

	// Check the magic number.
	int audioFormat;
	if (bcstmHeader->magic == cpu_to_be32(BCSTM_MAGIC)) {
		// BCSTM magic number.
		audioFormat = BCSTMPrivate::AUDIO_FORMAT_BCSTM;
	} else if (bcstmHeader->magic == cpu_to_be32(BFSTM_MAGIC)) {
		// BFSTM magic number.
		audioFormat = BCSTMPrivate::AUDIO_FORMAT_BFSTM;
       } else if (bcstmHeader->magic == cpu_to_be32(BCWAV_MAGIC)) {
		// BCWAV magic number.
		audioFormat = BCSTMPrivate::AUDIO_FORMAT_BCWAV;
	} else {
		// Invalid magic number.
		return BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
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
			return BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
	}

	// TODO: Check the version number, file size, and header size?

	// Check the block count.
	// INFO, SEEK, and DATA must all be present.
	// (BCWAV: No SEEK block.)
	const uint16_t block_count = (needsByteswap
		? __swab16(bcstmHeader->block_count)
		: bcstmHeader->block_count);
	const uint16_t req_block_count =
		(audioFormat != BCSTMPrivate::AUDIO_FORMAT_BCWAV ? 3 : 2);
	if (block_count < req_block_count) {
		// Not enough blocks.
		return BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
	}

	// INFO, SEEK, and DATA offset and sizes must both be valid.
	// Invalid values: 0, ~0 (0xFFFFFFFF)
	// No byteswapping is needed here.
	if (bcstmHeader->info.ref.offset == 0 ||
	    bcstmHeader->info.ref.offset == ~0U ||
	    bcstmHeader->info.size == 0 ||
	    bcstmHeader->info.size == ~0U)
	{
		// Missing a required block.
		return BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
	}

	// SEEK is not present in BCWAV.
	if (audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCWAV) {
		if (bcstmHeader->cwav.data.ref.offset == 0 ||
		    bcstmHeader->cwav.data.ref.offset == ~0U ||
		    bcstmHeader->cwav.data.size == 0 ||
		    bcstmHeader->cwav.data.size == ~0U)
		{
			// Missing a required block.
			return BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
		}
	} else {
		// BCSTM/BFSTM
		if (bcstmHeader->cstm.seek.ref.offset == 0 ||
		    bcstmHeader->cstm.seek.ref.offset == ~0U ||
		    bcstmHeader->cstm.seek.size == 0 ||
		    bcstmHeader->cstm.seek.size == ~0U ||
		    bcstmHeader->cstm.data.ref.offset == 0 ||
		    bcstmHeader->cstm.data.ref.offset == ~0U ||
		    bcstmHeader->cstm.data.size == 0 ||
		    bcstmHeader->cstm.data.size == ~0U)
		{
			// Missing a required block.
			return BCSTMPrivate::AUDIO_FORMAT_UNKNOWN;
		}
	}

	// This is a supported file.
	return audioFormat;
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

	switch (d->audioFormat) {
		case BCSTMPrivate::AUDIO_FORMAT_BCSTM:
		case BCSTMPrivate::AUDIO_FORMAT_BCWAV: {
			// Nintendo 3DS
			static const char *const sysNames_3DS[4] = {
				"Nintendo 3DS", "Nintendo 3DS", "3DS", nullptr
			};
			return sysNames_3DS[type & SYSNAME_TYPE_MASK];
		}

		case BCSTMPrivate::AUDIO_FORMAT_BFSTM: {
			// Wii U
			static const char *const sysNames_WiiU[4] = {
				"Nintendo Wii U", "Wii U", "Wii U", nullptr
			};
			return sysNames_WiiU[type & SYSNAME_TYPE_MASK];
		}

		default:
			assert(!"BCSTM: Invalid audio format.");
			break;
	}

	// Should not get here...
	return nullptr;
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
		".bfstm",
		".bcwav",

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
	d->fields->reserve(8);	// Maximum of 8 fields.

	// Type
	// Disambiguates between the supported formats.
	static const char type_tbl[][8] = {
		"BCSTM", "BFSTM", "BCWAV"
	};
	if (d->audioFormat > BCSTMPrivate::AUDIO_FORMAT_UNKNOWN &&
	    d->audioFormat < ARRAY_SIZE(type_tbl))
	{
		d->fields->addField_string(C_("BCSTM", "Type"), type_tbl[d->audioFormat]);
	} else {
		d->fields->addField_string(C_("BCSTM", "Type"),
			rp_sprintf(C_("RomData", "Unknown (%d)"), d->audioFormat));
	}

	// TODO: Show the version field?

	// Endianness
	d->fields->addField_string(C_("BCSTM", "Endianness"),
		(bcstmHeader->bom == cpu_to_be16(BCSTM_BOM_HOST))
			? C_("BCSTM", "Big-Endian")
			: C_("BCSTM", "Little-Endian"));

	// Get stream info data.
	uint8_t codec, loop_flag, channel_count;
	uint32_t sample_rate, sample_count, loop_start, loop_end;
	if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCWAV) {
		// BCWAV uses a simpler info block.
		const BCWAV_INFO_Block *const info = &d->infoBlock.cwav;
		codec		= info->codec;
		loop_flag	= info->loop_flag;
		channel_count	= 2;    // fixed value for CWAV
		sample_rate	= d->bcstm32_to_cpu(info->sample_rate);
		loop_start	= d->bcstm32_to_cpu(info->loop_start);
		loop_end	= d->bcstm32_to_cpu(info->loop_end);

		// TODO: Figure out the sample count.
		sample_count	= 0;
	} else {
		// BCSTM/BFSTM has a full Stream Info block.
		const BCSTM_Stream_Info *const info = &d->infoBlock.cstm.stream_info;
		codec		= info->codec;
		loop_flag	= info->loop_flag;
		channel_count	= info->channel_count;
		sample_rate	= d->bcstm32_to_cpu(info->sample_rate);
		loop_start	= d->bcstm32_to_cpu(info->loop_start);

		if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCSTM) {
			loop_end = d->bcstm32_to_cpu(info->loop_end);

			// BCSTM: Sample count needs to be calculated based on
			// sample block count, number of samples per block, and
			// number of samples in the last block.
			sample_count =
				(d->bcstm32_to_cpu(info->sample_block_count - 1) *
				 d->bcstm32_to_cpu(info->sample_block_sample_count)) +
				d->bcstm32_to_cpu(info->last_sample_block_sample_count);
		} else /* if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BFSTM) */ {
			// TODO: Verify that this isn't used in looping BFSTMs.
			loop_end = 0;

			// BFSTM: Sample block count is too high for some reason.
			// Use the total frame count instead.
			sample_count = d->bcstm32_to_cpu(info->frame_count);
		}
	}

	// Codec
	static const char *const codec_tbl[] = {
		NOP_C_("BCSTM|Codec", "Signed 8-bit PCM"),
		NOP_C_("BCSTM|Codec", "Signed 16-bit PCM"),
		NOP_C_("BCSTM|Codec", "DSP ADPCM"),
		NOP_C_("BCSTM|Codec", "IMA ADPCM"),
	};
	if (codec < ARRAY_SIZE(codec_tbl)) {
		d->fields->addField_string(C_("BCSTM", "Codec"),
			dpgettext_expr(RP_I18N_DOMAIN, "BCSTM|Codec", codec_tbl[codec]));
	} else {
		d->fields->addField_string(C_("BCSTM", "Codec"),
			rp_sprintf(C_("RomData", "Unknown (%u)"), codec));
	}

	// Number of channels
	d->fields->addField_string_numeric(C_("RomData|Audio", "Channels"), channel_count);

	// Sample rate
	// NOTE: Using ostringstream for localized numeric formatting.
	ostringstream oss;
	oss << sample_rate << " Hz";
	d->fields->addField_string(C_("RomData|Audio", "Sample Rate"), oss.str());

	// Length (non-looping)
	// TODO: Figure this out for BCWAV.
	if (d->audioFormat != BCSTMPrivate::AUDIO_FORMAT_BCWAV) {
		d->fields->addField_string(C_("RomData|Audio", "Length"),
			formatSampleAsTime(sample_count, sample_rate));
	}

	// Looping
	d->fields->addField_string(C_("BCSTM", "Looping"),
		(loop_flag ? C_("RomData", "Yes") : C_("RomData", "No")));
	if (loop_flag) {
		d->fields->addField_string(C_("BCSTM", "Loop Start"),
			formatSampleAsTime(loop_start, sample_rate));
		if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCSTM) {
			// TODO: Verify that this isn't used in looping BFSTMs.
			d->fields->addField_string(C_("BCSTM", "Loop End"),
				formatSampleAsTime(loop_end, sample_rate));
		}
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

	// Get stream info data.
	uint8_t channel_count;
	uint32_t sample_rate, sample_count;
	if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCWAV) {
		// BCWAV uses a simpler info block.
		const BCWAV_INFO_Block *const info = &d->infoBlock.cwav;
		channel_count	= 2;    // fixed value for CWAV
		sample_rate	= d->bcstm32_to_cpu(info->sample_rate);

		// TODO: Figure out the sample count.
		sample_count	= 0;
	} else {
		// BCSTM/BFSTM has a full Stream Info block.
		const BCSTM_Stream_Info *const info = &d->infoBlock.cstm.stream_info;
		channel_count	= info->channel_count;
		sample_rate	= d->bcstm32_to_cpu(info->sample_rate);

		if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BCSTM) {
			// BCSTM: Sample count needs to be calculated based on
			// sample block count, number of samples per block, and
			// number of samples in the last block.
			sample_count =
				(d->bcstm32_to_cpu(info->sample_block_count - 1) *
				 d->bcstm32_to_cpu(info->sample_block_sample_count)) +
				d->bcstm32_to_cpu(info->last_sample_block_sample_count);
		} else /* if (d->audioFormat == BCSTMPrivate::AUDIO_FORMAT_BFSTM) */ {
			// BFSTM: Sample block count is too high for some reason.
			// Use the total frame count instead.
			sample_count = d->bcstm32_to_cpu(info->frame_count);
		}
	}

	// Number of channels
	d->metaData->addMetaData_integer(Property::Channels, channel_count);

	// Sample rate
	d->metaData->addMetaData_integer(Property::SampleRate, sample_rate);

	// Length, in milliseconds (non-looping)
	// TODO: Figure this out for BCWAV.
	if (d->audioFormat != BCSTMPrivate::AUDIO_FORMAT_BCWAV) {
		d->metaData->addMetaData_integer(Property::Duration,
			convSampleToMs(sample_count, sample_rate));
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
