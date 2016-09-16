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

#include "TextFuncs.hpp"
using LibRomData::rp_string;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace LibRomData {

class GcnFstPrivate
{
	public:
		GcnFstPrivate(const uint8_t *fstData, uint32_t len, uint8_t offsetShift);
		~GcnFstPrivate();

	private:
		GcnFstPrivate(const GcnFstPrivate &other);
		GcnFstPrivate &operator=(const GcnFstPrivate &other);

	public:
		// FST data.
		GCN_FST_Entry *fstData;
		uint32_t fstData_sz;

		// String table. (Pointer into d->fstData.)
		// String table. (malloc'd)
		const char *string_table;
		uint32_t string_table_sz;

		// Offset shift.
		uint8_t offsetShift;

		// FstDir* reference counter.
		int fstDirCount;

		/**
		 * Get an FST entry.
		 *
		 * NOTE: FST entries have NOT been byteswapped.
		 * Use be32_to_cpu() when reading.
		 *
		 * @param idx		[in] FST entry index.
		 * @param ppszName	[out, opt] Entry name. (Do not free this!)
		 * @return FST entry, or nullptr on error.
		 */
		const GCN_FST_Entry *entry(int idx, const char **ppszName = nullptr) const;
};

/** GcnFstPrivate **/

GcnFstPrivate::GcnFstPrivate(const uint8_t *fstData, uint32_t len, uint8_t offsetShift)
	: fstData(nullptr)
	, fstData_sz(0)
	, string_table(nullptr)
	, string_table_sz(0)
	, offsetShift(offsetShift)
	, fstDirCount(0)
{
	static_assert(sizeof(GCN_FST_Entry) == GCN_FST_Entry_SIZE,
		"sizeof(GCN_FST_Entry) is incorrect. (Should be 12)");

	if (len < sizeof(GCN_FST_Entry)) {
		// Invalid FST length.
		return;
	}

	// String table is stored directly after the root entry.
	const GCN_FST_Entry *root_entry = reinterpret_cast<const GCN_FST_Entry*>(fstData);
	uint32_t string_table_offset = be32_to_cpu(root_entry->dir.last_entry_idx) * sizeof(GCN_FST_Entry);
	if (string_table_offset >= len) {
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
	this->fstData = reinterpret_cast<GCN_FST_Entry*>(fst8);
	memcpy(this->fstData, fstData, len);

	// Save a pointer to the string table.
	string_table = reinterpret_cast<char*>(&fst8[string_table_offset]);
	string_table_sz = len - string_table_offset;
}

GcnFstPrivate::~GcnFstPrivate()
{
	assert(fstDirCount == 0);
	free(fstData);
}

/**
 * Get an FST entry.
 *
 * NOTE: FST entries have NOT been byteswapped.
 * Use be32_to_cpu() when reading.
 *
 * @param idx		[in] FST entry index.
 * @param ppszName	[out, opt] Entry name. (Do not free this!)
 * @return FST entry, or nullptr on error.
 */
const GCN_FST_Entry *GcnFstPrivate::entry(int idx, const char **ppszName) const
{
	if (!fstData || idx < 0) {
		// No FST, or idx is invalid.
		return nullptr;
	}

	// NOTE: For the root directory, last_entry_idx is number of entries.
	if ((uint32_t)idx >= be32_to_cpu(fstData[0].root_dir.file_count)) {
		// Index is out of range.
		return nullptr;
	}

	if (ppszName) {
		// Get the name entry from the string table.
		uint32_t offset = be32_to_cpu(fstData[idx].file_type_name_offset) & 0xFFFFFF;
		if (offset < string_table_sz) {
			*ppszName = &string_table[offset];
		} else {
			// Offset is out of range.
			*ppszName = nullptr;
		}
	}

	return &fstData[idx];
}

/** GcnFst **/

/**
 * Parse a GameCube FST.
 * @param fstData FST data.
 * @param len Length of fstData, in bytes.
 * @param offsetShift File offset shift. (0 = GCN, 2 = Wii)
 */
GcnFst::GcnFst(const uint8_t *fstData, uint32_t len, uint8_t offsetShift)
	: d(new GcnFstPrivate(fstData, len, offsetShift))
{ }

GcnFst::~GcnFst()
{
	delete d;
}

/**
 * Get the number of FST entries.
 * @return Number of FST entries, or -1 on error.
 */
int GcnFst::count(void) const
{
	if (!d->fstData) {
		// No FST.
		return -1;
	}

	return (be32_to_cpu(d->fstData[0].dir.last_entry_idx) + 1);
}

/**
 * Open a directory.
 * @param path	[in] Directory path.
 * @return FstDir*, or nullptr on error.
 */
GcnFst::FstDir *GcnFst::opendir(const rp_char *path)
{
	// TODO: Ignoring path right now.
	// Always reading the root directory.
	if (!d->fstData) {
		// No FST.
		return nullptr;
	}

	const char *pName;
	const GCN_FST_Entry *fst_entry = d->entry(0, &pName);
	if (!fst_entry) {
		// Error retrieving the root directory.
		return nullptr;
	}

	FstDir *dirp = reinterpret_cast<FstDir*>(malloc(sizeof(*dirp)));
	if (!dirp) {
		// malloc() failed.
		return nullptr;
	}

	d->fstDirCount++;
	dirp->dir_idx = 0;

	// Initialize the entry to the root directory.
	// readdir() will automatically seek to the next entry.
	dirp->entry.idx = dirp->dir_idx;
	dirp->entry.type = DT_DIR;
	dirp->entry.name = pName;
	// offset and size are not valid for directories.
	dirp->entry.offset = 0;
	dirp->entry.size = 0;

	// Return the FstDir*.
	return dirp;
}

// Extract the d_type from a file_name_type_offset.
// TODO: Move somewhere else?
#define FTNO_D_TYPE(ftno) ((be32_to_cpu(ftno) >> 24) == 1 ? DT_DIR : DT_REG)

/**
 * Read a directory entry.
 * @param dirp FstDir pointer.
 * @return FstDirEntry*, or nullptr if end of directory or on error.
 * (End of directory does not set lastError; an error does.)
 */
GcnFst::FstDirEntry *GcnFst::readdir(FstDir *dirp)
{
	if (!dirp) {
		// No directory pointer.
		return nullptr;
	}

	// Seek to the next entry in the directory.
	int idx = dirp->entry.idx;
	const GCN_FST_Entry *fst_entry = d->entry(idx, nullptr);
	if (!fst_entry) {
		// No more entries.
		return nullptr;
	}

	if (idx != dirp->dir_idx && FTNO_D_TYPE(fst_entry->file_type_name_offset) == DT_DIR) {
		// Skip over this directory.
		idx = be32_to_cpu(fst_entry->dir.last_entry_idx) + 1;
	} else {
		// Go to the next file.
		idx++;
	}

	const char *pName;
	fst_entry = d->entry(idx, &pName);
	dirp->entry.idx = idx;
	if (!fst_entry) {
		// No more entries.
		dirp->entry.type = 0;
		dirp->entry.name = nullptr;
		return nullptr;
	}

	// Save the entry information.
	dirp->entry.type = FTNO_D_TYPE(fst_entry->file_type_name_offset);
	dirp->entry.name = pName;
	if (dirp->entry.type == DT_DIR) {
		// offset and size are not valid for directories.
		dirp->entry.offset = 0;
		dirp->entry.size = 0;
	} else {
		// Save the offset and size.
		dirp->entry.offset = ((int64_t)be32_to_cpu(fst_entry->file.offset) << d->offsetShift);
		dirp->entry.size = be32_to_cpu(fst_entry->file.size);
	}

	return &dirp->entry;
}

/**
 * Close an opened directory.
 * @param dirp FstDir pointer.
 * @return 0 on success; negative POSIX error code on error.
 */
int GcnFst::closedir(FstDir *dirp)
{
	assert(d->fstDirCount > 0);
	if (!dirp) {
		// Invalid pointer.
		return -EINVAL;
	}

	free(dirp);
	d->fstDirCount--;
	return 0;
}

}
