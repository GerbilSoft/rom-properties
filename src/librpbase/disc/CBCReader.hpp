/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * CBCReader.hpp: AES-128-CBC data reader class.                           *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "IDiscReader.hpp"

#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

namespace LibRpBase {

class CBCReaderPrivate;
class CBCReader final : public LibRpBase::IDiscReader
{
public:
	/**
	 * Construct a CBCReader with the specified IRpFile.
	 *
	 * NOTE: The IRpFile *must* remain valid while this
	 * CBCReader is open.
	 *
	 * @param file 		[in] IRpFile
	 * @param offset	[in] Encrypted data start offset, in bytes.
	 * @param length	[in] Encrypted data length, in bytes.
	 * @param key		[in] Encryption key. (Must be 128-bit) [If NULL, acts like no encryption.]
	 * @param iv		[in] Initialization vector. (Must be 128-bit) [If NULL, uses ECB instead of CBC.]
	 */
	RP_LIBROMDATA_PUBLIC
	CBCReader(const LibRpFile::IRpFilePtr &file, off64_t offset, off64_t length,
		const uint8_t *key, const uint8_t *iv);
public:
	RP_LIBROMDATA_PUBLIC
	~CBCReader() final;

private:
	typedef IDiscReader super;
	RP_DISABLE_COPY(CBCReader)

protected:
	friend class CBCReaderPrivate;
	CBCReaderPrivate *const d_ptr;

public:
	/** IDiscReader **/

	/**
	 * isDiscSupported() is not handled by CBCReader.
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
	/* FIXME: ATTR_ACCESS_SIZE() is causing an ICE on gcc-13.1.0.
	ATTR_ACCESS_SIZE(write_only, 2, 3) */
	RP_LIBROMDATA_PUBLIC
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
	RP_LIBROMDATA_PUBLIC
	off64_t tell(void) final;

	/**
	 * Get the data size.
	 * This size does not include the partition header,
	 * and it's adjusted to exclude hashes.
	 * @return Data size, or -1 on error.
	 */
	RP_LIBROMDATA_PUBLIC
	off64_t size(void) final;
};

typedef std::shared_ptr<CBCReader> CBCReaderPtr;

}
