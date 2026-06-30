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

// C++ STL classes
using std::array;
using std::string;
using std::vector;
#include <unordered_map>
using std::unordered_map;

namespace LibRomData {

class ParamSFOPrivate final : public RomDataPrivate
{
public:
	explicit ParamSFOPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(ParamSFOPrivate);

	int readNullTerminatedString(uint32_t offset, std::string &str);
	int readString(uint32_t offset, size_t length, std::string &str);

public:
	/** RomDataInfo **/
	static const array<const char *, 1 + 1> exts;
	static const array<const char *, 1 + 1> mimeTypes;
	static const RomDataInfo romDataInfo;

	// PSF header.
	psf_header_t fileHeader;

	// PSF key data.
	rp::uvector<psf_key_t> keys;
	unordered_map<std::string, psf_key_t> keyLookup;

	// Key/value lookup cache.
	unordered_map<std::string, std::string> cachedStringValues;
	unordered_map<std::string, uint32_t> cachedIntValues;

	bool isBigEndian;
};

ROMDATA_IMPL(ParamSFO)

const array<const char *, 1 + 1> ParamSFOPrivate::exts = {{".sfo", nullptr}};

const array<const char *, 1 + 1> ParamSFOPrivate::mimeTypes = {{// Unofficial MIME type.
	// TODO: Get this upstreamed on FreeDesktop.org.
	"application/x-playstation-sfo", nullptr}};

const RomDataInfo ParamSFOPrivate::romDataInfo = {"PARAM.SFO", exts.data(), mimeTypes.data()};

int ParamSFOPrivate::readNullTerminatedString(uint32_t offset, std::string &str)
{
	if (offset == 0)
		return 0;

	if (file->seek(offset) != 0)
		return -1; // failed to seek
	// really bad, loop a read of a single byte til we reach a NULL
	char c = 0;
	do {
		if (file->read(&c, sizeof(c)) != sizeof(c))
			return -2; // failed to read
		if (c != 0)
			str.push_back(c);
	} while (c != 0);
	return 1;
}

int ParamSFOPrivate::readString(uint32_t offset, size_t length, std::string &str)
{
	if (offset == 0)
		return 0;

	// resize the string buffer
	str.resize(length);

	if (file->seekAndRead(offset, str.data(), length) != length)
		return -1; // failed

	return 1;
}

ParamSFOPrivate::ParamSFOPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	memset(&fileHeader, 0, sizeof(fileHeader));
	isBigEndian = false;
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
#endif

	// Read the key table.
	d->keys.resize(d->fileHeader.numKeys);
	for (int i = 0; i < d->fileHeader.numKeys; i++) {
		psf_key_t key;
		size = d->file->read(&key, sizeof(psf_key_t));
		if (size != sizeof(psf_key_t)) {
			d->isValid = false;
			d->file.reset();
			return;
		}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		// Byteswap the key.
		key.keyNameOffset = le16_to_cpu(key.keyNameOffset);
		key.valueType = le16_to_cpu(key.valueType);
		key.dataLength = le32_to_cpu(key.dataLength);
		key.dataMax = le32_to_cpu(key.dataMax);
		key.dataOffset = le32_to_cpu(key.dataOffset);
#endif

		// Ensure it's a data format we know.
		assert(key.valueType == kPSF_Int32 || key.valueType == kPSF_UTF8);
		if (key.valueType != kPSF_Int32 && key.valueType != kPSF_UTF8) {
			d->isValid = false;
			d->file.reset();
			return;
		}

		d->keys[i] = key;
	}

	// Read the key names.
	d->keyLookup.reserve(d->keys.size());
	for (psf_key_t key : d->keys) {
		// Read the name of the key.
		std::string keyName;
		if (!d->readNullTerminatedString(d->fileHeader.keyOffset + key.keyNameOffset, keyName)) {
			d->isValid = false;
			d->file.reset();
			return;
		}

		d->keyLookup[keyName] = key;
	}

	d->isValid = true;

	d->fileType = FileType::ConfigurationFile;
	d->mimeType = "application/x-playstation-sfo";
}

ParamSFO::SFOValueType ParamSFO::getKeyValueType(std::string key)
{
	RP_D(ParamSFO);

	if (!d->keyLookup.contains(key)) {
		return SFOValueType::None;
	}

	psf_key_t *psfKey = &d->keyLookup[key];

	if (psfKey->valueType == kPSF_UTF8) {
		return SFOValueType::UTF8;
	} else if (psfKey->valueType == kPSF_Int32) {
		return SFOValueType::Int32;
	} else {
		return SFOValueType::None;
	}
}

const std::string ParamSFO::getStringValue(std::string key)
{
	// TODO: Can we error out of this function?

	RP_D(ParamSFO);
	if (d->cachedStringValues.contains(key)) {
		// We already fetched a string value for that key.
		return d->cachedStringValues[key];
	} else if (!d->file) {
		// File isn't open.
		return string();
	} else if (!d->isValid) {
		// File isn't valid
		return string();
	} else if (!d->keyLookup.contains(key)) {
		// We don't have this key
		return string();
	}

	psf_key_t *psfKey = &d->keyLookup[key];
	if (psfKey->valueType != kPSF_UTF8) {
		// Not a UTF8 key.
		return string();
	}

	std::string value;
	// HACK: We take 1 off the data length to trim null terminators.
	if (!d->readString(d->fileHeader.dataOffset + psfKey->dataOffset, psfKey->dataLength - 1, value)) {
		// Failed to read the value.
		return string();
	}

	// Cache the value for later.
	d->cachedStringValues[key] = value;

	return value;
}

uint32_t ParamSFO::getIntValue(std::string key)
{
	// TODO: Can we error out of this function?
	RP_D(ParamSFO);
	if (d->cachedIntValues.contains(key)) {
		// We already fetched a string value for that key.
		return d->cachedIntValues[key];
	} else if (!d->file) {
		// File isn't open.
		return 0;
	} else if (!d->isValid) {
		// File isn't valid
		return 0;
	} else if (!d->keyLookup.contains(key)) {
		// We don't have this key
		return 0;
	}

	psf_key_t *psfKey = &d->keyLookup[key];
	if (psfKey->valueType != kPSF_Int32) {
		// Not an Int32 key.
		return 0;
	}

	uint32_t value = 0;
	if (d->file->seekAndRead(d->fileHeader.dataOffset + psfKey->dataOffset, &value, sizeof(uint32_t)) !=
		sizeof(uint32_t)) {
		// Failed to read the value.
		return 0;
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the value
	value = le32_to_cpu(value);
#endif

	d->cachedIntValues[key] = value;

	return value;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ParamSFO::systemName(unsigned int type) const
{
	RP_D(const ParamSFO);

	static_assert(SYSNAME_TYPE_MASK == 3, "ParamSFO::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {"Sony PlayStation PARAM.SFO", "PARAM.SFO", "SFO", nullptr};

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

} //namespace LibRomData