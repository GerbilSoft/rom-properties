/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSettingTxt.hpp: Nintendo Wii setting.txt file reader.                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiSettingTxt.hpp"

#include "ini.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// C++ STL classes
using std::array;
using std::map;
using std::pair;
using std::string;
using std::vector;

namespace LibRomData {

class WiiSettingTxtPrivate final : public RomDataPrivate
{
public:
	explicit WiiSettingTxtPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiSettingTxtPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// setting.txt (INI data)
	vector<pair<string, string >> setting_txt;

	/**
	 * ini.h callback for parsing setting.txt.
	 * @param user		[in] User data parameter (this)
	 * @param section	[in] Section name
	 * @param name		[in] Value name
	 * @param value		[in] Value
	 * @return 0 to continue; 1 to stop.
	 */
	static int parse_setting_txt(void *user, const char *section, const char *name, const char *value);
};

ROMDATA_IMPL(WiiSettingTxt)

/** WiiSettingTxtPrivate **/

/* RomDataInfo */
// NOTE: This will be handled using the same
// settings as WiiSave.
const array<const char*, 1+1> WiiSettingTxtPrivate::exts = {{
	".txt",		// NOTE: Conflicts with plain text files.

	nullptr
}};
const array<const char*, 1+1> WiiSettingTxtPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-wii-setting-txt",

	nullptr
}};
const RomDataInfo WiiSettingTxtPrivate::romDataInfo = {
	"WiiSettingTxt", exts.data(), mimeTypes.data()
};

WiiSettingTxtPrivate::WiiSettingTxtPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{ }

/**
 * ini.h callback for parsing setting.txt.
 * @param user		[in] User data parameter (this)
 * @param section	[in] Section name
 * @param name		[in] Value name
 * @param value		[in] Value
 * @return 0 to continue; 1 to stop.
 */
int WiiSettingTxtPrivate::parse_setting_txt(void *user, const char *section, const char *name, const char *value)
{
	if (section[0] != '\0') {
		// Sections aren't expected here...
		return 1;
	}

	WiiSettingTxtPrivate *const d = static_cast<WiiSettingTxtPrivate*>(user);

	// If "VIDEO=PAL", set isPAL.
	if (!strcmp(name, "VIDEO")) {
		d->isPAL = (!strcmp(value, "PAL"));
	}

	// Save the value in the vector.
	// NOTE: Not using a map<> because we want to show the keys as-is.
	d->setting_txt.push_back(std::make_pair(name, value));
	return 0;
}

/** WiiSettingTxt **/

/**
 * Read a Nintendo Wii setting.txt file.
 *
 * A save file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
WiiSettingTxt::WiiSettingTxt(const IRpFilePtr &file)
	: super(new WiiSettingTxtPrivate(file))
{
	// This class handles configuration files.
	RP_D(WiiSettingTxt);
	d->mimeType = "application/x-wii-setting-txt";	// unofficial, not on fd.o
	d->fileType = FileType::ConfigurationFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// File must be exactly 256 bytes.
	if (d->file->size() != 256) {
		// Wrong size.
		d->file.reset();
		return;
	}

	// Read the entire file.
	uint8_t buf[256 + 1];
	d->file->rewind();
	size_t size = d->file->read(buf, sizeof(buf) - 1);
	if (size != sizeof(buf) - 1) {
		d->file.reset();
		return;
	}
	buf[sizeof(buf) - 1] = 0;	// ensure NULL termination if all 256 bytes are used

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(buf) - 1, buf},
		nullptr,	// ext (not needed for WiiSettingTxt)
		sizeof(buf) - 1	// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}

	// Search for the first non-NULL byte at the end of the buffer.
	size_t buf_len = sizeof(buf) - 1;
	while (buf_len > 0) {
		if (buf[buf_len - 1] != 0) {
			// Found the first non-NULL byte.
			break;
		}

		buf_len--;
	}
	if (buf_len == 0) {
		// Entire buffer is NULL.
		d->file.reset();
		return;
	}

	// Decrypt the buffer.
	uint32_t key = 0x73B5DBFA;
	uint8_t *const p_end = &buf[buf_len];
	for (uint8_t *p = buf; p < p_end; p++) {
		*p ^= static_cast<uint8_t>(key & 0xFF);
		key = (key << 1) | (key >> 31);
	}

	// Parse the buffer as an INI file.
	// TODO: Fail on error?
	ini_parse_string(reinterpret_cast<const char*>(buf), d->parse_setting_txt, d);
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiSettingTxt::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 256)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// The first line in setting.txt is "AREA=".
	// When encrypted, this corresponds to: BB A6 AC 92
	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);
	if (be32_to_cpu(pData32[0]) == 0xBBA6AC92) {
		// Found "AREA=".
		return 0;
	}

	// Not suported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiSettingTxt::systemName(unsigned int type) const
{
	RP_D(const WiiSettingTxt);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Wii has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiSettingTxt::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		// TODO: Check for Wii U serial numbers?
		"Nintendo Wii", "Wii", "Wii", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiSettingTxt::loadFieldData(void)
{
	RP_D(WiiSettingTxt);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// No file.
		// A closed file is OK, since we already loaded the header.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown save banner file type.
		return -EIO;
	}

	// Add the fields from setting.txt directly.
	// TODO: Can we do any sort of useful parsing?
	d->fields.reserve(d->setting_txt.size());
	for (const auto &p : d->setting_txt) {
		d->fields.addField_string(p.first.c_str(), p.second);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

} // namespace LibRomData
