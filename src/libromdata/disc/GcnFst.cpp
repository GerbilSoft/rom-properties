/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnFst.cpp: GameCube/Wii FST parser.                                    *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "GcnFst.hpp"
#include "byteswap.h"

// C includes. (C++ namespace)
#include <cstdlib>
#include <cstring>

namespace LibRomData {

/**
 * Parse a GameCube FST.
 * @param fstData FST data.
 * @param len Length of fstData, in bytes.
 * @param offsetShift File offset shift. (0 = GCN, 2 = Wii)
 */
GcnFst::GcnFst(const uint8_t *fstData, uint32_t len, uint8_t offsetShift)
	: m_fstData(nullptr)
	, m_fstData_sz(0)
	, m_string_table(nullptr)
	, m_string_table_sz(0)
	, m_offsetShift(offsetShift)
{
	static_assert(sizeof(GCN_FST_Entry) == GCN_FST_Entry_SIZE,
		"sizeof(GCN_FST_Entry) is incorrect. (Should be 12)");

	if (len < sizeof(GCN_FST_Entry)) {
		// Invalid FST length.
		return;
	}

	// String table is stored directly after the root entry.
	const GCN_FST_Entry *root_entry = reinterpret_cast<const GCN_FST_Entry*>(fstData);
	uint32_t string_table_offset = root_entry->dir.last_entry_idx * sizeof(GCN_FST_Entry);
	if (string_table_offset <= len) {
		// Invalid FST length.
		return;
	}

	// Copy the FST data.
	uint8_t *fst8 = reinterpret_cast<uint8_t*>(malloc(len+1));
	if (!fst8) {
		// Could not allocate memory for the FST.
		return;
	}
	fst8[len] = 0; // Make sure the string table is NULL-terminated.
	m_fstData = reinterpret_cast<GCN_FST_Entry*>(fst8);
	memcpy(m_fstData, fstData, len);

	// Save a pointer to the string table.
	m_string_table = reinterpret_cast<char*>(&fst8[string_table_offset]);
	m_string_table_sz = len - string_table_offset;
}

GcnFst::~GcnFst()
{
	free(m_fstData);
}

/**
 * Get the number of FST entries.
 * @return Number of FST entries, or -1 on error.
 */
int GcnFst::count(void) const
{
	if (!m_fstData) {
		// No FST.
		return -1;
	}

	return (be32_to_cpu(m_fstData[0].dir.last_entry_idx) + 1);
}

/**
 * Get an FST entry.
 * @param idx		[in] FST entry index.
 * @param ppszName	[out, opt] Entry name. (Do not free this!)
 * @return FST entry, or nullptr on error.
 */
const GCN_FST_Entry *GcnFst::entry(int idx, const char **ppszName) const
{
	// TODO: opendir()/readdir()-like interface instead of
	// accessing the raw FST data.

	if (!m_fstData || idx < 0) {
		// No FST, or idx is invalid.
		return nullptr;
	}

	if ((uint32_t)idx > be32_to_cpu(m_fstData[0].dir.last_entry_idx)) {
		// Index is out of range.
		return nullptr;
	}

	if (ppszName) {
		// Get the name entry from the string table.
		uint32_t offset = be32_to_cpu(m_fstData[idx].file_type_name_offset) & 0xFFFFFF;
		if (offset < m_string_table_sz) {
			*ppszName = &m_string_table[offset];
		} else {
			// Offset is out of range.
			*ppszName = nullptr;
		}
	}

	return &m_fstData[idx];
}

}
