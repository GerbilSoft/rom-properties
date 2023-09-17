/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AmiiboData.cpp: Nintendo amiibo identification data.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.libromdata.h"
#include "AmiiboData.hpp"

#include "byteswap_rp.h"
#include "tcharx.h"
#include "../../amiibo-data/amiibo_bin_structs.h"

// OS-specific includes
#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include "librptext/wchar.hpp"
#endif /* _WIN32 */

// Other rom-properties libraries
#include "librpfile/FileSystem.hpp"
#include "librpfile/RpFile.hpp"
using namespace LibRpFile;

// C++ includes
using std::string;
using std::tstring;
using std::unique_ptr;

// Mini-U82T()
#ifdef _WIN32
#  include "librptext/wchar.hpp"
#else /* !_WIN32 */
#  define U82T_s(u8str) (u8str)
#endif /* _WIN32 */

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

/**
 * References:
 * - https://www.3dbrew.org/wiki/Amiibo
 * - https://www.reddit.com/r/amiibo/comments/38hwbm/nfc_character_identification_my_findings_on_the/
 * - https://docs.google.com/spreadsheets/d/19E7pMhKN6x583uB6bWVBeaTMyBPtEAC-Bk59Y6cfgxA/
 */

/**
 * amiibo ID format
 * Two 4-byte pages starting at page 21 (raw offset 0x54).
 * Format: ssscvvtt-aaaaSS02
 * - sssc: Character series and ID.
 *         Series is bits 54-63.
 *         Character is bits 48-53.
 *         This allows up to 64 characters per series.
 *         Some series, e.g. Pokemon, has multiple series
 *         identifiers reserved.
 * - vv: Character variation.
 * - tt: Type. 00 = figure, 01 == card, 02 == plush (yarn)
 * - aaaa: amiibo ID. (Unique across all amiibo.)
 * - SS: amiibo series.
 * - 02: Always 02.
 */

class AmiiboDataPrivate {
	public:
		AmiiboDataPrivate();

	private:
		RP_DISABLE_COPY(AmiiboDataPrivate)

	public:
		// Static AmiiboData instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static AmiiboData instance;

	public:
		// amiibo.bin data
		rp::uvector<uint8_t> amiibo_bin_data;

		// Convenience pointers to amiibo.bin structs.
		const AmiiboBinHeader *pHeader;
		const char *pStrTbl;
		const uint32_t *pCSeriesTbl;
		const CharTableEntry *pCharTbl;
		const CharVariantTableEntry *pCharVarTbl;
		const uint32_t *pASeriesTbl;
		const AmiiboIDTableEntry *pAmiiboIDTbl;

		// Cached table lengths.
		uint32_t strTbl_len;
		// Cached table counts. (divided by sizeof each element)
		uint32_t cseriesTbl_count;
		uint32_t charTbl_count;
		uint32_t charVarTbl_count;
		uint32_t aseriesTbl_count;
		uint32_t amiiboIdTbl_count;

		// amiibo.bin timestamps
		time_t amiibo_bin_check_ts;	// Last check timestamp
		time_t amiibo_bin_file_ts;	// File mtime

		enum class AmiiboBinFileType : uint8_t {
			None = 0,
			System = 1,
			User = 2,
		};
		AmiiboBinFileType amiiboBinFileType;

	private:
		/**
		 * Get an amiibo-data.bin filename.
		 * @param amiiboBinFileType AmiiboBinFileType
		 * @return amiibo-data.bin filename, or empty string on error.
		 */
		tstring getAmiiboBinFilename(AmiiboBinFileType amiiboBinFileType) const;

	public:
		/**
		 * Load amiibo-data.bin if it's needed.
		 * @return 0 on success or if load isn't needed; negative POSIX error code on error.
		 */
		int loadIfNeeded(void);

	private:
		/**
		 * Clear the loaded data.
		 */
		void clear(void);

	public:
		/**
		 * String table lookup.
		 * @param idx String table index.
		 * @return String, or nullptr on error.
		 */
		inline const char *strTbl_lookup(uint32_t idx) const;
};

// amiibo-data.bin filename
#define AMIIBO_BIN_FILENAME "amiibo-data.bin"

AmiiboDataPrivate::AmiiboDataPrivate()
	: pHeader(nullptr)
	, pStrTbl(nullptr)
	, pCSeriesTbl(nullptr)
	, pCharTbl(nullptr)
	, pCharVarTbl(nullptr)
	, pASeriesTbl(nullptr)
	, pAmiiboIDTbl(nullptr)
	, strTbl_len(0)
	, cseriesTbl_count(0)
	, charTbl_count(0)
	, charVarTbl_count(0)
	, aseriesTbl_count(0)
	, amiiboIdTbl_count(0)
	, amiibo_bin_check_ts(-1)
	, amiibo_bin_file_ts(-1)
	, amiiboBinFileType(AmiiboBinFileType::None)
{
	// Loading amiibo-data.bin will be delayed until it's needed.
}

/**
 * Get an amiibo-data.bin filename.
 * @param amiiboBinFileType AmiiboBinFileType
 * @return amiibo-data.bin filename, or empty string on error.
 */
tstring AmiiboDataPrivate::getAmiiboBinFilename(AmiiboBinFileType amiiboBinFileType) const
{
	tstring tfilename;

	switch (amiiboBinFileType) {
		default:
		case AmiiboBinFileType::None:
			break;

		case AmiiboBinFileType::System: {
#if defined(DIR_INSTALL_SHARE)
			tfilename = _T(DIR_INSTALL_SHARE) _T(DIR_SEP_STR) _T(AMIIBO_BIN_FILENAME);
#elif defined(_WIN32)
			TCHAR dll_filename[MAX_PATH];
			SetLastError(ERROR_SUCCESS);	// required for XP
			DWORD dwResult = GetModuleFileName(HINST_THISCOMPONENT,
				dll_filename, _countof(dll_filename));
			if (dwResult == 0 || dwResult >= _countof(dll_filename) || GetLastError() != ERROR_SUCCESS) {
				// Cannot get the DLL filename.
				// TODO: Windows XP doesn't SetLastError() if the
				// filename is too big for the buffer.
				break;
			}

			// Remove the last backslash.
			tfilename.assign(dll_filename);
			size_t bs_pos = tfilename.rfind(DIR_SEP_CHR);
			if (bs_pos == string::npos) {
				// No backslash...
				tfilename.clear();
				break;
			}
			tfilename.resize(bs_pos+1);
			tfilename += _T(AMIIBO_BIN_FILENAME);
			if (GetFileAttributes(tfilename.c_str()) != INVALID_FILE_ATTRIBUTES) {
				// Found the amiibo.bin file.
				break;
			}

			// Not in the DLL directory.
			// Check parent directory.
			tfilename.resize(bs_pos);
			bs_pos = tfilename.rfind(DIR_SEP_CHR);
			if (bs_pos == string::npos) {
				// No backslash...
				tfilename.clear();
				break;
			}
			tfilename.resize(bs_pos+1);
			tfilename += _T(AMIIBO_BIN_FILENAME);
			if (GetFileAttributes(tfilename.c_str()) != INVALID_FILE_ATTRIBUTES) {
				// Found the amiibo.bin file.
				break;
			}

			// Not found...
			tfilename.clear();
#endif
			break;
		}

		case AmiiboBinFileType::User:
			tfilename = U82T_s(FileSystem::getConfigDirectory());
			if (!tfilename.empty()) {
				if (tfilename.at(tfilename.size()-1) != DIR_SEP_CHR) {
					tfilename += DIR_SEP_CHR;
				}
			}
			tfilename += _T(AMIIBO_BIN_FILENAME);
			break;
	}

	return tfilename;
}

/**
 * Load amiibo.bin if it's needed.
 * @return 0 on success or if load isn't needed; negative POSIX error code on error.
 */
int AmiiboDataPrivate::loadIfNeeded(void)
{
	// Determine the amiibo-data.bin file to load.
	tstring tfilename;

	const time_t now = time(nullptr);
	if (!amiibo_bin_data.empty()) {
		// amiibo data is already loaded.
		if (now == amiibo_bin_check_ts) {
			// Same time as last time. (seconds resolution)
			return 0;
		}
	}

	// Check the following paths:
	// - ~/.config/rom-properties/amiibo-data.bin (user override)
	// - /usr/share/rom-properties/amiibo-data.bin (system-wide)

	// NOTE: mtime is checked even if no file is loaded, since this is
	// also used to check if the file exists in the first place.
	AmiiboBinFileType bin_ft = AmiiboBinFileType::None;
	time_t mtime = -1;
	bool ok = false;	// Set to true once a valid file is found.

	// Check the user filename.
	tfilename = getAmiiboBinFilename(AmiiboBinFileType::User);
	if (!tfilename.empty()) {
		// Check the mtime to see if we need to reload it.
		int ret = FileSystem::get_mtime(tfilename, &mtime);
		if (ret == 0) {
			if (mtime == this->amiibo_bin_file_ts) {
				// User file exists, and the mtime matches the previous mtime.
				if (this->amiiboBinFileType == AmiiboBinFileType::User) {
					// Same filetype. No reload is necessary.
					return 0;
				}
			}

			// mtime and/or type doesn't match.
			// Reload is required.
			bin_ft = AmiiboBinFileType::User;
			ok = true;
		}
	}

	if (!ok) {
		// Check the system filename.
		tfilename = getAmiiboBinFilename(AmiiboBinFileType::System);
		if (!tfilename.empty()) {
			// Check the mtime to see if we need to reload it.
			// TODO: wchar_t* overload.
			int ret = FileSystem::get_mtime(tfilename, &mtime);
			if (ret == 0) {
				if (mtime == this->amiibo_bin_file_ts) {
					// User file exists, and the mtime matches the previous mtime.
					if (this->amiiboBinFileType == AmiiboBinFileType::System) {
						// Same filetype. No reload is necessary.
						return 0;
					}
				}

				// mtime and/or type doesn't match.
				// Reload is required.
				bin_ft = AmiiboBinFileType::System;
				ok = true;
			}
		}
	}

	if (!ok) {
		// Unable to find any valid amiibo-data.bin file.
		// If data was already loaded before, keep using it.
		return (amiibo_bin_data.empty() ? -ENOENT : 0);
	}

	// Load amiibo.bin.
	unique_ptr<RpFile> f_amiibo_bin(new RpFile(tfilename, RpFile::FM_OPEN_READ));
	if (!f_amiibo_bin->isOpen()) {
		// Unable to open the file.
		int err = -f_amiibo_bin->lastError();
		return err;
	}

	// Make sure the file is larger than sizeof(AmiiboBinHeader)
	// and it's under 1 MB.
	const off64_t fileSize_o = f_amiibo_bin->size();
	if (fileSize_o < (off64_t)sizeof(AmiiboBinHeader) ||
	    fileSize_o >= 1024*1024)
	{
		// Over 1 MB.
		return -ENOMEM;
	}

	// Clear all offsets before loading the data.
	clear();

	// Load the data.
	const size_t fileSize = static_cast<size_t>(fileSize_o);
	amiibo_bin_data.resize(fileSize);
	size_t size = f_amiibo_bin->read(amiibo_bin_data.data(), fileSize);
	if (size != fileSize) {
		// Read error.
		int err = -f_amiibo_bin->lastError();
		if (err == 0) {
			err = -EIO;
		}
		amiibo_bin_data.clear();
		return err;
	}
	f_amiibo_bin.reset(nullptr);

	// Verify the header.
	const AmiiboBinHeader *const pHeader_tmp =
		reinterpret_cast<const AmiiboBinHeader*>(&amiibo_bin_data[0]);
	if (memcmp(pHeader_tmp->magic, AMIIBO_BIN_MAGIC, sizeof(pHeader_tmp->magic)) != 0) {
		// Invalid magic.
		amiibo_bin_data.clear();
		return -EIO;
	}

	// Validate offsets.
	const uint32_t strtbl_offset = le32_to_cpu(pHeader_tmp->strtbl_offset);
	const uint32_t strtbl_len = le32_to_cpu(pHeader_tmp->strtbl_len);
	if (strtbl_offset < sizeof(AmiiboBinHeader) || strtbl_len == 0 ||
	    ((uint64_t)strtbl_offset + (uint64_t)strtbl_len) > static_cast<uint64_t>(fileSize))
	{
		// String table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	// Make sure the string table both starts and ends with NULL.
	if (amiibo_bin_data[strtbl_offset] != 0 ||
	    amiibo_bin_data[strtbl_offset + strtbl_len - 1] != 0)
	{
		// Missing NULLs.
		amiibo_bin_data.clear();
		return -EIO;
	}

	// Validate other offsets.
	const uint32_t cseries_offset = le32_to_cpu(pHeader_tmp->cseries_offset);
	const uint32_t cseries_len = le32_to_cpu(pHeader_tmp->cseries_len);
	if (cseries_offset < sizeof(AmiiboBinHeader) || cseries_len == 0 ||
	    cseries_len % sizeof(uint32_t) != 0 ||
	    ((uint64_t)cseries_offset + (uint64_t)cseries_len) > static_cast<uint64_t>(fileSize))
	{
		// p.21 character series table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t char_offset = le32_to_cpu(pHeader_tmp->char_offset);
	const uint32_t char_len = le32_to_cpu(pHeader_tmp->char_len);
	if (char_offset < sizeof(AmiiboBinHeader) || char_len == 0 ||
	    char_len % sizeof(CharTableEntry) != 0 ||
	    ((uint64_t)char_offset + (uint64_t)char_len) > static_cast<uint64_t>(fileSize))
	{
		// p.21 character table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t cvar_offset = le32_to_cpu(pHeader_tmp->cvar_offset);
	const uint32_t cvar_len = le32_to_cpu(pHeader_tmp->cvar_len);
	if (cvar_offset < sizeof(AmiiboBinHeader) || cvar_len == 0 ||
	    cvar_len % sizeof(CharVariantTableEntry) != 0 ||
	    ((uint64_t)cvar_offset + (uint64_t)cvar_len) > static_cast<uint64_t>(fileSize))
	{
		// p.21 character variant table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t aseries_offset = le32_to_cpu(pHeader_tmp->aseries_offset);
	const uint32_t aseries_len = le32_to_cpu(pHeader_tmp->aseries_len);
	if (aseries_offset < sizeof(AmiiboBinHeader) || aseries_len == 0 ||
	    aseries_len % sizeof(uint32_t) != 0 ||
	    ((uint64_t)aseries_offset + (uint64_t)aseries_len) > static_cast<uint64_t>(fileSize))
	{
		// p.22 amiibo series table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t amiibo_offset = le32_to_cpu(pHeader_tmp->amiibo_offset);
	const uint32_t amiibo_len = le32_to_cpu(pHeader_tmp->amiibo_len);
	if (amiibo_offset < sizeof(AmiiboBinHeader) || amiibo_len == 0 ||
	    amiibo_len % sizeof(AmiiboIDTableEntry) != 0 ||
	    ((uint64_t)amiibo_offset + (uint64_t)amiibo_len) > static_cast<uint64_t>(fileSize))
	{
		// p.22 amiibo ID table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	// Save the updated timestamp and file type.
	amiibo_bin_check_ts = now;
	amiibo_bin_file_ts = mtime;
	amiiboBinFileType = bin_ft;

	// Save the table values and offsets.
	pStrTbl = reinterpret_cast<const char*>(&amiibo_bin_data[strtbl_offset]);
	pCSeriesTbl = reinterpret_cast<const uint32_t*>(&amiibo_bin_data[cseries_offset]);
	pCharTbl = reinterpret_cast<const CharTableEntry*>(&amiibo_bin_data[char_offset]);
	pCharVarTbl = reinterpret_cast<const CharVariantTableEntry*>(&amiibo_bin_data[cvar_offset]);
	pASeriesTbl = reinterpret_cast<const uint32_t*>(&amiibo_bin_data[aseries_offset]);
	pAmiiboIDTbl = reinterpret_cast<const AmiiboIDTableEntry*>(&amiibo_bin_data[amiibo_offset]);
	strTbl_len = strtbl_len;
	cseriesTbl_count = cseries_len / sizeof(uint32_t);
	charTbl_count = char_len / sizeof(CharTableEntry);
	charVarTbl_count = cvar_len / sizeof(CharVariantTableEntry);
	aseriesTbl_count = aseries_len / sizeof(uint32_t);
	amiiboIdTbl_count = amiibo_len / sizeof(AmiiboIDTableEntry);
	return 0;
}

/**
 * Clear the loaded data.
 */
void AmiiboDataPrivate::clear(void)
{
	amiibo_bin_check_ts = -1;
	amiibo_bin_file_ts = -1;
	amiiboBinFileType = AmiiboBinFileType::None;

	pHeader = nullptr;
	pStrTbl = nullptr;
	pCSeriesTbl = nullptr;
	pCharTbl = nullptr;
	pCharVarTbl = nullptr;
	pASeriesTbl = nullptr;
	pAmiiboIDTbl = nullptr;
	strTbl_len = 0;
	cseriesTbl_count = 0;
	charTbl_count = 0;
	charVarTbl_count = 0;
	aseriesTbl_count = 0;
	amiiboIdTbl_count = 0;
}

/**
 * String table lookup.
 * @param idx String table index.
 * @return String, or nullptr on error.
 */
inline const char *AmiiboDataPrivate::strTbl_lookup(uint32_t idx) const
{
	assert(strTbl_len > 0);
	assert(idx < strTbl_len);
	if (strTbl_len <= 0 || idx == 0 || idx >= strTbl_len)
		return nullptr;
	return &pStrTbl[idx];
}

/** AmiiboData **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
AmiiboData AmiiboDataPrivate::instance;

AmiiboData::AmiiboData()
	: d_ptr(new AmiiboDataPrivate())
{
	// amiibo data is loaded on demand.
}

AmiiboData::~AmiiboData()
{
	delete d_ptr;
	d_ptr = nullptr;
}

/**
 * Get the AmiiboData instance.
 *
 * This automatically initializes the object and
 * reloads the amiibo data if it has been modified.
 *
 * @return AmiiboData instance.
 */
AmiiboData *AmiiboData::instance(void)
{
	// Initialize the singleton instance.
	AmiiboData *const q = &AmiiboDataPrivate::instance;

	// amiibo data is loaded on demand.

	// Return the singleton instance.
	return q;
}

/**
 * Look up a character series name.
 * @param char_id Character ID. (Page 21) [must be host-endian]
 * @return Character series name, or nullptr if not found.
 */
const char *AmiiboData::lookup_char_series_name(uint32_t char_id) const
{
	RP_D(AmiiboData);
	if (d->loadIfNeeded() != 0)
		return nullptr;

	const unsigned int cseries_id = (char_id >> 22) & 0x3FF;
	if (cseries_id >= d->cseriesTbl_count)
		return nullptr;
	return d->strTbl_lookup(le32_to_cpu(d->pCSeriesTbl[cseries_id]));
}

/**
 * Look up a character's name.
 * @param char_id Character ID. (Page 21) [must be host-endian]
 * @return Character name. (If variant, the variant name is used.)
 * If an invalid character ID or variant, nullptr is returned.
 */
const char *AmiiboData::lookup_char_name(uint32_t char_id) const
{
	RP_D(AmiiboData);
	if (d->loadIfNeeded() != 0)
		return nullptr;

	const uint32_t id = ((char_id >> 16) & 0xFFFF);

	// Do a binary search.
	//const CharTableEntry key = {id, 0};
	const CharTableEntry *const pCharTblEnd = d->pCharTbl + d->charTbl_count;
	auto pCTEntry = std::lower_bound(d->pCharTbl, pCharTblEnd, id,
		[](const CharTableEntry &entry, uint32_t id2) noexcept -> bool {
			const uint32_t id1 = le32_to_cpu(entry.char_id) & ~CHARTABLE_VARIANT_FLAG;
			return (id1 < id2);
		});
	if (pCTEntry == pCharTblEnd || (le32_to_cpu(pCTEntry->char_id) & ~CHARTABLE_VARIANT_FLAG) != id) {
		// Character ID not found.
		return nullptr;
	}

	// Check for variants.
	const char *name = nullptr;
	if (pCTEntry->char_id & cpu_to_le32(CHARTABLE_VARIANT_FLAG)) {
		// Do a binary search in the character variant table.
		const uint16_t cv_char_id = ((char_id >> 16) & 0xFFFF);
		const uint8_t variant_id = (char_id >> 8) & 0xFF;
		const CharVariantTableEntry key = {cv_char_id, variant_id, 0, 0};

		const CharVariantTableEntry *const pCharVarTblEnd = d->pCharVarTbl + d->charVarTbl_count;
		auto pCVTEntry = std::lower_bound(d->pCharVarTbl, pCharVarTblEnd, key,
			[](const CharVariantTableEntry &key1, const CharVariantTableEntry &key2) noexcept -> bool {
				// Compare the character ID first.
				if (le16_to_cpu(key1.char_id) < key2.char_id)
					return true;

				// Compare the variant ID.
				return (key1.var_id < key2.var_id);
			});

		if (pCVTEntry != pCharVarTblEnd &&
		    le16_to_cpu(pCVTEntry->char_id) == cv_char_id &&
		    pCVTEntry->var_id == variant_id)
		{
			// Character variant ID found.
			name = d->strTbl_lookup(le32_to_cpu(pCVTEntry->name));
		} else {
			// Character variant ID not found.
			// Maybe it's an error...
			// Default to the main character name.
			name = d->strTbl_lookup(le32_to_cpu(pCTEntry->name));
		}
	} else {
		// No variants.
		name = d->strTbl_lookup(le32_to_cpu(pCTEntry->name));
	}

	return name;
}

/**
 * Look up an amiibo series name.
 * @param amiibo_id	[in] amiibo ID. (Page 22) [must be host-endian]
 * @return amiibo series name, or nullptr if not found.
 */
const char *AmiiboData::lookup_amiibo_series_name(uint32_t amiibo_id) const
{
	RP_D(AmiiboData);
	if (d->loadIfNeeded() != 0)
		return nullptr;

	const unsigned int aseries_id = (amiibo_id >> 8) & 0xFF;
	if (aseries_id >= d->aseriesTbl_count)
		return nullptr;

	return d->strTbl_lookup(le32_to_cpu(d->pASeriesTbl[aseries_id]));
}

/**
 * Look up an amiibo's series identification.
 * @param amiibo_id	[in] amiibo ID. (Page 22) [must be host-endian]
 * @param pReleaseNo	[out,opt] Release number within series.
 * @param pWaveNo	[out,opt] Wave number within series.
 * @return amiibo series name, or nullptr if not found.
 */
const char *AmiiboData::lookup_amiibo_series_data(uint32_t amiibo_id, int *pReleaseNo, int *pWaveNo) const
{
	RP_D(AmiiboData);
	if (d->loadIfNeeded() != 0)
		return nullptr;

	const unsigned int id = (amiibo_id >> 16) & 0xFFFF;
	if (id >= d->amiiboIdTbl_count) {
		// ID is out of range.
		return nullptr;
	}

	const AmiiboIDTableEntry *const pAmiibo = &d->pAmiiboIDTbl[id];
	if (pReleaseNo) {
		*pReleaseNo = le16_to_cpu(pAmiibo->release_no);
	}
	if (pWaveNo) {
		*pWaveNo = pAmiibo->wave_no;
	}

	return d->strTbl_lookup(le32_to_cpu(pAmiibo->name));
}

}
