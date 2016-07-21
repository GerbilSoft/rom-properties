/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData.hpp: ROM data base class.                                       *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__

#include <stdint.h>
#include <string>
#include <vector>
#include "TextFuncs.hpp"

namespace LibRomData {

class RomData
{
	public:
		// ROM field types.
		enum RomFieldType {
			RFT_INVALID,	// Invalid
			RFT_STRING,	// Basic string.
			RFT_BITFIELD,	// Bitfield.
		};

		// The ROM data class holds a number of customizable fields.
		// These fields are hard-coded by the subclass and passed
		// to the constructor.
		struct RomFieldDesc {
			const rp_char *name;	// Display name.
			RomFieldType type;	// ROM field type.

			// Some types require more information.
			// TODO: Optimize by using a union?
			struct {
				// Number of bits to check. (must be 1-32)
				int elements;
				// Bitfield names.
				// Must be an array of at least 'elements' strings.
				// If a name is nullptr, that element is skipped.
				const rp_char **names;
			} bitfield;
		};

		// ROM field data.
		// Actual contents depend on the field type.
		struct RomFieldData {
			RomFieldType type;	// ROM field type.
			union {
				const rp_char *str;	// String data.
				uint32_t bitfield;	// Bitfield.
			};
		};

	protected:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * ROM data base class.
		 * Subclass must pass an array of RomFieldDesc structs.
		 */
		RomData(const RomFieldDesc *fields, int fieldCount);
	public:
		virtual ~RomData();

	private:
		RomData(const RomData &);
		RomData &operator=(const RomData &);

	public:
		/**
		 * Is this ROM valid?
		 * @return True if it is; false if it isn't.
		 */
		bool isValid(void) const;

	protected:
		// Subclass must set this to true if the ROM is valid.
		bool m_isValid;

	public:

		/**
		 * Get the number of fields.
		 * @return Number of fields.
		 */
		int count(void) const;

		/**
		 * Get a ROM field description.
		 * @param idx Field index.
		 * @return ROM field, or nullptr if the index is invalid.
		 */
		const RomFieldDesc *desc(int idx) const;

		/**
		 * Get the data for a ROM field.
		 * @param idx Field index.
		 * @return ROM field data, or nullptr if the index is invalid.
		 */
		const RomFieldData *data(int idx) const;

	private:
		// ROM field descriptions.
		const RomFieldDesc *const m_fields;
		int const m_fieldCount;

	private:
		// ROM field data.
		// This must be filled in using the convenience functions.
		// NOTE: Strings are *copied* into this vector (to prevent
		// std::string issues) and are deleted by the destructor.
		std::vector<RomFieldData> m_fieldData;

	protected:
		/** Convenience functions for m_fieldData. **/

		/**
		 * Add a string field.
		 * @param str String.
		 * @return Field index.
		 */
		int addField_string(const rp_char *str);

		/**
		 * Add a string field.
		 * @param str String.
		 * @return Field index.
		 */
		int addField_string(const rp_string &str);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__ */
