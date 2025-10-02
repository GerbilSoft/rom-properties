/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CIAReader.hpp: Nintendo 3DS CIA reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "../Handheld/n3ds_structs.h"

// librpbase
#include "librpbase/disc/IDiscReader.hpp"

namespace LibRomData {

class CIAReaderPrivate;
class CIAReader final : public LibRpBase::IDiscReader
{
public:
	/**
	 * Construct a CIAReader with the specified IRpFile.
	 *
	 * NOTE: The IRpFile *must* remain valid while this
	 * CIAReader is open.
	 *
	 * @param file 			[in] IRpFile.
	 * @param content_offset	[in] Content start offset, in bytes.
	 * @param content_length	[in] Content length, in bytes.
	 * @param ticket		[in,opt] Ticket for decryption. (nullptr if NoCrypto)
	 * @param tmd_content_index	[in,opt] TMD content index for decryption.
	 */
	CIAReader(const LibRpFile::IRpFilePtr &file,
		off64_t content_offset, uint32_t content_length,
		const N3DS_Ticket_t *ticket,
		uint16_t tmd_content_index);
public:
	~CIAReader() final;

private:
	typedef IDiscReader super;
	RP_DISABLE_COPY(CIAReader)

protected:
	friend class CIAReaderPrivate;
	CIAReaderPrivate *const d_ptr;

public:
	/** IDiscReader **/

	/**
	 * isDiscSupported() is not handled by CIAReader.
	 * @return -1
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const final
	{
		RP_UNUSED(pHeader);
		RP_UNUSED(szHeader);
		return -1;
	}

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
	 * @param pos		[in] Partition position
	 * @param whence	[in] Where to seek from
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos, SeekWhence whence) final;

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
};

typedef std::shared_ptr<CIAReader> CIAReaderPtr;

}
