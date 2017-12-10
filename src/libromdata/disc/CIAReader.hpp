/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CIAReader.hpp: Nintendo 3DS CIA reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_CIAREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_CIAREADER_HPP__

#include "../Handheld/n3ds_structs.h"

// librpbase
#include "librpbase/disc/IPartition.hpp"

namespace LibRpBase {
	class IRpFile;
}

namespace LibRomData {

class CIAReaderPrivate;
class CIAReader : public LibRpBase::IPartition
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
		CIAReader(LibRpBase::IRpFile *file,
			int64_t content_offset, uint32_t content_length,
			const N3DS_Ticket_t *ticket,
			uint16_t tmd_content_index);
		virtual ~CIAReader();

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(CIAReader)

	protected:
		friend class CIAReaderPrivate;
		CIAReaderPrivate *const d_ptr;

	public:
		/** IDiscReader **/

		/**
		 * Is the partition open?
		 * This usually only returns false if an error occurred.
		 * @return True if the partition is open; false if it isn't.
		 */
		virtual bool isOpen(void) const override final;

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) override final;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) override final;

		/**
		 * Seek to the beginning of the partition.
		 */
		virtual void rewind(void) override final;

		/**
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		virtual int64_t tell(void) override final;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		virtual int64_t size(void) override final;

	public:
		/** IPartition **/

		/**
		 * Get the partition size.
		 * This size includes the partition header and hashes.
		 * @return Partition size, or -1 on error.
		 */
		virtual int64_t partition_size(void) const override final;

		/**
		 * Get the used partition size.
		 * This size includes the partition header and hashes,
		 * but does not include "empty" sectors.
		 * @return Used partition size, or -1 on error.
		 */
		virtual int64_t partition_size_used(void) const override final;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_CIAREADER_HPP__ */
