/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader.hpp: Nintendo 3DS NCCH reader.                               *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_HPP__

#include "IPartition.hpp"
#include "../n3ds_structs.h"

namespace LibRomData {

class IRpFile;

class NCCHReaderPrivate;
class NCCHReader : public IPartition
{
	public:
		/**
		 * Construct an NCCHReader with the specified IRpFile.
		 *
		 * NOTE: The IRpFile *must* remain valid while this
		 * NCCHReader is open.
		 *
		 * @param file IRpFile.
		 * @param media_unit_shift Media unit shift.
		 * @param ncch_offset NCCH start offset, in bytes.
		 * @param ncch_length NCCH length, in bytes.
		 */
		NCCHReader(IRpFile *file, uint8_t media_unit_shift,
			  int64_t ncch_offset, uint32_t ncch_length);
		virtual ~NCCHReader();

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(NCCHReader)

	protected:
		friend class NCCHReaderPrivate;
		NCCHReaderPrivate *const d_ptr;

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

	public:
		/** NCCHReader **/

		/**
		 * Get the NCCH header.
		 * @return NCCH header, or nullptr if it couldn't be loaded.
		 */
		const N3DS_NCCH_Header_NoSig_t *ncchHeader(void) const;

		/**
		 * Get the NCCH extended header.
		 * @return NCCH extended header, or nullptr if it couldn't be loaded.
		 */
		const N3DS_NCCH_ExHeader_t *ncchExHeader(void) const;

		/**
		 * Get the ExeFS header.
		 * @return ExeFS header, or nullptr if it couldn't be loaded.
		 */
		const N3DS_ExeFS_Header_t *exefsHeader(void) const;

		/**
		 * Get the NCCH crypto type.
		 * @param pNcchHeader NCCH header.
		 * @return NCCH crypto type, or nullptr if unknown.
		 */
		static const rp_char *cryptoType_static(const N3DS_NCCH_Header_NoSig_t *pNcchHeader);

		/**
		 * Get the NCCH crypto type.
		 * @return NCCH crypto type, or nullptr if unknown.
		 */
		const rp_char *cryptoType(void) const;

		/**
		 * Open a file. (read-only)
		 *
		 * NOTE: Only ExeFS is currently supported.
		 *
		 * @param section NCCH section.
		 * @param filename Filename. (ASCII)
		 * @return IRpFile*, or nullptr on error.
		 */
		IRpFile *open(int section, const char *filename);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_HPP__ */
