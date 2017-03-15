/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnPartition.hpp: GameCube partition reader.                            *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_GCNPARTITION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_GCNPARTITION_HPP__

#include "IPartition.hpp"
#include "GcnFst.hpp"

namespace LibRomData {

class GcnPartitionPrivate;
class GcnPartition : public IPartition
{
	public:
		/**
		 * Construct a GcnPartition with the specified IDiscReader.
		 *
		 * NOTE: The IDiscReader *must* remain valid while this
		 * GcnPartition is open.
		 *
		 * TODO: TGC support?
		 *
		 * @param discReader IDiscReader.
		 * @param partition_offset Partition start offset.
		 */
		GcnPartition(IDiscReader *discReader, int64_t partition_offset);
		virtual ~GcnPartition();
	protected:
		explicit GcnPartition(GcnPartitionPrivate *d);

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(GcnPartition)

	protected:
		friend class GcnPartitionPrivate;
		GcnPartitionPrivate *const d_ptr;

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
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		virtual int64_t tell(void) override;

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

		/** IFst wrapper functions. **/

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return IFst::Dir*, or nullptr on error.
		 */
		IFst::Dir *opendir(const rp_char *path);

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return IFst::Dir*, or nullptr on error.
		 */
		inline IFst::Dir *opendir(const LibRomData::rp_string &path)
		{
			return opendir(path.c_str());
		}

		/**
		 * Read a directory entry.
		 * @param dirp IFst::Dir pointer.
		 * @return IFst::DirEnt, or nullptr if end of directory or on error.
		 * (TODO: Add lastError()?)
		 */
		IFst::DirEnt *readdir(IFst::Dir *dirp);

		/**
		 * Close an opened directory.
		 * @param dirp IFst::Dir pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int closedir(IFst::Dir *dirp);

		/**
		 * Open a file. (read-only)
		 * @param filename Filename.
		 * @return IRpFile*, or nullptr on error.
		 */
		IRpFile *open(const rp_char *filename);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_GCNPARTITION_HPP__ */
