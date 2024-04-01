/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IPartition.hpp: Partition reader interface.                             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpfile/IRpFile.hpp"
#include "IDiscReader.hpp"
#include "IFst.hpp"

namespace LibRpBase {

class NOVTABLE IPartition : public IDiscReader
{
protected:
	explicit IPartition(const LibRpFile::IRpFilePtr &file) : super(file) { }
public:
	~IPartition() override = 0;

private:
	typedef IDiscReader super;
	RP_DISABLE_COPY(IPartition)

public:
	/** IDiscReader **/

	/**
	 * isDiscSupported() is not handled by IPartition.
	 * TODO: Maybe implement it to determine if the partition type is supported?
	 * TODO: Move to IPartition.cpp?
	 * @return -1
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const final
	{
		RP_UNUSED(pHeader);
		RP_UNUSED(szHeader);
		return -1;
	}

public:
	/**
	 * Get the partition size.
	 * This includes the partition headers and any
	 * metadata, e.g. Wii sector hashes, if present.
	 * @return Partition size, or -1 on error.
	 */
	virtual off64_t partition_size(void) const = 0;

	/**
	 * Get the used partition size.
	 * This includes the partition headers and any
	 * metadata, e.g. Wii sector hashes, if present.
	 * It does *not* include "empty" sectors.
	 * @return Used partition size, or -1 on error.
	 */
	virtual off64_t partition_size_used(void) const = 0;

public:
	/** IFst wrapper functions **/
	// NOTE: May return nullptr if not implemented for a given class.

	/**
	 * Open a directory.
	 * @param path	[in] Directory path.
	 * @return IFst::Dir*, or nullptr on error.
	 */
	virtual IFst::Dir *opendir(const char *path);

	/**
	 * Open a directory.
	 * @param path	[in] Directory path.
	 * @return IFst::Dir*, or nullptr on error.
	 */
	inline IFst::Dir *opendir(const std::string &path)
	{
		return this->opendir(path.c_str());
	}

	/**
	 * Read a directory entry.
	 * @param dirp IFst::Dir pointer.
	 * @return IFst::DirEnt, or nullptr if end of directory or on error.
	 * (TODO: Add lastError()?)
	 */
	virtual IFst::DirEnt *readdir(IFst::Dir *dirp);

	/**
	 * Close an opened directory.
	 * @param dirp IFst::Dir pointer.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int closedir(IFst::Dir *dirp);

	/**
	 * Open a file. (read-only)
	 * @param filename Filename.
	 * @return IRpFile*, or nullptr on error.
	 */
	virtual LibRpFile::IRpFilePtr open(const char *filename);
};

typedef std::shared_ptr<IPartition> IPartitionPtr;

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IPartition::~IPartition() = default;

}
