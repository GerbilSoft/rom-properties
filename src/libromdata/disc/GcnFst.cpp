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

// C++ includes.
#include <string>
using std::string;

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
		 * Get an FST entry's name.
		 * @param fst_entry FST entry.
		 * @return Name, or nullptr if an error occurred.
		 */
		inline const char *entry_name(const GCN_FST_Entry *fst_entry) const;

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

		/**
		 * Find a path.
		 * @param path Path.
		 * @return fst_entry if found, or nullptr if not.
		 */
		const GCN_FST_Entry *find_path(const rp_char *path) const;
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

// Extract the d_type from a file_name_type_offset.
// TODO: Move somewhere else?
#define FTNO_D_TYPE(ftno) ((be32_to_cpu(ftno) >> 24) == 1 ? DT_DIR : DT_REG)

/**
 * Get an FST entry's name.
 * @param fst_entry FST entry.
 * @return Name, or nullptr if an error occurred.
 */
inline const char *GcnFstPrivate::entry_name(const GCN_FST_Entry *fst_entry) const
{
	// Get the name entry from the string table.
	uint32_t offset = be32_to_cpu(fst_entry->file_type_name_offset) & 0xFFFFFF;
	if (offset < string_table_sz) {
		return &string_table[offset];
	}

	// Offset is out of range.
	return nullptr;
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
		if (idx == 0) {
			// Root directory has no name.
			*ppszName = nullptr;
		} else {
			// Get the entry's name.
			*ppszName = entry_name(&fstData[idx]);
		}
	}

	return &fstData[idx];
}

/**
 * Find a path.
 * @param path Path.
 * @return fst_entry if found, or nullptr if not.
 */
const GCN_FST_Entry *GcnFstPrivate::find_path(const rp_char *path) const
{
	if (!path) {
		// Invalid path.
		return nullptr;
	} else if (!path[0] || (path[0] == '/' && !path[1])) {
		// Empty path or "/".
		// Return the root directory.
		return this->entry(0, nullptr);
	}

	// Convert the path to UTF-8.
	// TODO: ASCII or Latin-1 instead?
	string path8 = rp_string_to_utf8(path, rp_strlen(path));
	if (path8.empty()) {
		// Invalid path.
		return nullptr;
	}

	// If there's a trailing slash, remove it.
	if (path8[path8.size()-1] == '/') {
		path8.resize(path8.size()-1);
	}

	// Get the root directory.
	const GCN_FST_Entry *fst_entry = this->entry(0, nullptr);
	if (!fst_entry) {
		// Can't find the root directory.
		return nullptr;
	}

	// Should not have "" or "/" here.
	assert(!path8.empty());
	assert(path8 != "/");

	// Skip the initial slash.
	int idx = 1;	// Ignore the root directory.
	int last_fst_idx = be32_to_cpu(fst_entry->root_dir.file_count) - 1;
	size_t slash_pos = 0;
	do {
		size_t next_slash_pos = path8.find('/', slash_pos + 1);
		string path_component;
		bool are_more_slashes = true;
		if (next_slash_pos == string::npos) {
			// No more slashes.
			are_more_slashes = false;
			path_component = path8.substr(slash_pos + 1);
		} else {
			// Found another slash.
			int sz = (int)(next_slash_pos - slash_pos - 1);
			if (sz <= 0) {
				// Empty path component.
				slash_pos = next_slash_pos;
				continue;
			}
			path_component = path8.substr(slash_pos + 1, (size_t)sz);
		}

		if (path_component.empty()) {
			// Empty path component.
			slash_pos = next_slash_pos;
			continue;
		}

		// Search this directory for a matching path component.
		bool found = false;
		for (; idx <= last_fst_idx; idx++) {
			const char *pName;
			fst_entry = this->entry(idx, &pName);
			if (!fst_entry) {
				// Invalid path.
				return nullptr;
			}

			// TODO: Is GCN/Wii case-sensitive?
			if (pName && !strcmp(path_component.c_str(), pName)) {
				// Found a match.
				found = true;
				break;
			}

			// If this is a directory, skip it.
			if (FTNO_D_TYPE(fst_entry->file_type_name_offset) == DT_DIR) {
				idx = be32_to_cpu(fst_entry->dir.last_entry_idx);
			}
		}

		if (!found) {
			// No match.
			return nullptr;
		}

		if (FTNO_D_TYPE(fst_entry->file_type_name_offset) == DT_DIR) {
			// Directory. Save the last_fst_idx.
			last_fst_idx = be32_to_cpu(fst_entry->dir.last_entry_idx);
			idx++;
		} else {
			// File: Make sure there's no more slashes.
			if (are_more_slashes) {
				// More slashes. Not a match.
				return nullptr;
			}
		}

		// Next slash.
		slash_pos = next_slash_pos;
	} while (slash_pos != string::npos);

	// Found the directory entry.
	return fst_entry;
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

	// Find the path.
	const GCN_FST_Entry *fst_entry = d->find_path(path);
	if (!fst_entry) {
		// Error finding the path.
		return nullptr;
	}

	if (FTNO_D_TYPE(fst_entry->file_type_name_offset) != DT_DIR) {
		// Not a directory.
		// TODO: Set ENOTDIR?
		return nullptr;
	}

	FstDir *dirp = reinterpret_cast<FstDir*>(malloc(sizeof(*dirp)));
	if (!dirp) {
		// malloc() failed.
		return nullptr;
	}

	d->fstDirCount++;
	// TODO: Better way to get dir_idx?
	dirp->dir_idx = (int)(fst_entry - d->fstData);

	// Initialize the entry to the root directory.
	// readdir() will automatically seek to the next entry.
	dirp->entry.idx = dirp->dir_idx;
	dirp->entry.type = DT_DIR;
	dirp->entry.name = d->entry_name(fst_entry);
	// offset and size are not valid for directories.
	dirp->entry.offset = 0;
	dirp->entry.size = 0;

	// Return the FstDir*.
	return dirp;
}

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

	// Get the directory's fst_entry.
	const GCN_FST_Entry *dir_fst_entry = d->entry(dirp->dir_idx);
	if (!dir_fst_entry) {
		// dir_idx is corrupted.
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

	if (idx >= (int)be32_to_cpu(dir_fst_entry->dir.last_entry_idx)) {
		// Last entry in the directory.
		return nullptr;
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
	// Allow nullptr to be closed in release builds.
	// (Same behavior as free().)
	// In debug builds, this will assert().
	assert(dirp != nullptr);
	if (!dirp) {
		return 0;
	}

	assert(d->fstDirCount > 0);
	free(dirp);
	d->fstDirCount--;
	return 0;
}

}
