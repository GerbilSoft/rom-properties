/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NEResourceReader.hpp: New Executable resource reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IResourceReader.hpp"

namespace LibRomData {

class NEResourceReaderPrivate;
class NEResourceReader final : public IResourceReader
{
public:
	/**
	 * Construct an NEResourceReader with the specified IRpFile.
	 *
	 * NOTE: The IRpFile *must* remain valid while this
	 * NEResourceReader is open.
	 *
	 * @param file IRpFile.
	 * @param rsrc_tbl_addr Resource table start address.
	 * @param rsrc_tbl_size Resource table size.
	 */
	NEResourceReader(const LibRpFile::IRpFilePtr &file, uint32_t rsrc_tbl_addr, uint32_t rsrc_tbl_size);
	~NEResourceReader() final;

private:
	typedef IResourceReader super;
	RP_DISABLE_COPY(NEResourceReader)
protected:
	friend class NEResourceReaderPrivate;
	NEResourceReaderPrivate *const d_ptr;

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
	 * @param type Resource type ID.
	 * @param id Resource ID. (-1 for "first entry")
	 * @param lang Language ID. (-1 for "first entry")
	 * @return IRpFile*, or nullptr on error.
	 */
	LibRpFile::IRpFilePtr open(uint16_t type, int id, int lang) final;

	/**
	 * Load a VS_VERSION_INFO resource.
	 * Data will be byteswapped to host-endian if necessary.
	 * @param id		[in] Resource ID. (-1 for "first entry")
	 * @param lang		[in] Language ID. (-1 for "first entry")
	 * @param pVsFfi	[out] VS_FIXEDFILEINFO (host-endian)
	 * @param pVsSfi	[out] StringFileInfo section.
	 * @return 0 on success; non-zero on error.
	 */
	int load_VS_VERSION_INFO(int id, int lang, VS_FIXEDFILEINFO *pVsFfi, StringFileInfo *pVsSfi) final;
};

}
