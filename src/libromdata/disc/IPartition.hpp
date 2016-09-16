/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IPartition.hpp: Partition reader interface.                             *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_IPARTITION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_IPARTITION_HPP__

#include "IDiscReader.hpp"

namespace LibRomData {

class IPartition : public IDiscReader
{
	protected:
		IPartition() { }
	public:
		virtual ~IPartition() = 0;

	private:
		IPartition(const IPartition &other);
		IPartition &operator=(const IPartition &other);

	public:
		/**
		 * Get the partition size.
		 * This includes the partition headers and any
		 * metadata, e.g. Wii sector hashes, if present.
		 * @return Partition size, or -1 on error.
		 */
		virtual int64_t partition_size(void) const = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IPartition::~IPartition() { }

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_IPARTITION_HPP__ */
