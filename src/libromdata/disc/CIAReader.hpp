/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CIAReader.hpp: Nintendo 3DS CIA reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "../Handheld/n3ds_structs.h"

// librpbase
#include "librpbase/disc/IPartition.hpp"

namespace LibRomData {

class CIAReaderPrivate;
class CIAReader final : public LibRpBase::IPartition
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
		CIAReader(const std::shared_ptr<LibRpFile::IRpFile> &file,
			off64_t content_offset, uint32_t content_length,
			const N3DS_Ticket_t *ticket,
			uint16_t tmd_content_index);
	protected:
		~CIAReader() final;	// call unref() instead

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(CIAReader)

	protected:
		friend class CIAReaderPrivate;
		CIAReaderPrivate *const d_ptr;

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
};

}
