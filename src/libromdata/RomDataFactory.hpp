/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataFactory.hpp: RomData factory class.                              *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_ROMDATAFACTORY_HPP__
#define __ROMPROPERTIES_LIBROMDATA_ROMDATAFACTORY_HPP__

#include "config.libromdata.h"
#include "common.h"

// C++ includes.
#include <vector>

namespace LibRomData {

class IRpFile;

class RomData;
class RomDataFactory
{
	private:
		RomDataFactory();
		~RomDataFactory();
	private:
		RP_DISABLE_COPY(RomDataFactory)

	public:
		/**
		 * Create a RomData class for the specified ROM file.
		 *
		 * NOTE: RomData::isValid() is checked before returning a
		 * created RomData instance, so returned objects can be
		 * assumed to be valid as long as they aren't nullptr.
		 *
		 * If imgbf is non-zero, at least one of the specified image
		 * types must be supported by the RomData class in order to
		 * be returned.
		 *
		 * @param file ROM file.
		 * @param thumbnail If true, RomData class must support at least one image type.
		 * @return RomData class, or nullptr if the ROM isn't supported.
		 */
		static RomData *create(IRpFile *file, bool thumbnail = false);

		struct ExtInfo {
			const rp_char *ext;
			bool hasThumbnail;
		};

		/**
		 * Get all supported file extensions.
		 * Used for Win32 COM registration.
		 *
		 * NOTE: The return value is a struct that includes a flag
		 * indicating if the file type handler supports thumbnails.
		 *
		 * @return All supported file extensions, including the leading dot.
		 */
		static std::vector<ExtInfo> supportedFileExtensions(void);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATAFACTORY_HPP__ */
