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
			RFT_INVALID,	// Invalid. (skips the field)
			RFT_STRING,	// Basic string.
			RFT_BITFIELD,	// Bitfield.
			RFT_LISTDATA,	// ListData.
			RFT_DATETIME,	// Date/Time.
		};

		// Description for String.
		struct StringDesc {
			enum StringFormat {
				// Print the string using a monospaced font.
				STRF_MONOSPACE	= (1 << 0),

				// Print the string using a "warning" font.
				// (usually bold and red)
				STRF_WARNING	= (1 << 1),

				// "Credits" field.
				// Used for providing credits for an external database.
				// This field disables highlighting and enables links
				// using HTML-style "<a>" tags. This field is also
				// always shown at the bottom of the dialog and with
				// center-aligned text.
				// NOTE: Maximum of one STRF_CREDITS per RomData subclass.
				STRF_CREDITS	= (1 << 2),
			};

			// Custom formatting options.
			// (See the StringFormat enum.)
			uint32_t formatting;
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
			const rp_char *const *names;
		};

		// Description for ListData.
		struct ListDataDesc {
			// Number of fields per row.
			int count;
			// List field names. (headers)
			// Must be an array of at least 'fields' strings.
			// If a name is nullptr, that field is skipped.
			const rp_char *const *names;
		};

		// Display flags for RFT_DATETIME.
		enum DateTimeFlags {
			// Show the date value.
			RFT_DATETIME_HAS_DATE = (1 << 0),

			// Show the time value.
			RFT_DATETIME_HAS_TIME = (1 << 1),

			// Mask for date/time display values.
			RFT_DATETIME_HAS_DATETIME_MASK = RFT_DATETIME_HAS_DATE | RFT_DATETIME_HAS_TIME,

			// Show the timestamp as UTC instead of the local timezone.
			// This is useful for timestamps that aren't actually
			// adjusted for the local timezone.
			RFT_DATETIME_IS_UTC = (1 << 2),
		};

		// Description for RFT_DATETIME.
		struct DateTimeDesc {
			uint32_t flags;	// DateTimeFlags
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
				const StringDesc *str_desc;	// May be nullptr.
				const BitfieldDesc *bitfield;
				const ListDataDesc *list_data;
				const DateTimeDesc *date_time;
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

				// Date/Time. (UNIX format)
				// NOTE: -1 is used to indicate
				// an invalid date/time.
				int64_t date_time;
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
		 * TODO: Move to RomFieldsPrivate?
		 */
		void detach(void);

	public:
		/** Convenience functions for RomData subclasses. **/

		/**
		 * Add invalid field data.
		 * This effectively hides the field.
		 * @return Field index.
		 */
		int addData_invalid(void);

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
		int addData_string_numeric(uint32_t val, Base base = FB_DEC, int digits = 0);
		
		/**
		 * Add a string field formatted like a hex dump
		 * @param buf Input bytes.
		 * @param size Byte count.
		 * @return Field index.
		 */
		int addData_string_hexdump(const uint8_t *buf, size_t size);

		/**
		 * Add a string field formatted for an address range.
		 * @param start Start address.
		 * @param end End address.
		 * @param suffix Suffix string.
		 * @param digits Number of leading digits. (default is 8 for 32-bit)
		 * @return Field index.
		 */
		int addData_string_address_range(uint32_t start, uint32_t end,
					const rp_char *suffix, int digits = 8);

		/**
		 * Add a string field formatted for an address range.
		 * @param start Start address.
		 * @param end End address.
		 * @param digits Number of leading digits. (default is 8 for 32-bit)
		 * @return Field index.
		 */
		inline int addData_string_address_range(uint32_t start, uint32_t end, int digits = 8)
		{
			return addData_string_address_range(start, end, nullptr, digits);
		}

		/**
		 * Add bitfield data.
		 * @param bitfield Bitfield.
		 * @return Field index.
		 */
		int addData_bitfield(uint32_t bitfield);

		/**
		 * Add ListData.
		 * @param list_data ListData. (must be allocated with new)
		 * @return Field index.
		 */
		int addData_listData(ListData *list_data);

		/**
		 * Add DateTime.
		 * @param date_time Date/Time.
		 * @return Field index.
		 */
		int addData_dateTime(int64_t date_time);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMFIELDS_HPP__ */
