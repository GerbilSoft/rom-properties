/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IPartition.hpp: Partition reader interface.                             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpfile/IRpFile.hpp"
#include "IDiscReader.hpp"

namespace LibRpBase {

class IPartition : public IDiscReader
{
	protected:
		explicit IPartition(const LibRpFile::IRpFilePtr &file) : super(file) { }
		explicit IPartition(const IDiscReaderPtr &discReader) : super(discReader) { }
	public:
		~IPartition() = 0;

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
		ATTR_ACCESS_SIZE(read_only, 2, 3)
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
