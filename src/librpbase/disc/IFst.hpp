/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IFst.hpp: File System Table interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#include "common.h"

// C includes (C++ namespace)
#include <cstdint>

// Directory type values.
// Based on dirent.h from glibc-2.23.
#include "d_type.h"

// C++ includes.
#include <string>

namespace LibRpBase {

class NOVTABLE IFst
{
protected:
	IFst() = default;
public:
	virtual ~IFst() = 0;

private:
	RP_DISABLE_COPY(IFst)

public:
	// TODO: Base class?

	/**
	 * Is the FST open?
	 * @return True if open; false if not.
	 */
	virtual bool isOpen(void) const = 0;

	/**
	 * Have any errors been detected in the FST?
	 * @return True if yes; false if no.
	 */
	virtual bool hasErrors(void) const = 0;

public:
	/** opendir() interface **/

	struct DirEnt {
		off64_t offset;		// Starting address
		off64_t size;		// File size
		const char *name;	// Filename

		// TODO: Additional placeholders?
		unsigned int ptnum;	// Partition or content number
		int idx;		// File index
		uint8_t type;		// File type (See d_type.h)
	};

	struct Dir {
		IFst *const parent;	// IFst that owns this Dir
		int dir_idx;		// Directory index in the FST
		DirEnt entry;		// Current DirEnt

		explicit Dir(IFst *parent)
			: parent(parent)
		{}
	};

	/**
	 * Open a directory.
	 * @param path	[in] Directory path.
	 * @return Dir*, or nullptr on error.
	 */
	virtual Dir *opendir(const char *path) = 0;

	/**
	 * Open a directory.
	 * @param path	[in] Directory path.
	 * @return Dir*, or nullptr on error.
	 */
	inline Dir *opendir(const std::string &path)
	{
		return opendir(path.c_str());
	}

	/**
	 * Read a directory entry.
	 * @param dirp Dir pointer.
	 * @return DirEnt*, or nullptr if end of directory or on error.
	 * (TODO: Add lastError()?)
	 */
	virtual const DirEnt *readdir(Dir *dirp) = 0;

	/**
	 * Close an opened directory.
	 * @param dirp Dir pointer.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int closedir(Dir *dirp) = 0;

	/**
	 * Get the directory entry for the specified file.
	 * @param filename	[in] Filename.
	 * @param dirent	[out] Pointer to DirEnt buffer.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int find_file(const char *filename, DirEnt *dirent) = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IFst::~IFst() = default;

}
