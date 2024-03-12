/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUFst.hpp: Wii U FST parser                                           *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/disc/IFst.hpp"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

namespace LibRomData {

class WiiUFstPrivate;
class WiiUFst final : public LibRpBase::IFst
{
public:
	/**
	 * Parse a Wii U FST.
	 * @param fstData FST data
	 * @param len Length of fstData, in bytes
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	RP_LIBROMDATA_PUBLIC
	WiiUFst(const uint8_t *fstData, uint32_t len);

	~WiiUFst() final;

private:
	typedef IFst super;
	RP_DISABLE_COPY(WiiUFst)

private:
	friend class WiiUFstPrivate;
	WiiUFstPrivate *const d;

public:
	/**
	 * Is the FST open?
	 * @return True if open; false if not.
	 */
	RP_LIBROMDATA_PUBLIC
	bool isOpen(void) const final;

	/**
	 * Have any errors been detected in the FST?
	 * @return True if yes; false if no.
	 */
	RP_LIBROMDATA_PUBLIC
	bool hasErrors(void) const final;

public:
	/** opendir() interface **/

	/**
	 * Open a directory.
	 * @param path	[in] Directory path.
	 * @return Dir*, or nullptr on error.
	 */
	Dir *opendir(const char *path) final;

	/**
	 * Read a directory entry.
	 * @param dirp Dir pointer.
	 * @return DirEnt*, or nullptr if end of directory or on error.
	 * (TODO: Add lastError()?)
	 */
	DirEnt *readdir(Dir *dirp) final;

	/**
	 * Close an opened directory.
	 * @param dirp Dir pointer.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int closedir(Dir *dirp) final;

	/**
	 * Get the directory entry for the specified file.
	 * @param filename	[in] Filename.
	 * @param dirent	[out] Pointer to DirEnt buffer.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int find_file(const char *filename, DirEnt *dirent) final;

public:
	/**
	 * Get the total size of all files.
	 *
	 * This is a shortcut function that reads the FST
	 * directly instead of using opendir().
	 *
	 * @return Size of all files, in bytes. (-1 on error)
	 */
	off64_t totalUsedSize(void) const;
};

}
