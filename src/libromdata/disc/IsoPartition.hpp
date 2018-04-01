/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IsoPartition.hpp: ISO-9660 partition reader.                            *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_ISOPARTITION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_ISOPARTITION_HPP__

#include "librpbase/disc/IPartition.hpp"
#include "librpbase/disc/IFst.hpp"

namespace LibRpBase {
	class IRpFile;
}

namespace LibRomData {

class IsoPartitionPrivate;
class IsoPartition : public LibRpBase::IPartition
{
	public:
		/**
		 * Construct an IsoPartition with the specified IDiscReader.
		 *
		 * NOTE: The IDiscReader *must* remain valid while this
		 * IsoPartition is open.
		 *
		 * @param discReader IDiscReader.
		 * @param partition_offset Partition start offset.
		 * @@param iso_start_offset ISO start offset, in blocks. (If -1, uses heuristics.)
		 */
		IsoPartition(IDiscReader *discReader, int64_t partition_offset, int iso_start_offset = -1);
		virtual ~IsoPartition();

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(IsoPartition)

	protected:
		friend class IsoPartitionPrivate;
		IsoPartitionPrivate *const d_ptr;

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
		size_t read(void *ptr, size_t size) override;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) override;

		/**
		 * Seek to the beginning of the partition.
		 */
		void rewind(void) final;

		/**
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		int64_t tell(void) override;

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

		/** IFst wrapper functions. **/

		// TODO
#if 0
		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return IFst::Dir*, or nullptr on error.
		 */
		LibRpBase::IFst::Dir *opendir(const char *path);

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return IFst::Dir*, or nullptr on error.
		 */
		inline LibRpBase::IFst::Dir *opendir(const std::string &path)
		{
			return opendir(path.c_str());
		}

		/**
		 * Read a directory entry.
		 * @param dirp IFst::Dir pointer.
		 * @return IFst::DirEnt, or nullptr if end of directory or on error.
		 * (TODO: Add lastError()?)
		 */
		LibRpBase::IFst::DirEnt *readdir(LibRpBase::IFst::Dir *dirp);

		/**
		 * Close an opened directory.
		 * @param dirp IFst::Dir pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int closedir(LibRpBase::IFst::Dir *dirp);
#endif

		/**
		 * Open a file. (read-only)
		 * @param filename Filename.
		 * @return IRpFile*, or nullptr on error.
		 */
		LibRpBase::IRpFile *open(const char *filename);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_ISOPARTITION_HPP__ */
