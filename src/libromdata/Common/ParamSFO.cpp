/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ParamSFO.cpp: PlayStation PARAM.SFO / PSF reader                        *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * Copyright (c) 2026 by Emma / InvoxiPlayGames.                           *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ParamSFO.hpp"
#include "RomData_p.hpp"

#include "paramsfo_structs.h"

#include <cstdint>

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
using std::vector;
using std::unordered_map;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class ParamSFOPrivate final : public RomDataPrivate
{
public:
	explicit ParamSFOPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(ParamSFOPrivate);

public:
	// Maximum string length (TODO: Better maximum?)
	static constexpr int MAX_STRING_LENGTH = 1024;

	/**
	 * Read a NULL-terminated string.
	 * @param offset	[in] Offset in the file
	 * @param str 		[out] NULL-terminated string
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readNullTerminatedString(uint32_t offset, string &str);

	/**
	 * Read a string with an explicit length.
	 * @param offset	[in] Offset in the file
	 * @param length	[in] String length
	 * @param str 		[out] NULL-terminated string
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readString(uint32_t offset, size_t length, string &str);

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

	// PSF header
	psf_header_t fileHeader;

	// PSF key data.
	rp::uvector<psf_key_t> keys;
	unordered_map<string, psf_key_t> keyLookup;

	// Key/value lookup cache.
	unordered_map<string, string> cachedStringValues;
	unordered_map<string, uint32_t> cachedIntValues;

public:
	ParamSFO::SFOValueType getKeyValueType(const char *key) const;
	string getStringValue(const char *key);
	uint32_t getIntValue(const char *key);

	/**
	 * Get the minimum OS version.
	 * @return Minimum OS version, or empty string if not found.
	 */
	string getMinimumOSVersion(void);
};

ROMDATA_IMPL(ParamSFO)

const array<const char*, 1+1> ParamSFOPrivate::exts = {{
	".sfo",

	nullptr
}};

const array<const char*, 1+1> ParamSFOPrivate::mimeTypes = {{
	// Unofficial MIME type.
	// TODO: Get this upstreamed on FreeDesktop.org.
	"application/x-playstation-sfo",

	nullptr
}};

const RomDataInfo ParamSFOPrivate::romDataInfo = {
	"PARAM.SFO", exts.data(), mimeTypes.data()
};

ParamSFOPrivate::ParamSFOPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	memset(&fileHeader, 0, sizeof(fileHeader));
}

/**
 * Read a NULL-terminated string.
 * @param offset	[in] Offset in the file
 * @param str 		[out] NULL-terminated string
 * @return 0 on success; negative POSIX error code on error.
 */
int ParamSFOPrivate::readNullTerminatedString(uint32_t offset, string &str)
{
	str.clear();
	if (offset == 0) {
		return -ENOENT;
	}

	// Read a string.
	char buf[MAX_STRING_LENGTH];
	size_t size = file->seekAndRead(offset, buf, sizeof(buf));
	if (size == 0) {
		// Seek and/or read error.
		return -EIO;
	} else if (size > sizeof(buf)) {
		size = sizeof(buf);
	}

	// Ensure NULL termination.
	buf[size-1] = '\0';

	// Find the first NULL terminator.
	const char *pNull = static_cast<const char*>(memchr(buf, '\0', sizeof(buf)));
	if (pNull) {
		str.assign(buf, pNull - buf);
	} else {
		// Shouldn't happen...
		str.assign(buf, sizeof(buf));
	}

	return 0;
}

/**
 * Read a string with an explicit length.
 * @param offset	[in] Offset in the file
 * @param length	[in] String length
 * @param str 		[out] NULL-terminated string
 * @return 0 on success; negative POSIX error code on error.
 */
int ParamSFOPrivate::readString(uint32_t offset, size_t length, string &str)
{
	str.clear();
	if (offset == 0) {
		return -ENOENT;
	}

	// Read directly into the string buffer.
	str.resize(length);
	if (file->seekAndRead(offset, const_cast<char*>(str.data()), length) != length) {
		// Seek and/or read error.
		str.clear();
		return -EIO;
	}

	return 0;
}

ParamSFO::SFOValueType ParamSFOPrivate::getKeyValueType(const char *key) const
{
	auto iter = keyLookup.find(key);
	if (iter == keyLookup.end()) {
		// We don't have this key.
		return ParamSFO::SFOValueType::None;
	}

	const psf_key_t &psfKey = iter->second;

	ParamSFO::SFOValueType ret;
	switch (psfKey.valueType) {
		case kPSF_UTF8S:
		case kPSF_UTF8:
			ret = ParamSFO::SFOValueType::UTF8;
			break;

		case kPSF_Int32:
			ret = ParamSFO::SFOValueType::Int32;
			break;

		default:
			ret = ParamSFO::SFOValueType::None;
			break;
	}
	return ret;
}

string ParamSFOPrivate::getStringValue(const char *key)
{
	// Did we look up this key already?
	{
		auto iter = cachedStringValues.find(key);
		if (iter != cachedStringValues.end()) {
			// We already fetched a string value for this key.
			return iter->second;
		}
	}

	if (!file) {
		// File isn't open.
		return {};
	} else if (!isValid) {
		// File isn't valid
		return {};
	}

	auto iter = keyLookup.find(key);
	if (iter == keyLookup.end()) {
		// We don't have this key.
		return {};
	}

	const psf_key_t &psfKey = iter->second;
	assert(psfKey.valueType == kPSF_UTF8 || psfKey.valueType == kPSF_UTF8S);
	if (psfKey.valueType != kPSF_UTF8 && psfKey.valueType != kPSF_UTF8S) {
		// Not a UTF-8 string key.
		return {};
	}

	if (psfKey.dataLength <= 0) {
		// Empty string, or negative length?
		return {};
	}
	// Limit data length to 1,024.
	assert(psfKey.dataLength <= ParamSFOPrivate::MAX_STRING_LENGTH);
	int dataLength = psfKey.dataLength;
	if (dataLength > ParamSFOPrivate::MAX_STRING_LENGTH) {
		dataLength = ParamSFOPrivate::MAX_STRING_LENGTH;
	}

	string value;
	if (readString(fileHeader.dataOffset + psfKey.dataOffset, dataLength, value) != 0) {
		// Failed to read the value.
		return {};
	}

	// kPSF_UTF8: String should be NULL-terminated, so we'll need to
	// find the first NULL byte and terminate the string there.
	// NOTE: Some strings have a larger data length than they should...
	// Assassin's Creed - Bloodlines (Europe) (PSP) (PSN).iso: PSP_SYSTEM_VER == "5.50\0\x95"
	if (psfKey.valueType == kPSF_UTF8 && !value.empty()) {
		size_t null_pos = value.find('\0');
		if (null_pos != string::npos) {
			value.resize(null_pos);
		}
	}

	// Cache the value for later.
	cachedStringValues.emplace(key, value);
	return value;
}

uint32_t ParamSFOPrivate::getIntValue(const char *key)
{
	// TODO: Can we error out of this function?

	// Did we look up this key already?
	{
		auto iter = cachedIntValues.find(key);
		if (iter != cachedIntValues.end()) {
			// We already fetched a uint32_t value for this key.
			return iter->second;
		}
	}

	if (!file) {
		// File isn't open.
		return 0;
	} else if (!isValid) {
		// File isn't valid
		return 0;
	}

	auto iter = keyLookup.find(key);
	if (iter == keyLookup.end()) {
		// We don't have this key.
		return 0;
	}

	const psf_key_t &psfKey = iter->second;
	if (psfKey.valueType != kPSF_Int32) {
		// Not an Int32 key.
		return 0;
	}

	// Verify the int size. (Must be 4)
	assert(psfKey.dataLength == sizeof(uint32_t));
	if (psfKey.dataLength != sizeof(uint32_t)) {
		// Size is incorrect.
		return 0;
	}

	uint32_t value = 0;
	if (file->seekAndRead(fileHeader.dataOffset + psfKey.dataOffset, &value, sizeof(uint32_t)) != sizeof(uint32_t)) {
		// Failed to read the value.
		return 0;
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the value
	value = le32_to_cpu(value);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	cachedIntValues.emplace(key, value);
	return value;
}

/**
 * Get the minimum OS version.
 * @return Minimum OS version, or empty string if not found.
 */
string ParamSFOPrivate::getMinimumOSVersion(void)
{
	// Check for PSP.
	string s_systemVer = getStringValue("PSP_SYSTEM_VER");
	if (!s_systemVer.empty()) {
		return s_systemVer;
	}

	// Check for PS3.
	s_systemVer = getStringValue("PS3_SYSTEM_VER");
	if (!s_systemVer.empty()) {
		// TODO: Reformat the system version?
		// The standard format is "XX.YYYY", e.g. "03.4100", but usually
		// we don't want to show a leading 0 for major or trailing 0s for minor.
		return s_systemVer;
	}

	// Check for PSP2_SYSTEM_VER (PS Vita) and/or SYSTEM_VER (PS4).
	// These are stored as uint32_t, not strings.
	// TODO: SYSTEM_ROOT_VER - preferred over SYSTEM_VER or not? Skipping for now...
	// TODO: PSP2_SYSTEM_ROOT_VER - preferred over PSP2_SYSTEM_VER or not? Skipping for now...
	// TODO: Verify that PSP2_SYSTEM_VER uses the same format as SYSTEM_VER.
	uint32_t u_systemVer = getIntValue("PSP2_SYSTEM_VER");
	if (u_systemVer == 0) {
		// PSP2_SYSTEM_VER not found. Try SYSTEM_VER.
		u_systemVer = getIntValue("SYSTEM_VER");
	}
	if (u_systemVer >= 0x10000) {
		// For PS4 at least, only the HIWORD is relevant.
		// Also, the value is in BCD, so print it as hex.
		// FIXME: "EB0035-CUSA42526_00-DEGROIDPS4EE0000-A0102-V0100-PARAM.SFO" has 0x10508000...
		u_systemVer >>= 16;
		return fmt::format(FSTR("{:X}.{:0>2X}"), (u_systemVer >> 8) & 0xFF, u_systemVer & 0xFF);
	}

	// Minimum OS version is not available...
	return {};
}

ParamSFO::ParamSFO(const IRpFilePtr &file)
	: super(new ParamSFOPrivate(file))
{
	RP_D(ParamSFO);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PSF header.
	uint8_t header_bytes[sizeof(psf_header_t)];
	d->file->rewind();
	size_t size = d->file->read(header_bytes, sizeof(psf_header_t));
	if (size != sizeof(psf_header_t)) {
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {{0, sizeof(psf_header_t), header_bytes}, nullptr, 0};
	if (isRomSupported_static(&info) < 0) {
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Copy the header into our class.
	memcpy(&d->fileHeader, header_bytes, sizeof(psf_header_t));

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the header.
	d->fileHeader.magic = le32_to_cpu(d->fileHeader.magic);
	d->fileHeader.version = le32_to_cpu(d->fileHeader.version);
	d->fileHeader.keyOffset = le32_to_cpu(d->fileHeader.keyOffset);
	d->fileHeader.dataOffset = le32_to_cpu(d->fileHeader.dataOffset);
	d->fileHeader.numKeys = le32_to_cpu(d->fileHeader.numKeys);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Sanity check: Must have at least 1 key, and a maximum of 16,384 keys.
	// (Probably too many...)
	assert(d->fileHeader.numKeys >= 1);
	assert(d->fileHeader.numKeys <= 16384);
	if (d->fileHeader.numKeys < 1 || d->fileHeader.numKeys > 16384) {
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Read the key table.
	d->keys.resize(d->fileHeader.numKeys);
	size = d->file->read(d->keys.data(), d->keys.size() * sizeof(psf_key_t));
	if (size != d->keys.size() * sizeof(psf_key_t)) {
		d->keys.clear();
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Validate the keys.
	for (psf_key_t &key : d->keys) {
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		// Byteswap the key.
		key.keyNameOffset = le16_to_cpu(key.keyNameOffset);
		key.valueType = le16_to_cpu(key.valueType);
		key.dataLength = le32_to_cpu(key.dataLength);
		key.dataMax = le32_to_cpu(key.dataMax);
		key.dataOffset = le32_to_cpu(key.dataOffset);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

		// Ensure it's a data format we know.
		assert(key.valueType == kPSF_Int32 || key.valueType == kPSF_UTF8 || key.valueType == kPSF_UTF8S);
		if (key.valueType != kPSF_Int32 && key.valueType != kPSF_UTF8 && key.valueType != kPSF_UTF8S) {
			d->keys.clear();
			d->isValid = false;
			d->file.reset();
			return;
		}
	}

	// Read the key names.
	d->keyLookup.reserve(d->keys.size());
	for (psf_key_t key : d->keys) {
		// Read the name of the key.
		string keyName;
		if (d->readNullTerminatedString(d->fileHeader.keyOffset + key.keyNameOffset, keyName) != 0) {
			d->keys.clear();
			d->isValid = false;
			d->file.reset();
			return;
		}

		d->keyLookup.emplace(std::move(keyName), key);
	}

	d->isValid = true;

	d->fileType = FileType::ConfigurationFile;
	d->mimeType = "application/x-playstation-sfo";
}

int ParamSFO::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData || info->header.addr != 0 || info->header.size < sizeof(psf_header_t)) {
		return -1;
	}

	const psf_header_t *const psfheader = reinterpret_cast<const psf_header_t *>(info->header.pData);

	if (psfheader->magic != be32_to_cpu(PSF_MAGIC) || psfheader->version != be32_to_cpu(PSF_VERSION)) {
		return -1;
	}

	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ParamSFO::systemName(unsigned int type) const
{
	RP_D(const ParamSFO);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	static_assert(SYSNAME_TYPE_MASK == 3,
		"ParamSFO::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Sony PlayStation PARAM.SFO", "PARAM.SFO", "SFO", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return 0 on success; negative POSIX error code on error.
 */
int ParamSFO::loadFieldData(void)
{
	RP_D(ParamSFO);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Add a tab with a list of entries in the SFO file.
	// (Most of this taken from EXE_PE.cpp)
	d->fields.addTab("SFO");
	d->fields.reserve(1);
	{
		// Generate list data
		auto *const vv_data = new RomFields::ListData_t();
		vv_data->reserve(d->keys.size());
		for (auto &key : d->keyLookup) {
			vv_data->push_back(vector<string>());
			auto &row = vv_data->back();
			row.reserve(2);
			row.push_back(key.first);

			SFOValueType valueType = getKeyValueType(key.first);
			if (valueType == SFOValueType::UTF8) {
				row.push_back(getStringValue(key.first));
			} else if (valueType == SFOValueType::Int32) {
				row.push_back(fmt::format("0x{0:08X}", getIntValue(key.first)));
			} else {
				// Unknown value type?
				row.push_back(string());
			}
		}

		static const array<const char *, 2> field_names = {{
			NOP_C_("SFO", "Key"),
			NOP_C_("SFO", "Value"),
		}};
		vector<string> *const v_field_names = RomFields::strArrayToVector_i18n("SFO", field_names);

		RomFields::AFLD_PARAMS params;
		params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
		params.headers = v_field_names;
		params.data.single = vv_data;
		params.col_attrs.align_data = AFLD_ALIGN2(TXA_D, TXA_D);
		params.col_attrs.sorting = AFLD_ALIGN2(COLSORT_NC, COLSORT_NC);
		params.col_attrs.sort_col = 0; // Name
		params.col_attrs.sort_dir = RomFields::COLSORTORDER_ASCENDING;
		d->fields.addField_listData(C_("SFO", "Values"), &params);
	}

	return 0;
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int ParamSFO::loadMetaData(void)
{
	RP_D(ParamSFO);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown disc image type.
		return -EIO;
	}

	d->metaData.reserve(6);	// Maximum of 6 metadata properties.

	/**
	 * Example PSP param.sfo file:
	 * |     Key      |    Value    |
	 * |--------------|-------------|
	 * |TITLE         |Sonic Rivals™|
	 * |REGION        |0x00008000   |
	 * |PSP_SYSTEM_VER|2.81         |
	 * |PARENTAL_LEVEL|0x00000003   |
	 * |DISC_VERSION  |1.01         |
	 * |DISC_TOTAL    |0x00000001   |
	 * |DISC_NUMBER   |0x00000001   |
	 * |CATEGORY      |UG           |
	 * |DISC_ID       |ULES00622    |
	 * |BOOTABLE      |0x00000001   |
	 */
	// TODO: Check PS2, PS3, PS Vita, PS4, PS5?

	// Title
	const string title = getStringValue("TITLE");
	if (!title.empty()) {
		d->metaData.addMetaData_string(Property::Title, title);
	}

	// TODO: Figure out the PARENTAL_LEVEL field.
	// TODO: Add a metadata property for revision / disc version.

	// Publisher
	const string publisher = getStringValue("PROVIDER");
	if (!publisher.empty()) {
		d->metaData.addMetaData_string(Property::Publisher, publisher);
	}

	// Disc number (if multi-disc)
	const uint32_t discTotal = getIntValue("DISC_TOTAL");
	if (discTotal > 1) {
		// Multiple discs!
		const uint32_t discNumber = getIntValue("DISC_NUMBER");
		if (discNumber >= 1) {
			d->metaData.addMetaData_uint(Property::DiscNumber, discNumber);
		}
	}

	// TODO: Figure out the CATEGORY field.
	// TODO: Figure out the BOOTABLE field.

	/** Custom properties! **/

	// Game ID
	string gameID = getStringValue("DISC_ID");
	if (!gameID.empty()) {
		// Add a '-' between the last letter and the first number.
		string gameID_dash;
		gameID_dash.reserve(gameID.size() + 1);
		for (size_t i = 0; i < gameID.size(); i++) {
			if (isalpha_ascii(gameID[i])) {
				// Still a letter.
				gameID_dash += gameID[i];
			} else {
				// Not a letter.
				// Add a dash and then the rest of the game ID.
				gameID_dash += '-';
				gameID_dash += gameID.substr(i);
				break;
			}
		}

		d->metaData.addMetaData_string(Property::GameID, gameID_dash);
	}

	// Title ID
	string titleID = getStringValue("TITLE_ID");
	if (!titleID.empty()) {
		// Add a '-' between the last letter and the first number.
		string titleID_dash;
		titleID_dash.reserve(titleID.size() + 1);
		for (size_t i = 0; i < titleID.size(); i++) {
			if (isalpha_ascii(titleID[i])) {
				// Still a letter.
				titleID_dash += titleID[i];
			} else {
				// Not a letter.
				// Add a dash and then the rest of the game ID.
				titleID_dash += '-';
				titleID_dash += titleID.substr(i);
				break;
			}
		}

		d->metaData.addMetaData_string(Property::TitleID, titleID_dash);
	}

	// OS Version
	string s_osVersion = d->getMinimumOSVersion();
	if (!s_osVersion.empty()) {
		d->metaData.addMetaData_string(Property::OSVersion, s_osVersion);
	}

	// Version
	// Prefer APP_VER if it's available; otherwise, use DISC_VERSION.
	string s_version = getStringValue("APP_VER");
	if (s_version.empty()) {
		s_version = getStringValue("DISC_VERSION");
	}
	if (!s_version.empty()) {
		d->metaData.addMetaData_string(Property::Version, s_version);
	}

	// Finished reading the metadata.
	return d->metaData.count();
}

ParamSFO::SFOValueType ParamSFO::getKeyValueType(const char *key) const
{
	RP_D(const ParamSFO);
	return d->getKeyValueType(key);
}

string ParamSFO::getStringValue(const char *key)
{
	RP_D(ParamSFO);
	return d->getStringValue(key);
}

uint32_t ParamSFO::getIntValue(const char *key)
{
	RP_D(ParamSFO);
	return d->getIntValue(key);
}

} //namespace LibRomData
