/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * CBCReader.hpp: AES-128-CBC data reader class.                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "IPartition.hpp"

namespace LibRpBase {

class CBCReaderPrivate;
class CBCReader final : public LibRpBase::IPartition
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
		CBCReader(const LibRpFile::IRpFilePtr &file, off64_t offset, off64_t length,
			const uint8_t *key, const uint8_t *iv);
	protected:
		~CBCReader() final;	// call unref() instead

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(CBCReader)

	protected:
		friend class CBCReaderPrivate;
		CBCReaderPrivate *const d_ptr;

	public:
		/** IDiscReader **/

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		/* FIXME: ATTR_ACCESS_SIZE() is causing an ICE on gcc-13.1.0.
		ATTR_ACCESS_SIZE(write_only, 2, 3) */
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
};

}
