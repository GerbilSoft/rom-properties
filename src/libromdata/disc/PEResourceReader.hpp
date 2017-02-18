/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PEResourceReader.hpp: Portable Executable resource reader.              *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_PERESOURCEREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_PERESOURCEREADER_HPP__

#include "IPartition.hpp"

namespace LibRomData {

class IRpFile;

class PEResourceReaderPrivate;
class PEResourceReader : public IPartition
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
		PEResourceReader(IRpFile *file, uint32_t rsrc_addr, uint32_t rsrc_size, uint32_t rsrc_va);
		~PEResourceReader();

	private:
		RP_DISABLE_COPY(PEResourceReader)
	protected:
		friend class PEResourceReaderPrivate;
		PEResourceReaderPrivate *const d_ptr;

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
		virtual size_t read(void *ptr, size_t size) override;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) override;

		/**
		 * Seek to the beginning of the partition.
		 */
		virtual void rewind(void) override final;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		virtual int64_t size(void) override final;

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

	public:
		/** Resource access functions. **/

		/**
		 * Open a resource.
		 * @param type Resource type ID.
		 * @param id Resource ID. (-1 for "first entry")
		 * @param lang Language ID. (-1 for "first entry")
		 * @return IRpFile*, or nullptr on error.
		 */
		IRpFile *open(uint16_t type, int id, int lang);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_PERESOURCEREADER_HPP__ */
