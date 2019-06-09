/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ADX.hpp: CRI ADX audio reader.                                          *
 *                                                                         *
 * Copyright (c) 2018-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ADX.hpp"
#include "librpbase/RomData_p.hpp"

#include "adx_structs.h"

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
#include <cstring>

// C++ includes.
#include <string>
#include <sstream>
using std::ostringstream;
using std::string;

namespace LibRomData {

ROMDATA_IMPL(ADX)

class ADXPrivate : public RomDataPrivate
{
	public:
		ADXPrivate(ADX *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ADXPrivate)

	public:
		// ADX header.
		// NOTE: **NOT** byteswapped in memory.
		ADX_Header adxHeader;
		const ADX_LoopData *pLoopData;
};

/** ADXPrivate **/

ADXPrivate::ADXPrivate(ADX *q, IRpFile *file)
	: super(q, file)
	, pLoopData(nullptr)
{
	// Clear the ADX header struct.
	memset(&adxHeader, 0, sizeof(adxHeader));
}

/** ADX **/

/**
 * Read a CRI ADX audio file.
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
ADX::ADX(IRpFile *file)
	: super(new ADXPrivate(this, file))
{
	RP_D(ADX);
	d->className = "ADX";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read 4,096 bytes to ensure we have enough
	// data to detect the copyright string.
	uint8_t header[4096];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for ADX.
	info.szFile = 0;	// Not needed for ADX.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Save the ROM header.
	memcpy(&d->adxHeader, header, sizeof(d->adxHeader));

	// Initialize pLoopData.
	switch (d->adxHeader.loop_data_style) {
		case 3:
			d->pLoopData = &d->adxHeader.loop_03.data;
			break;
		case 4:
			d->pLoopData = &d->adxHeader.loop_04.data;
			break;
		case 5:
		default:
			// No loop data.
			d->pLoopData = nullptr;
			break;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ADX::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(ADX_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const ADX_Header *const adx_header =
		reinterpret_cast<const ADX_Header*>(info->header.pData);

	// Check the ADX magic number.
	if (adx_header->magic != cpu_to_be16(ADX_MAGIC_NUM)) {
		// Not the ADX magic number.
		return -1;
	}

	// Check the format.
	switch (adx_header->format) {
		case ADX_FORMAT_FIXED_COEFF_ADPCM:
		case ADX_FORMAT_ADX:
		case ADX_FORMAT_ADX_EXP_SCALE:
		case ADX_FORMAT_AHX_DC:
		case ADX_FORMAT_AHX:
			// Valid format.
			break;

		default:
			// Not a valid format.
			return -1;
	}

	// Check the copyright string.
	unsigned int cpy_offset = be16_to_cpu(adx_header->data_offset);
	if (cpy_offset < 2) {
		// Invalid offset.
		return -1;
	}
	cpy_offset -= 2;
	if (cpy_offset + sizeof(ADX_MAGIC_STR) - 1 > info->header.size) {
		// Out of range.
		return -1;
	}
	if (memcmp(&info->header.pData[cpy_offset], ADX_MAGIC_STR, sizeof(ADX_MAGIC_STR)-1) != 0) {
		// Missing copyright string.
		return -1;
	}

	// This is an ADX file.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ADX::systemName(unsigned int type) const
{
	RP_D(const ADX);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// ADX has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"ADX::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"CRI ADX", "ADX", "ADX", nullptr
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
const char *const *ADX::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".adx",
		".ahx",	// TODO: Is this used for AHX format?

		// TODO: AAX is two ADXes glued together.
		//".aax",

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
const char *const *ADX::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"audio/x-adx",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ADX::loadFieldData(void)
{
	RP_D(ADX);
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

	// ADX header
	const ADX_Header *const adxHeader = &d->adxHeader;
	d->fields->reserve(8);	// Maximum of 8 fields.

	// Format
	const char *format;
	switch (adxHeader->format) {
		case ADX_FORMAT_FIXED_COEFF_ADPCM:
			format = C_("ADX|Format", "Fixed Coefficient ADPCM");
			break;
		case ADX_FORMAT_ADX:
			// NOTE: Not translatable.
			format = "ADX";
			break;
		case ADX_FORMAT_ADX_EXP_SCALE:
			format = C_("ADX|Format", "ADX with Exponential Scale");
			break;
		case ADX_FORMAT_AHX_DC:
			// NOTE: Not translatable.
			format = "AHX (Dreamcast)";
			break;
		case ADX_FORMAT_AHX:
			// NOTE: Not translatable.
			format = "AHX";
			break;
		default:
			// NOTE: This should not be reachable...
			format = C_("RomData", "Unknown");
			break;
	}
	d->fields->addField_string(C_("RomData|Audio", "Format"), format);

	// Number of channels
	d->fields->addField_string_numeric(C_("RomData|Audio", "Channels"), adxHeader->channel_count);

	// Sample rate and sample count
	const uint32_t sample_rate = be32_to_cpu(adxHeader->sample_rate);
	const uint32_t sample_count = be32_to_cpu(adxHeader->sample_count);

	// Sample rate
	// NOTE: Using ostringstream for localized numeric formatting.
	ostringstream oss;
	oss << sample_rate << " Hz";
	d->fields->addField_string(C_("RomData|Audio", "Sample Rate"), oss.str());

	// Length. (non-looping)
	d->fields->addField_string(C_("RomData|Audio", "Length"),
		formatSampleAsTime(sample_count, sample_rate));

#if 0
	// High-pass cutoff
	// TODO: What does this value represent?
	// FIXME: Disabling until I figure this out.
	oss.str("");
	oss << be16_to_cpu(adxHeader->high_pass_cutoff) << " Hz";
	d->fields->addField_string(C_("ADX", "High-Pass Cutoff"), oss.str());
#endif

	// Translated strings
	const char *const s_yes = C_("ADX", "Yes");
	const char *const s_no  = C_("ADX", "No");

	// Encryption
	d->fields->addField_string(C_("ADX", "Encrypted"),
		(adxHeader->flags & ADX_FLAG_ENCRYPTED ? s_yes : s_no));

	// Looping
	const ADX_LoopData *const pLoopData = d->pLoopData;
	const bool isLooping = (pLoopData && pLoopData->loop_flag != 0);
	d->fields->addField_string(C_("ADX", "Looping"),
		(isLooping ? s_yes : s_no));
	if (isLooping) {
		d->fields->addField_string(C_("ADX", "Loop Start"),
			formatSampleAsTime(be32_to_cpu(pLoopData->start_sample), sample_rate));
		d->fields->addField_string(C_("ADX", "Loop End"),
			formatSampleAsTime(be32_to_cpu(pLoopData->end_sample), sample_rate));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int ADX::loadMetaData(void)
{
	RP_D(ADX);
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

	// ADX header
	const ADX_Header *const adxHeader = &d->adxHeader;

	// Number of channels
	d->metaData->addMetaData_integer(Property::Channels, adxHeader->channel_count);

	// Sample rate and sample count
	const uint32_t sample_rate = be32_to_cpu(adxHeader->sample_rate);
	const uint32_t sample_count = be32_to_cpu(adxHeader->sample_count);

	// Sample rate
	d->metaData->addMetaData_integer(Property::SampleRate, sample_rate);

	// Length, in milliseconds (non-looping)
	d->metaData->addMetaData_integer(Property::Duration,
		convSampleToMs(sample_count, sample_rate));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
