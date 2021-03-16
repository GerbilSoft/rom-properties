/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AmiiboData.cpp: Nintendo amiibo identification data.                    *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.libromdata.h"
#include "AmiiboData.hpp"

#include "byteswap_rp.h"
#include "../../amiibo-data/amiibo_bin_structs.h"
#include "librpfile/RpFile.hpp"
using LibRpFile::RpFile;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
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
		ao::uvector<uint8_t> amiibo_bin_data;
		time_t amiibo_bin_ts;

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

	public:
		/**
		 * Load amiibo.bin if it's needed.
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

		/**
		 * CharTableEntry bsearch() comparison function.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API CharTableEntry_compar(const void *a, const void *b);

		/**
		 * CharVariantTableEntry bsearch() comparison function.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API CharVariantTableEntry_compar(const void *a, const void *b);
};

AmiiboDataPrivate::AmiiboDataPrivate()
	: amiibo_bin_ts(-1)
	, pHeader(nullptr)
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
{
	// Loading will be delayed until it's needed.
}

/**
 * Load amiibo.bin if it's needed.
 * @return 0 on success or if load isn't needed; negative POSIX error code on error.
 */
int AmiiboDataPrivate::loadIfNeeded(void)
{
	// TODO: Check timestamps.
	if (!amiibo_bin_data.empty()) {
		// Already loaded.
		return 0;
	}

	// Load amiibo.bin.
	// TODO: Correct paths.
#ifdef DIR_INSTALL_SHARE
	static const char AMIIBO_BIN_FILENAME[] = DIR_INSTALL_SHARE "/amiibo-data.bin";
#else
	static const char AMIIBO_BIN_FILENAME[] = "amiibo-data.bin";
#endif
	RpFile *const pFile = new RpFile(AMIIBO_BIN_FILENAME, RpFile::FM_OPEN_READ);
	if (!pFile->isOpen()) {
		// Unable to open the file.
		int err = -pFile->lastError();
		pFile->unref();
		return err;
	}

	// Make sure the file is larger than sizeof(AmiiboBinHeader)
	// and it's under 1 MB.
	const int64_t filesize = pFile->size();
	if (filesize < (int64_t)sizeof(AmiiboBinHeader) || filesize >= 1024*1024) {
		// Over 1 MB.
		pFile->unref();
		return -ENOMEM;
	}

	// Clear all offsets before loading the data.
	clear();

	// Load the data.
	amiibo_bin_data.resize(filesize);
	size_t size = pFile->read(amiibo_bin_data.data(), filesize);
	pFile->unref();
	if (size != static_cast<size_t>(filesize)) {
		// Read error.
		int err = -pFile->lastError();
		if (err == 0) {
			err = -EIO;
		}
		amiibo_bin_data.clear();
		return err;
	}

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
	    ((uint64_t)strtbl_offset + (uint64_t)strtbl_len) > static_cast<uint64_t>(filesize))
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
	    ((uint64_t)cseries_offset + (uint64_t)cseries_len) > static_cast<uint64_t>(filesize))
	{
		// p.21 character series table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t char_offset = le32_to_cpu(pHeader_tmp->char_offset);
	const uint32_t char_len = le32_to_cpu(pHeader_tmp->char_len);
	if (char_offset < sizeof(AmiiboBinHeader) || char_len == 0 ||
	    char_len % sizeof(CharTableEntry) != 0 ||
	    ((uint64_t)char_offset + (uint64_t)char_len) > static_cast<uint64_t>(filesize))
	{
		// p.21 character table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t cvar_offset = le32_to_cpu(pHeader_tmp->cvar_offset);
	const uint32_t cvar_len = le32_to_cpu(pHeader_tmp->cvar_len);
	if (cvar_offset < sizeof(AmiiboBinHeader) || cvar_len == 0 ||
	    cvar_len % sizeof(CharVariantTableEntry) != 0 ||
	    ((uint64_t)cvar_offset + (uint64_t)cvar_len) > static_cast<uint64_t>(filesize))
	{
		// p.21 character variant table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t aseries_offset = le32_to_cpu(pHeader_tmp->aseries_offset);
	const uint32_t aseries_len = le32_to_cpu(pHeader_tmp->aseries_len);
	if (aseries_offset < sizeof(AmiiboBinHeader) || aseries_len == 0 ||
	    aseries_len % sizeof(uint32_t) != 0 ||
	    ((uint64_t)aseries_offset + (uint64_t)aseries_len) > static_cast<uint64_t>(filesize))
	{
		// p.22 amiibo series table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	const uint32_t amiibo_offset = le32_to_cpu(pHeader_tmp->amiibo_offset);
	const uint32_t amiibo_len = le32_to_cpu(pHeader_tmp->amiibo_len);
	if (amiibo_offset < sizeof(AmiiboBinHeader) || amiibo_len == 0 ||
	    amiibo_len % sizeof(AmiiboIDTableEntry) != 0 ||
	    ((uint64_t)amiibo_offset + (uint64_t)amiibo_len) > static_cast<uint64_t>(filesize))
	{
		// p.22 amiibo ID table offsets are invalid.
		amiibo_bin_data.clear();
		return -EIO;
	}

	// Save the table values and offsets.
	// TODO: Timestamp?
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
	amiibo_bin_ts = -1;
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
	if (strTbl_len <= 0 || idx >= strTbl_len)
		return nullptr;
	return &pStrTbl[idx];
}

/**
 * CharTableEntry bsearch() comparison function.
 * @param a
 * @param b
 * @return
 */
int RP_C_API AmiiboDataPrivate::CharTableEntry_compar(const void *a, const void *b)
{
	uint32_t id1 = le32_to_cpu(static_cast<const CharTableEntry*>(a)->char_id);
	uint32_t id2 = le32_to_cpu(static_cast<const CharTableEntry*>(b)->char_id);

	// NOTE: Masking off the character variant flag.
	id1 &= ~CHARTABLE_VARIANT_FLAG;
	id2 &= ~CHARTABLE_VARIANT_FLAG;

	if (id1 < id2) return -1;
	else if (id1 > id2) return 1;
	else return 0;
}

/**
 * CharVariantTableEntry bsearch() comparison function.
 * @param a
 * @param b
 * @return
 */
int RP_C_API AmiiboDataPrivate::CharVariantTableEntry_compar(const void *a, const void *b)
{
	const CharVariantTableEntry *pA = static_cast<const CharVariantTableEntry*>(a);
	const CharVariantTableEntry *pB = static_cast<const CharVariantTableEntry*>(b);

	if (pA->char_id == pB->char_id) {
		// Same character ID. Compare the variant ID.
		const uint8_t varA = pA->var_id;
		const uint8_t varB = pB->var_id;
		if (varA < varB) return -1;
		else if (varA > varB) return 1;
		else return 0;
	}

	// Character ID doesn't match.
	const uint16_t idA = le16_to_cpu(pA->char_id);
	const uint16_t idB = le16_to_cpu(pB->char_id);

	if (idA < idB) return -1;
	else if (idA > idB) return 1;
	else return 0;
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

	const uint16_t id = (char_id >> 16) & 0xFFFF;

	// Do a binary search.
	const CharTableEntry key = {id, 0};
	const CharTableEntry *const cres =
		static_cast<const CharTableEntry*>(bsearch(&key,
			d->pCharTbl,
			d->charTbl_count,
			sizeof(CharTableEntry),
			AmiiboDataPrivate::CharTableEntry_compar));
	if (!cres) {
		// Character ID not found.
		return nullptr;
	}

	// Check for variants.
	const char *name = nullptr;
	if (le32_to_cpu(cres->char_id) & CHARTABLE_VARIANT_FLAG) {
		// Do a binary search in the character variant table.
		const uint8_t variant_id = (char_id >> 8) & 0xFF;
		const CharVariantTableEntry key = {id, variant_id, 0, 0};
		const CharVariantTableEntry *const vres =
			static_cast<const CharVariantTableEntry*>(bsearch(&key,
				d->pCharVarTbl,
				d->charVarTbl_count,
				sizeof(CharVariantTableEntry),
				AmiiboDataPrivate::CharVariantTableEntry_compar));

		if (vres) {
			// Character variant ID found.
			name = d->strTbl_lookup(le32_to_cpu(vres->name));
		} else {
			// Character variant ID not found.
			// Maybe it's an error...
			// Default to the main character name.
			name = d->strTbl_lookup(le32_to_cpu(cres->name));
		}
	} else {
		// No variants.
		name = d->strTbl_lookup(le32_to_cpu(cres->name));
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
		*pReleaseNo = pAmiibo->release_no;
	}
	if (pWaveNo) {
		*pWaveNo = pAmiibo->wave_no;
	}

	return d->strTbl_lookup(le32_to_cpu(pAmiibo->name));
}

}
