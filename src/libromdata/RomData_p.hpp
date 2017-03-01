/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData_p.hpp: ROM data base class. (PRIVATE CLASS)                     *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_ROMDATA_P_HPP__
#define __ROMPROPERTIES_LIBROMDATA_ROMDATA_P_HPP__

// C includes.
#include <stdint.h>

// C++ includes.
#include <vector>

#include "RomData.hpp"

namespace LibRomData {

class IRpFile;
class RomData;
class RomFields;
class rp_image;

class RomData;
class RomDataPrivate
{
	public:
		/**
		 * Initialize a RomDataPrivate storage class.
		 *
		 * @param q RomData class.
		 * @param file ROM file.
		 * @param fields Array of ROM Field descriptions.
		 * @param count Number of ROM Field descriptions.
		 */
		RomDataPrivate(RomData *q, IRpFile *file);

		/**
		 * Initialize a RomDataPrivate storage class.
		 *
		 * @param q RomData class.
		 * @param file ROM file.
		 * @param fields Array of ROM Field descriptions.
		 * @param count Number of ROM Field descriptions.
		 */
		DEPRECATED RomDataPrivate(RomData *q, IRpFile *file, const RomFields::Desc *fields, int count);

		virtual ~RomDataPrivate();

	private:
		RP_DISABLE_COPY(RomDataPrivate)
	protected:
		friend class RomData;
		RomData *const q_ptr;

	public:
		int ref_count;			// Reference count.
		bool isValid;			// Subclass must set this to true if the ROM is valid.
		IRpFile *file;			// Open file.
		RomFields *const fields;	// ROM fields.

		// File type. (default is FTYPE_ROM_IMAGE)
		RomData::FileType fileType;

		// Internal images.
		rp_image *images[RomData::IMG_INT_MAX - RomData::IMG_INT_MIN + 1];

		// Image processing flags for internal images.
		// External images use RomData::imgpf_extURL().
		uint32_t imgpf[RomData::IMG_INT_MAX+1];

	public:
		/** Convenience functions. **/

		/**
		 * Format a file size.
		 * @param fileSize File size.
		 * @return Formatted file size.
		 */
		static rp_string formatFileSize(int64_t fileSize);

		/**
		 * Get the GameTDB URL for a given game.
		 * @param system System name.
		 * @param type Image type.
		 * @param region Region name.
		 * @param gameID Game ID.
		 * @param ext File extension, e.g. ".png" or ".jpg".
		 * TODO: PAL multi-region selection?
		 * @return GameTDB URL.
		 */
		static rp_string getURL_GameTDB(
			const char *system, const char *type,
			const char *region, const char *gameID,
			const char *ext);

		/**
		 * Get the GameTDB cache key for a given game.
		 * @param system System name.
		 * @param type Image type.
		 * @param region Region name.
		 * @param gameID Game ID.
		 * @param ext File extension, e.g. ".png" or ".jpg".
		 * TODO: PAL multi-region selection?
		 * @return GameTDB cache key.
		 */
		static rp_string getCacheKey_GameTDB(
			const char *system, const char *type,
			const char *region, const char *gameID,
			const char *ext);

		/**
		 * Select the best size for an image.
		 * @param sizeDefs Image size definitions.
		 * @param size Requested thumbnail dimension. (assuming a square thumbnail)
		 * @return Image size definition, or nullptr on error.
		 */
		static const RomData::ImageSizeDef *selectBestSize(const std::vector<RomData::ImageSizeDef> &sizeDefs, int size);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATA_P_HPP__ */
