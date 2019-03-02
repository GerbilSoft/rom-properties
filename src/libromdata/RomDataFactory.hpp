/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataFactory.hpp: RomData factory class.                              *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_ROMDATAFACTORY_HPP__
#define __ROMPROPERTIES_LIBROMDATA_ROMDATAFACTORY_HPP__

#include "librpbase/common.h"

// C++ includes.
#include <vector>

namespace LibRpBase {
	class RomData;
	class IRpFile;
}

namespace LibRomData {

class RomDataFactory
{
	private:
		RomDataFactory();
		~RomDataFactory();
	private:
		RP_DISABLE_COPY(RomDataFactory)

	public:
		/**
		 * Bitfield of RomData subclass attributes.
		 */
		enum RomDataAttr {
			// RomData subclass has no attributes.
			RDA_NONE		= 0,

			// RomData subclass has thumbnails.
			RDA_HAS_THUMBNAIL	= (1 << 0),

			// RomData subclass may have "dangerous" permissions.
			RDA_HAS_DPOVERLAY	= (1 << 1),

			// Check for game-specific disc file systems.
			// (For internal RomDataFactory use only.)
			RDA_CHECK_ISO		= (1 << 2),
		};

		/**
		 * Create a RomData subclass for the specified ROM file.
		 *
		 * NOTE: RomData::isValid() is checked before returning a
		 * created RomData instance, so returned objects can be
		 * assumed to be valid as long as they aren't nullptr.
		 *
		 * If imgbf is non-zero, at least one of the specified image
		 * types must be supported by the RomData subclass in order to
		 * be returned.
		 *
		 * @param file ROM file.
		 * @param attrs RomDataAttr bitfield. If set, RomData subclass must have the specified attributes.
		 * @return RomData subclass, or nullptr if the ROM isn't supported.
		 */
		static LibRpBase::RomData *create(LibRpBase::IRpFile *file, unsigned int attrs = 0);

		struct ExtInfo {
			const char *ext;
			unsigned int attrs;

			ExtInfo(const char *ext, unsigned int attrs)
				: ext(ext)
				, attrs(attrs)
				{ }
		};

		/**
		 * Get all supported file extensions.
		 * Used for Win32 COM registration.
		 *
		 * NOTE: The return value is a struct that includes a flag
		 * indicating if the file type handler supports thumbnails
		 * and/or may have "dangerous" permissions.
		 *
		 * @return All supported file extensions, including the leading dot.
		 */
		static const std::vector<ExtInfo> &supportedFileExtensions(void);

		/**
		 * Get all supported MIME types.
		 * Used for KFileMetaData.
		 *
		 * @return All supported MIME types.
		 */
		static const std::vector<const char*> &supportedMimeTypes(void);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATAFACTORY_HPP__ */
