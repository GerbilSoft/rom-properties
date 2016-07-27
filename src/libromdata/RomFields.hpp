/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomFields.hpp: ROM fields class.                                        *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_ROMFIELDS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_ROMFIELDS_HPP__

#include <stdint.h>
#include <string>
#include <vector>
#include "TextFuncs.hpp"

namespace LibRomData {

class RomFieldsPrivate;
class RomFields
{
	public:
		// ROM field types.
		enum RomFieldType {
			RFT_INVALID,	// Invalid
			RFT_STRING,	// Basic string.
			RFT_BITFIELD,	// Bitfield.
			RFT_LISTDATA,	// ListData.
		};

		// Description for Bitfield.
		struct BitfieldDesc {
			// Number of bits to check. (must be 1-32)
			int elements;
			// Bit flags per row. (3 or 4 is usually good)
			int elemsPerRow;
			// Bit flag names.
			// Must be an array of at least 'elements' strings.
			// If a name is nullptr, that element is skipped.
			const rp_char **names;
		};

		// Description for ListData.
		struct ListDataDesc {
			// Number of fields per row.
			int count;
			// List field names. (headers)
			// Must be an array of at least 'fields' strings.
			// If a name is nullptr, that field is skipped.
			const rp_char **names;
		};

		// The ROM data class holds a number of customizable fields.
		// These fields are hard-coded by the subclass and passed
		// to the constructor.
		struct Desc {
			const rp_char *name;	// Display name.
			RomFieldType type;	// ROM field type.

			// Some types require more information.
			// The following pointer must point to the required
			// data structure for the type.
			union {
				const void *ptr;
				const BitfieldDesc *bitfield;
				const ListDataDesc *list_data;
			};
		};

		// List data for a list view.
		struct ListData {
			// Each entry in 'data' contains a vector of strings,
			// which represents each field.
			std::vector<std::vector<rp_string> > data;
		};

		// ROM field data.
		// Actual contents depend on the field type.
		struct Data {
			RomFieldType type;	// ROM field type.
			union {
				const rp_char *str;	// String data.
				uint32_t bitfield;	// Bitfield.
				ListData *list_data;	// ListData
			};
		};

	public:
		/**
		 * Initialize a ROM Fields class.
		 * @param fields Array of fields.
		 * @param count Number of fields.
		 */
		RomFields(const Desc *fields, int count);
		~RomFields();
	public:
		RomFields(const RomFields &other);
		RomFields &operator=(const RomFields &other);

	private:
		friend class RomFieldsPrivate;
		RomFieldsPrivate *d;

	public:
		/** Field accessors. **/

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
		const Desc *desc(int idx) const;

		/**
		 * Get the data for a ROM field.
		 * @param idx Field index.
		 * @return ROM field data, or nullptr if the index is invalid.
		 */
		const Data *data(int idx) const;

		/**
		 * Is data loaded?
		 * @return True if m_data has at least one row; false if m_data is nullptr or empty.
		 */
		bool isDataLoaded(void) const;

	private:
		/**
		 * Detach this instance from all other instances.
		 */
		void detach(void);

	public:
		/** Convenience functions for RomData subclasses. **/

		/**
		 * Add string field data.
		 * @param str String.
		 * @return Field index.
		 */
		int addData_string(const rp_char *str);

		/**
		 * Add string field data.
		 * @param str String.
		 * @return Field index.
		 */
		int addData_string(const rp_string &str);

		enum Base {
			FB_DEC,
			FB_HEX,
			FB_OCT,
		};

		/**
		 * Add string field data using a numeric value.
		 * @param val Numeric value.
		 * @param base Base. If not decimal, a prefix will be added.
		 * @param digits Number of leading digits. (0 for none)
		 * @return Field index.
		 */
		int addData_string_numeric(uint32_t val, Base base, int digits = 0);

		/**
		 * Add bitfield data.
		 * @param val Bitfield.
		 * @return Field index.
		 */
		int addData_bitfield(uint32_t bitfield);

		/**
		 * Add ListData.
		 * @param list_data ListData. (must be allocated with new)
		 * @return Field index.
		 */
		int addData_listData(ListData *list_data);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMFIELDS_HPP__ */
