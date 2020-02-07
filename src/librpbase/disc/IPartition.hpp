/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IPartition.hpp: Partition reader interface.                             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_DISC_IPARTITION_HPP__
#define __ROMPROPERTIES_LIBRPBASE_DISC_IPARTITION_HPP__

#include "IDiscReader.hpp"

namespace LibRpBase {

class IPartition : public IDiscReader
{
	protected:
		IPartition(IRpFile *file) : super(file) { }
		IPartition(IDiscReader *discReader) : super(discReader) { }
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
		virtual off64_t partition_size(void) const = 0;

		/**
		 * Get the used partition size.
		 * This includes the partition headers and any
		 * metadata, e.g. Wii sector hashes, if present.
		 * It does *not* include "empty" sectors.
		 * @return Used partition size, or -1 on error.
		 */
		virtual off64_t partition_size_used(void) const = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IPartition::~IPartition() { }

}

#endif /* __ROMPROPERTIES_LIBRPBASE_DISC_IPARTITION_HPP__ */
