/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PEResourceReader.hpp: Portable Executable resource reader.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/disc/IResourceReader.hpp"

namespace LibRomData {

class PEResourceReaderPrivate;
class PEResourceReader final : public LibRpBase::IResourceReader
{
public:
	/**
	 * Construct a PEResourceReader with the specified IRpFile.
	 *
	 * NOTE: The IRpFile *must* remain valid while this
	 * PEResourceReader is open.
	 *
	 * @param file IRpFile.
	 * @param rsrc_addr .rsrc section start offset.
	 * @param rsrc_size .rsrc section size.
	 * @param rsrc_va .rsrc virtual address.
	 */
	PEResourceReader(const LibRpFile::IRpFilePtr &file, uint32_t rsrc_addr, uint32_t rsrc_size, uint32_t rsrc_va);
	~PEResourceReader() final;

private:
	typedef LibRpBase::IResourceReader super;
	RP_DISABLE_COPY(PEResourceReader)
protected:
	friend class PEResourceReaderPrivate;
	PEResourceReaderPrivate *const d_ptr;

public:
	/** IDiscReader **/

	/**
	 * Read data from the partition.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) final;

	/**
	 * Set the partition position.
	 * @param pos Partition position.
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos) final;

	/**
	 * Get the partition position.
	 * @return Partition position on success; -1 on error.
	 */
	off64_t tell(void) final;

	/**
	 * Get the data size.
	 * This size does not include the partition header,
	 * and it's adjusted to exclude hashes.
	 * @return Data size, or -1 on error.
	 */
	off64_t size(void) final;

public:
	/** IPartition **/

	/**
	 * Get the partition size.
	 * This size includes the partition header and hashes.
	 * @return Partition size, or -1 on error.
	 */
	off64_t partition_size(void) const final;

	/**
	 * Get the used partition size.
	 * This size includes the partition header and hashes,
	 * but does not include "empty" sectors.
	 * @return Used partition size, or -1 on error.
	 */
	off64_t partition_size_used(void) const final;

public:
	/** IResourceReader functions **/

	/**
	 * Open a resource.
	 * @param type	[in] Resource type ID
	 * @param id	[in] Resource ID (-1 for "first entry")
	 * @param lang	[in] Language ID (-1 for "first entry")
	 * @return IRpFile*, or nullptr on error.
	 */
	LibRpFile::IRpFilePtr open(uint16_t type, int id, int lang) final;

#ifdef _WIN32
	open_MAKEINTRESOURCE_wrapper(LPSTR);
	open_MAKEINTRESOURCE_wrapper(LPCSTR);
	open_MAKEINTRESOURCE_wrapper(LPWSTR);
	open_MAKEINTRESOURCE_wrapper(LPCWSTR);
#endif /* _WIN32 */

	/**
	 * Load a VS_VERSION_INFO resource.
	 * Data will be byteswapped to host-endian if necessary.
	 * @param id		[in] Resource ID (-1 for "first entry")
	 * @param lang		[in] Language ID (-1 for "first entry")
	 * @param pVsFfi	[out] VS_FIXEDFILEINFO (host-endian)
	 * @param pVsSfi	[out] StringFileInfo section.
	 * @return 0 on success; non-zero on error.
	 */
	int load_VS_VERSION_INFO(int id, int lang, VS_FIXEDFILEINFO *pVsFfi, StringFileInfo *pVsSfi) final;

	/**
	 * Look up a resource ID given a zero-based index.
	 * Mostly useful for icon indexes.
	 *
	 * @param type	[in] Resource type ID
	 * @param index	[in] Zero-based index
	 * @return Resource ID, or negative POSIX error code on error.
	 */
	int lookup_resource_ID(int type, int index) final;
};

}
