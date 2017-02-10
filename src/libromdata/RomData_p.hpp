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

		// Lists of URLs and cache keys for external media types.
		// Each vector contains a list of URLs for the given
		// media type, in priority order. ([0] == highest priority)
		// This is done to allow for multiple quality levels.
		// TODO: Allow the user to customize quality levels?
		std::vector<RomData::ExtURL> extURLs[RomData::IMG_EXT_MAX - RomData::IMG_EXT_MIN + 1];

		// Image processing flags.
		uint32_t imgpf[RomData::IMG_EXT_MAX+1];

	public:
		/** Convenience functions. **/

		/**
		 * Format a file size.
		 * @param fileSize File size.
		 * @return Formatted file size.
		 */
		static rp_string formatFileSize(int64_t fileSize);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATA_P_HPP__ */
