/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnFst.cpp: GameCube/Wii FST parser.                                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "GcnFst.hpp"
#include "../Console/gcn_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::unordered_map;

namespace LibRomData {

class GcnFstPrivate
{
public:
	GcnFstPrivate(const uint8_t *fstData, uint32_t len, uint8_t offsetShift);
	~GcnFstPrivate();

private:
	RP_DISABLE_COPY(GcnFstPrivate)

public:
	bool hasErrors;
	uint8_t offsetShift;	// offset shift (0 for GCN, 2 for Wii)

	// IFst::Dir* reference counter
	int fstDirCount;

	GCN_FST_Entry *fstData;
	const char *string_table_ptr;	// pointer into fstData

	uint32_t fstData_sz;
	uint32_t string_table_sz;

	// String table, converted to Unicode.
	// - Key: String offset in the FST string table
	// - Value: UTF-8 string
	mutable unordered_map<uint32_t, string> u8_string_table;

	/**
	 * Check if an fst_entry is a directory.
	 * @return True if this is a directory; false if it's a regular file.
	 */
	static inline bool is_dir(const GCN_FST_Entry *fst_entry)
	{
		return ((be32_to_cpu(fst_entry->file_type_name_offset) >> 24) == 1);
	}

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
	 * @param path Path. (Absolute paths only!)
	 * @return fst_entry if found, or nullptr if not.
	 */
	const GCN_FST_Entry *find_path(const char *path) const;
};

/** GcnFstPrivate **/

GcnFstPrivate::GcnFstPrivate(const uint8_t *fstData, uint32_t len, uint8_t offsetShift)
	: hasErrors(false)
	, offsetShift(offsetShift)
	, fstDirCount(0)
	, fstData(nullptr)
	, string_table_ptr(nullptr)
	, fstData_sz(len)
	, string_table_sz(0)
{
	assert(fstData != nullptr);
	assert(len >= sizeof(GCN_FST_Entry));
	if (!fstData || len < sizeof(GCN_FST_Entry)) {
		// Invalid parameters.
		hasErrors = true;
		return;
	}

	// String table is stored after the file table.
	// Use the root entry to determine how many files are present.
	const GCN_FST_Entry *root_entry = reinterpret_cast<const GCN_FST_Entry*>(fstData);
	const uint32_t file_count = be32_to_cpu(root_entry->root_dir.file_count);
	if (file_count <= 1 || file_count > (fstData_sz / sizeof(GCN_FST_Entry))) {
		// Sanity check: File count is invalid.
		// - 1 file means it only has a root directory.
		// - 0 files isn't possible.
		// - Can't have more than fstData_sz / sizeof(GCN_FST_Entry) files.
		hasErrors = true;
		return;
	}

	const uint32_t string_table_offset = file_count * sizeof(GCN_FST_Entry);
	if (string_table_offset >= len) {
		// Invalid FST length.
		hasErrors = true;
		return;
	}

	// Sanity check: String table cannot contain '/'.
	string_table_sz = fstData_sz - string_table_offset;
	if (memchr(&fstData[string_table_offset], '/', string_table_sz) != nullptr) {
		// String table has '/'!
		hasErrors = true;
		return;
	}

	// Copy the FST data.
	// NOTE: +1 for NULL termination.
	uint8_t *const fst8 = new uint8_t[fstData_sz + 1];
	memcpy(fst8, fstData, fstData_sz);
	fst8[fstData_sz] = 0; // Make sure the string table is NULL-terminated.
	this->fstData = reinterpret_cast<GCN_FST_Entry*>(fst8);

	// Save a pointer to the string table.
	string_table_ptr = reinterpret_cast<char*>(&fst8[string_table_offset]);

#ifdef HAVE_UNORDERED_MAP_RESERVE
	// Reserve space in the string table.
	// NOTE: file_count includes the root directory entry.
	u8_string_table.reserve(file_count - 1);
#endif
}

GcnFstPrivate::~GcnFstPrivate()
{
	assert(fstDirCount == 0);
	delete[] fstData;
}

/**
 * Get an FST entry's name.
 * @param fst_entry FST entry.
 * @return Name, or nullptr if an error occurred.
 */
inline const char *GcnFstPrivate::entry_name(const GCN_FST_Entry *fst_entry) const
{
	// Get the name entry from the string table.
	const uint32_t offset = be32_to_cpu(fst_entry->file_type_name_offset) & 0xFFFFFF;
	if (offset >= string_table_sz) {
		// Out of range.
		return nullptr;
	}

	// Has this name already been converted to UTF-8?
	unordered_map<uint32_t, string>::const_iterator iter = u8_string_table.find(offset);
	if (iter != u8_string_table.end()) {
		// Name has already been converted.
		return iter->second.c_str();
	}

	// Name has not been converted.
	// Do the conversion now.
	const char *str = &string_table_ptr[offset];
	int len = static_cast<int>(strlen(str));	// TODO: Bounds checking.
	const string u8str = cp1252_sjis_to_utf8(str, len);
	iter = u8_string_table.emplace(offset, u8str).first;
	return iter->second.c_str();
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

	// NOTE: For the root directory, next_offset is the number of entries.
	if (static_cast<uint32_t>(idx) >= be32_to_cpu(fstData[0].root_dir.file_count)) {
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
 * @param path Path. (Absolute paths only!)
 * @return fst_entry if found, or nullptr if not.
 */
const GCN_FST_Entry *GcnFstPrivate::find_path(const char *path) const
{
	if (!path) {
		// Invalid path.
		return nullptr;
	}

	// Get the root directory.
	const GCN_FST_Entry *fst_entry = this->entry(0, nullptr);
	if (!fst_entry) {
		// Can't find the root directory.
		return nullptr;
	}

	if (!path[0] || (path[0] == '/' && !path[1])) {
		// Empty path or "/".
		// Return the root directory.
		return fst_entry;
	}

	// Store the path as a temporary string.
	string s_path;
	if (path[0] != '/') {
		// Prepend a slash.
		// (Relative paths aren't supported.)
		s_path = '/';
	}
	s_path.append(path);

	// Remove trailing slashes.
	while (s_path.size() > 1 && s_path[s_path.size()-1] == '/') {
		s_path.resize(s_path.size()-1);
	}
	if (s_path.empty() || s_path == "/") {
		// After removing all trailing slashes, we have the root directory.
		return fst_entry;
	}

	// Set of directory indexes that have already been processed.
	// Used to prevent infinite loops if the FST has weird corruption.
	std::unordered_set<int> idx_already;

	// Skip the initial slash.
	int idx = 1;	// Ignore the root directory.
	// NOTE: This is the index *after* the last file.
	int last_fst_idx = be32_to_cpu(fst_entry->root_dir.file_count);
	size_t slash_pos = 0;
	do {
		const size_t next_slash_pos = s_path.find('/', slash_pos + 1);
		string path_component;
		bool are_more_slashes = true;
		if (next_slash_pos == string::npos) {
			// No more slashes.
			are_more_slashes = false;
			path_component = s_path.substr(slash_pos + 1);
		} else {
			// Found another slash.
			int sz = static_cast<int>(next_slash_pos - slash_pos - 1);
			if (sz <= 0) {
				// Empty path component.
				slash_pos = next_slash_pos;
				continue;
			}
			path_component = s_path.substr(slash_pos + 1, static_cast<size_t>(sz));
		}

		if (path_component.empty()) {
			// Empty path component.
			slash_pos = next_slash_pos;
			continue;
		}

		// Search this directory for a matching path component.
		idx_already.clear();
		bool found = false;
		for (; idx < last_fst_idx; idx++) {
			auto iter = idx_already.find(idx);
			if (iter != idx_already.end()) {
				// Something is wrong! We've already iterated over this directory.
				return nullptr;
			}
			idx_already.insert(idx);

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
			if (is_dir(fst_entry)) {
				// NOTE: next_offset is the index *after* the
				// last entry in the subdirectory, so we have to
				// subtract 1 for proper enumeration.
				idx = be32_to_cpu(fst_entry->dir.next_offset) - 1;
			}
		}

		if (!found) {
			// No match.
			return nullptr;
		}

		if (is_dir(fst_entry)) {
			// Directory. Save the last_fst_idx.
			last_fst_idx = be32_to_cpu(fst_entry->dir.next_offset);
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
	: d_ptr(new GcnFstPrivate(fstData, len, offsetShift))
{ }

GcnFst::~GcnFst()
{
	delete d_ptr;
}

/**
 * Is the FST open?
 * @return True if open; false if not.
 */
bool GcnFst::isOpen(void) const
{
	RP_D(const GcnFst);
	return (d->string_table_ptr != nullptr);
}

/**
 * Have any errors been detected in the FST?
 * @return True if yes; false if no.
 */
bool GcnFst::hasErrors(void) const
{
	RP_D(const GcnFst);
	return d->hasErrors;
}

/** opendir() interface **/

/**
 * Open a directory.
 * @param path	[in] Directory path.
 * @return IFst::Dir*, or nullptr on error.
 */
IFst::Dir *GcnFst::opendir(const char *path)
{
	RP_D(GcnFst);
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

	if (!d->is_dir(fst_entry)) {
		// Not a directory.
		// TODO: Set ENOTDIR?
		return nullptr;
	}

	IFst::Dir *dirp = new IFst::Dir(this);
	d->fstDirCount++;
	// TODO: Better way to get dir_idx?
	dirp->dir_idx = static_cast<int>(fst_entry - d->fstData);

	// Initialize the entry to this directory.
	// readdir() will automatically seek to the next entry.
	dirp->entry.ptnum = 0;	// not used for GCN/Wii
	dirp->entry.idx = dirp->dir_idx;
	dirp->entry.type = DT_DIR;
	dirp->entry.name = d->entry_name(fst_entry);
	// offset and size are not valid for directories.
	dirp->entry.offset = 0;
	dirp->entry.size = 0;

	// Return the IFst::Dir*.
	return dirp;
}

/**
 * Read a directory entry.
 * @param dirp IFst::Dir pointer.
 * @return IFst::DirEnt*, or nullptr if end of directory or on error.
 * (End of directory does not set lastError; an error does.)
 */
IFst::DirEnt *GcnFst::readdir(IFst::Dir *dirp)
{
	assert(dirp != nullptr);
	assert(dirp->parent == this);
	if (!dirp || dirp->parent != this) {
		// No directory pointer, or the dirp
		// doesn't belong to this IFst.
		return nullptr;
	}

	// Get the directory's fst_entry.
	RP_D(GcnFst);
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

	if (idx != dirp->dir_idx && d->is_dir(fst_entry)) {
		// Skip over this directory.
		const int next_idx = be32_to_cpu(fst_entry->dir.next_offset);
		if (next_idx <= idx) {
			// Seeking backwards? (or looping to the same entry)
			d->hasErrors = true;
			return nullptr;
		}
		idx = next_idx;
	} else {
		// Go to the next file.
		idx++;
	}

	// NOTE: next_offset is the entry index *after* the last entry,
	// so this works for both the root directory and subdirectories.
	if (idx >= static_cast<int>(be32_to_cpu(dir_fst_entry->dir.next_offset))) {
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
	} else if (!pName || pName[0] == 0) {
		// Empty or NULL name. This is invalid.
		// Stop processing the directory.
		d->hasErrors = true;
		dirp->entry.type = 0;
		dirp->entry.name = nullptr;
		return nullptr;
	}

	// Save the entry information.
	const bool is_fst_dir = d->is_dir(fst_entry);
	dirp->entry.ptnum = 0;	// not used for GCN/Wii
	dirp->entry.type = is_fst_dir ? DT_DIR : DT_REG;
	dirp->entry.name = pName;
	if (is_fst_dir) {
		// offset and size are not valid for directories.
		dirp->entry.offset = 0;
		dirp->entry.size = 0;
	} else {
		// Save the offset and size.
		dirp->entry.offset = static_cast<off64_t>(be32_to_cpu(fst_entry->file.offset)) << d->offsetShift;
		dirp->entry.size = be32_to_cpu(fst_entry->file.size);
	}

	return &dirp->entry;
}

/**
 * Close an opened directory.
 * @param dirp IFst::Dir pointer.
 * @return 0 on success; negative POSIX error code on error.
 */
int GcnFst::closedir(IFst::Dir *dirp)
{
	assert(dirp != nullptr);
	assert(dirp->parent == this);
	if (!dirp) {
		// No directory pointer.
		// In release builds, this is a no-op.
		return 0;
	} else if (dirp->parent != this) {
		// The dirp doesn't belong to this IFst.
		return -EINVAL;
	}

	RP_D(GcnFst);
	assert(d->fstDirCount > 0);
	delete dirp;
	d->fstDirCount--;
	return 0;
}


/**
 * Get the directory entry for the specified file.
 * @param filename	[in] Filename.
 * @param dirent	[out] Pointer to DirEnt buffer.
 * @return 0 on success; negative POSIX error code on error.
 */
int GcnFst::find_file(const char *filename, DirEnt *dirent)
{
	if (!filename || !dirent) {
		// Invalid parameters.
		return -EINVAL;
	}

	RP_D(GcnFst);
	const GCN_FST_Entry *fst_entry = d->find_path(filename);
	if (!fst_entry) {
		// Not found.
		return -ENOENT;
	}

	// Copy the relevant information to dirent.
	const bool is_fst_dir = d->is_dir(fst_entry);
	dirent->ptnum = 0;	// not used for GCN/Wii
	dirent->type = is_fst_dir ? DT_DIR : DT_REG;
	dirent->name = d->entry_name(fst_entry);
	if (is_fst_dir) {
		// offset and size are not valid for directories.
		dirent->offset = 0;
		dirent->size = 0;
	} else {
		// Save the offset and size.
		dirent->offset = static_cast<off64_t>(be32_to_cpu(fst_entry->file.offset)) << d->offsetShift;
		dirent->size = be32_to_cpu(fst_entry->file.size);
	}

	return 0;
}

/**
 * Get the total size of all files.
 *
 * This is a shortcut function that reads the FST
 * directly instead of using opendir().
 *
 * @return Size of all files, in bytes. (-1 on error)
 */
off64_t GcnFst::totalUsedSize(void) const
{
	RP_D(const GcnFst);
	if (!d->fstData) {
		// No FST...
		return -1;
	}

	off64_t total_size = 0;
	const GCN_FST_Entry *entry = d->fstData;
	uint32_t file_count = be32_to_cpu(entry->root_dir.file_count);
	entry++;

	// NOTE: file_count includes the root directory entry,
	// which should be skipped.
	for (; file_count > 1; file_count--, entry++) {
		if (d->is_dir(entry))
			continue;
		total_size += static_cast<off64_t>(be32_to_cpu(entry->file.size));
	}
	return total_size;
}

}
