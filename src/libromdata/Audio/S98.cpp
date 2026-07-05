/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * S98.hpp: S98 audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"

#include "S98.hpp"
#include "RomData_p.hpp"

#include "PSFTagParser.hpp"
#include "s98_structs.h"

#include "common.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C includes
#include "ctypex.h"

// C++ STL classes
#include <unordered_map>
using std::array;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

// libfmt
#include "rp-libfmt.h"

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class S98Private final : public RomDataPrivate
{
public:
	explicit S98Private(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(S98Private)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// S98 header
	// NOTE: **NOT** byteswapped in memory.
	S98_Header s98Header;

	// Device info
	// NOTE: **NOT** byteswapped in memory.
	rp::uvector<S98_Device_Info_t> vDeviceInfo;

	/**
	 * Parse the tag section.
	 * @param tag_addr Tag section starting address.
	 * @return Map containing key/value entries.
	 */
	unordered_map<string, string> parseTags(off64_t tag_addr);
};

ROMDATA_IMPL(S98)

/** S98Private **/

/* RomDataInfo */
const array<const char*, 1+1> S98Private::exts = {{
	".s98",

	nullptr
}};
const array<const char*, 1+1> S98Private::mimeTypes = {{
        // Unofficial MIME types.
        // TODO: Get these upstreamed on FreeDesktop.org.
	"audio/x-s98",

	nullptr
}};
const RomDataInfo S98Private::romDataInfo = {
	"S98", exts.data(), mimeTypes.data()
};

S98Private::S98Private(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the S98 header struct.
	memset(&s98Header, 0, sizeof(s98Header));
}

/**
 * Parse the tag section.
 * @param tag_addr Tag section starting address.
 * @return Map containing key/value entries.
 */
unordered_map<string, string> S98Private::parseTags(off64_t tag_addr)
{
	// Load the tag section.
	// NOTE: Maximum of 16 KB.
	static constexpr off64_t TAG_SIZE_MAX = 16384;
	off64_t data_len = file->size() - tag_addr;
	if (data_len <= 0) {
		// Not enough data.
		return {};
	} else if (data_len > TAG_SIZE_MAX) {
		// S98 tags aren't necessarily stored at the end of the file,
		// so just truncate the data length.
		data_len = TAG_SIZE_MAX;
	}

	const size_t data_len_sz = static_cast<size_t>(data_len);
	unique_ptr<char[]> tag_data(new char[data_len_sz]);
	size_t size = file->seekAndRead(tag_addr, tag_data.get(), data_len_sz);
	if (size != data_len_sz) {
		// Read error.
		return {};
	}

	// Parse the tags.
	return PSFTagParser::parseTags(tag_data.get(), data_len_sz, PSFTagParser::PSFTagStyle::S98);
}

/** S98 **/

/**
 * Read an S98 audio file.
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
S98::S98(const IRpFilePtr &file)
	: super(new S98Private(file))
{
	RP_D(S98);
	d->mimeType = "audio/x-s98";	// unofficial (TODO: x-minipsf?)
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the S98 header.
	d->file->rewind();
	size_t size = d->file->read(&d->s98Header, sizeof(d->s98Header));
	if (size != sizeof(d->s98Header)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->s98Header), reinterpret_cast<const uint8_t*>(&d->s98Header)},
		nullptr,	// ext (not needed for S98)
		0		// szFile (not needed for S98)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Load the device info.
	// TODO: Support for v2, which has an implicit count.
	if (d->s98Header.version == '3') {
		uint32_t deviceCount = le32_to_cpu(d->s98Header.v3_device_count);
		if (deviceCount == 0) {
			// Using a single implicit OPNA with 7,987,200 Hz.
			d->vDeviceInfo.resize(1);
			S98_Device_Info_t &dev = d->vDeviceInfo[0];
			dev.type = S98_DEVICE_TYPE_OPNA_YM2608;
			dev.clock = 7987200;
			dev.v3_pan = 0;
			dev.v2_data = 0;
		} else {
			// Sanity check: Limit of 128 devices.
			assert(deviceCount <= 128U);
			deviceCount = std::min(deviceCount, 128U);

			const size_t expected_size = deviceCount * sizeof(S98_Device_Info_t);
			d->vDeviceInfo.resize(deviceCount);
			size_t size = d->file->seekAndRead(sizeof(S98_Header), d->vDeviceInfo.data(), expected_size);
			if (size != expected_size) {
				// Seek and/or read error.
				d->isValid = false;
				d->file.reset();
				d->vDeviceInfo.clear();
				return;
			}
		}
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int S98::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(S98_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const S98_Header *const s98Header =
		reinterpret_cast<const S98_Header*>(info->header.pData);

	// Check the S98 magic number.
	if (!memcmp(s98Header->magic, S98_MAGIC, sizeof(s98Header->magic))) {
		// Found the S98 magic number.
		// Is the version number valid?
		if (s98Header->version >= '0' && s98Header->version <= '3') {
			// Valid version number.
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *S98::systemName(unsigned int type) const
{
	RP_D(const S98);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// S98 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"S98::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"S98", "S98", "S98", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int S98::loadFieldData(void)
{
	RP_D(S98);
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

	// S98 header
	const S98_Header *const s98Header = &d->s98Header;

	// S98 fields:
	// - 4 regular fields.
	// - 12 fields in the "[S98]" section.
	d->fields.reserve(4+12);

	// Version
	const bool isV3 = (s98Header->version == '3');
	const char sVersion[2] = {s98Header->version, '\0'};
	d->fields.addField_string(C_("S98", "Format Version"), sVersion);

	// Timer info
	const uint32_t timer_info = (s98Header->timer_info != 0)
		? le32_to_cpu(s98Header->timer_info)
		: S98_TIMER_INFO_DEFAULT;
	const uint32_t timer_info2 = (s98Header->timer_info2 != 0)
		? le32_to_cpu(s98Header->timer_info2)
		: S98_TIMER_INFO2_DEFAULT;
	d->fields.addField_string(C_("S98", "Timer Info"),
		fmt::format(FSTR("{:d}/{:d}"), timer_info, timer_info2));

	// Tags (v3 only)
	if (isV3 && s98Header->tag_offset != 0) {
		unordered_map<string, string> tags = d->parseTags(le32_to_cpu(s98Header->tag_offset));
		if (!tags.empty()) {
			PSFTagParser::addTagsToRomFields(&d->fields, tags, "s98by");
		}
	}

	// Device info
	if (!d->vDeviceInfo.empty()) {
		const size_t columnCount = (isV3) ? 4 : 3;

		auto *const vv_deviceInfo = new RomFields::ListData_t();
		for (S98_Device_Info_t &devInfo : d->vDeviceInfo) {
			const uint32_t type = le32_to_cpu(devInfo.type);
#ifndef NDEBUG
			if (isV3) { assert(type != S98_DEVICE_TYPE_NONE); }
#endif /* !NDEBUG */
			if (type == S98_DEVICE_TYPE_NONE) {
				// v2: End of device info.
				// v3: Not valid; we have an explicit device count!
				break;
			}

			const size_t vidx = vv_deviceInfo->size();
			vv_deviceInfo->resize(vidx+1);
			auto &data_row = vv_deviceInfo->at(vidx);
			data_row.reserve(columnCount);

			// NOTE: Valid types are 0-9 and 15-16.
			// Will use a lookup table for 0-9, then special-case for 15/16.
			// TODO: Don't allow >OPM for v2 and earlier?

			// Chip IDs
			static const char chipID_tbl[10][8] = {
				"", "PSG", "OPN", "OPN2",
				"OPNA", "OPM", "OPLL", "OPL",
				"OPL2", "OPL3",
			};

			// Chip Models
			static const char chipModel_tbl[10][8] = {
				"", "YM2149", "YM2203", "YM2612",
				"YM2608", "YM2151", "YM2413", "YM3526",
				"YM3812", "YMF262",
			};

			const char *chipID, *chipModel;
			if (type <= 9) {
				chipID = chipID_tbl[type];
				chipModel = chipModel_tbl[type];
			} else if (type == S98_DEVICE_TYPE_PSG_AY_3_8910) {
				chipID = "PSG";
				chipModel = "AY-3-8910";
			} else if (type == S98_DEVICE_TYPE_DCSG_SN76489) {
				chipID = "DCSG";
				chipModel = "SN76489";
			} else {
				chipID = nullptr;
				chipModel = nullptr;
			}

			// Check for errors.
			if (unlikely(!chipID || chipID[0] == '\0')) {
				chipID = C_("S98", "(none)");
			}
			if (unlikely(!chipModel || chipModel[0] == '\0')) {
				chipModel = C_("S98", "(none)");
			}

			data_row.push_back(chipID);
			data_row.push_back(chipModel);
			data_row.push_back(LibRpText::formatFrequency(le32_to_cpu(devInfo.clock)));
			if (isV3) {
				// Panning (NOTE: Adding as a hex string for now...)
				data_row.push_back(fmt::format(FSTR("0x{:0>2X}"), le32_to_cpu(devInfo.v3_pan)));
			}
		}

		static const array<const char*, 4> device_info_names = {{
			NOP_C_("S98", "Chip"),
			NOP_C_("S98", "Model"),
			NOP_C_("S98", "Clock Rate"),
			NOP_C_("S98", "Pan")	// v3 only?
		}};
		vector<string> *v_device_info_names = RomFields::strArrayToVector_i18n("S98|DeviceInfo",
			device_info_names.data(), columnCount);

		RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 0);
		params.headers = v_device_info_names;
		params.data.single = vv_deviceInfo;
		d->fields.addField_listData(C_("S98", "Device Info"), &params);
	}

	// Finished reading the field data.
	return d->fields.count();
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int S98::loadMetaData(void)
{
	RP_D(S98);
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

	// PSF header
	const S98_Header *const s98Header = &d->s98Header;
	d->metaData.reserve(9);	// Maximum of 9 metadata properties.

	// Attempt to parse the tags before doing anything else.
	const bool isV3 = (s98Header->version == '3');
	if (!isV3 || s98Header->tag_offset == 0) {
		// No tags in this file.
		return d->metaData.count();
	}

	// Tags (v3 only)
	unordered_map<string, string> tags = d->parseTags(le32_to_cpu(s98Header->tag_offset));
	if (!tags.empty()) {
		// Add the tags to the metadata properties.
		PSFTagParser::addTagsToRomMetaData(&d->metaData, tags, "s98by");
	}

	// Finished reading the metadata.
	return d->metaData.count();
}

} // namespace LibRomData
