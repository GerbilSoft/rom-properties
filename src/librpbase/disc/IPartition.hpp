/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IPartition.hpp: Partition reader interface.                             *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_DISC_IPARTITION_HPP__
#define __ROMPROPERTIES_LIBRPBASE_DISC_IPARTITION_HPP__

#include "IDiscReader.hpp"

namespace LibRpBase {

class IPartition : public IDiscReader
{
	protected:
		IPartition() { }
	public:
		virtual ~IPartition() = 0;

	private:
		typedef IDiscReader super;
		RP_DISABLE_COPY(IPartition)

	public:
		/** IDiscReader **/

		/**
		 * isDiscSupported() is not handled by IPartition.
		 * TODO: Maybe implement it to determine if the partition type is supported?
		 * TODO: Move to IPartition.cpp?
		 * @return -1
		 */
		int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const final
		{
			RP_UNUSED(pHeader);
			RP_UNUSED(szHeader);
			return -1;
		}

	public:
		/**
		 * Get the partition size.
		 * This includes the partition headers and any
		 * metadata, e.g. Wii sector hashes, if present.
		 * @return Partition size, or -1 on error.
		 */
		virtual int64_t partition_size(void) const = 0;

		/**
		 * Get the used partition size.
		 * This includes the partition headers and any
		 * metadata, e.g. Wii sector hashes, if present.
		 * It does *not* include "empty" sectors.
		 * @return Used partition size, or -1 on error.
		 */
		virtual int64_t partition_size_used(void) const = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IPartition::~IPartition() { }

}

#endif /* __ROMPROPERTIES_LIBRPBASE_DISC_IPARTITION_HPP__ */
