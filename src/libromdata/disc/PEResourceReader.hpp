/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PEResourceReader.hpp: Portable Executable resource reader.              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_PERESOURCEREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_PERESOURCEREADER_HPP__

#include "IResourceReader.hpp"

namespace LibRomData {

class PEResourceReaderPrivate;
class PEResourceReader : public IResourceReader
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
		PEResourceReader(LibRpBase::IRpFile *file, uint32_t rsrc_addr, uint32_t rsrc_size, uint32_t rsrc_va);
		virtual ~PEResourceReader();

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
		bool isOpen(void) const final;

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size) final;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Seek to the beginning of the partition.
		 */
		void rewind(void) final;

		/**
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		int64_t size(void) final;

		/** IPartition **/

		/**
		 * Get the partition size.
		 * This size includes the partition header and hashes.
		 * @return Partition size, or -1 on error.
		 */
		int64_t partition_size(void) const final;

		/**
		 * Get the used partition size.
		 * This size includes the partition header and hashes,
		 * but does not include "empty" sectors.
		 * @return Used partition size, or -1 on error.
		 */
		int64_t partition_size_used(void) const final;

	public:
		/** IResourceReader functions. **/

		/**
		 * Open a resource.
		 * @param type Resource type ID.
		 * @param id Resource ID. (-1 for "first entry")
		 * @param lang Language ID. (-1 for "first entry")
		 * @return IRpFile*, or nullptr on error.
		 */
		LibRpBase::IRpFile *open(uint16_t type, int id, int lang) final;

		/**
		 * Load a VS_VERSION_INFO resource.
		 * @param id		[in] Resource ID. (-1 for "first entry")
		 * @param lang		[in] Language ID. (-1 for "first entry")
		 * @param pVsFfi	[out] VS_FIXEDFILEINFO (host-endian)
		 * @param pVsSfi	[out] StringFileInfo section.
		 * @return 0 on success; non-zero on error.
		 */
		int load_VS_VERSION_INFO(int id, int lang, VS_FIXEDFILEINFO *pVsFfi, StringFileInfo *pVsSfi) final;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_PERESOURCEREADER_HPP__ */
