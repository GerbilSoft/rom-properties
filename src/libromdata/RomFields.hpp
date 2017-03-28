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

#include "config.libromdata.h"
#include "common.h"

#include <stdint.h>
#include <array>
#include <string>
#include <vector>

namespace LibRomData {

class RomFieldsPrivate;
class RomFields
{
	public:
		// ROM field types.
		enum RomFieldType {
			RFT_INVALID,		// Invalid. (skips the field)
			RFT_STRING,		// Basic string.
			RFT_BITFIELD,		// Bitfield.
			RFT_LISTDATA,		// ListData.
			RFT_DATETIME,		// Date/Time.
			RFT_AGE_RATINGS,	// Age ratings.
		};

		// String format flags. (RFT_STRING)
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

				// Age ratings. (pointer to uint16_t[16])
				// See AgeRatingsCountry for field indexes.
				uint16_t *age_ratings;
			};
		};

		// Age Ratings indexes.
		// These correspond to Wii and/or 3DS fields.
		enum AgeRatingsCountry {
			AGE_JAPAN	= 0,	// Japan (CERO)
			AGE_USA		= 1,	// USA (ESRB)
			AGE_GERMANY	= 3,	// Germany (USK)
			AGE_EUROPE	= 4,	// Europe (PEGI)
			AGE_FINLAND	= 5,	// Finland (MEKU)
			AGE_PORTUGAL	= 6,	// Portugal (PEGI-PT)
			AGE_ENGLAND	= 7,	// England (BBFC)
			AGE_AUSTRALIA	= 8,	// Australia (AGCB)
			AGE_SOUTH_KOREA	= 9,	// South Korea (GRB)
			AGE_TAIWAN	= 10,	// Taiwan (CGSRR)

			AGE_MAX		= 16	// Maximum number of age rating fields
		};

		// Age Ratings bitfields.
		enum AgeRatingsBitfield {
			AGEBF_MIN_AGE_MASK	= 0x001F,	// Low 5 bits indicate the minimum age.
			AGEBF_ACTIVE		= 0x0020,	// Rating is only valid if this is set.
			AGEBF_PENDING		= 0x0040,	// Rating is pending.
			AGEBF_NO_RESTRICTION	= 0x0080,	// No age restriction.
			AGEBF_ONLINE_PLAY	= 0x0100,	// Rating may change due to online play.
			AGEBF_PROHIBITED	= 0x0200,	// Game is specifically prohibited.
		};

		// Age Ratings type.
		typedef std::array<uint16_t, AGE_MAX> age_ratings_t;

		// ROM field struct.
		// Dynamically allocated.
		struct Field {
			rp_string name;		// Field name.
			RomFieldType type;	// ROM field type.
			uint8_t tabIdx;		// Tab index. (0 for default)
			bool isValid;		// True if this field has valid data.

			// Field description.
			union _desc {
				uint32_t flags;	// Generic flags. (string, date)

				struct _bitfield {
					// Number of bits to check. (must be 1-32)
					// TODO: Remove this field.
					int elements;
					// Bit flags per row. (3 or 4 is usually good)
					int elemsPerRow;
					// Bit flag names.
					// Must be a vector of at least 'elements' strings.
					// If a name is nullptr, that element is skipped.
					const std::vector<rp_string> *names;
				} bitfield;
				struct _list_data {
					// List field names. (headers)
					// Must be a vector of at least 'fields' strings.
					// If a name is nullptr, that field is skipped.
					const std::vector<rp_string> *names;
				} list_data;
			} desc;

			// Field data.
			union _data {
				// Generic data for NULL.
				uint64_t generic;

				// RFT_STRING
				const rp_string *str;

				// RFT_BITFIELD
				uint32_t bitfield;

				// RFT_LISTDATA
				const std::vector<std::vector<rp_string> > *list_data;

				// RFT_DATETIME (UNIX format)
				// NOTE: -1 is used to indicate
				// an invalid date/time.
				int64_t date_time;

				// RFT_AGE_RATINGS
				// See AgeRatingsCountry for field indexes.
				const age_ratings_t *age_ratings;
			} data;
		};

	public:
		/**
		 * Initialize a ROM Fields class.
		 */
		RomFields();

		/**
		 * Initialize a ROM Fields class.
		 * @param desc Array of field descriptions.
		 * @param count Number of fields.
		 */
		DEPRECATED RomFields(const Desc *desc, int count);
		~RomFields();
	public:
		RomFields(const RomFields &other);
		RomFields &operator=(const RomFields &other);

	private:
		friend class RomFieldsPrivate;
		RomFieldsPrivate *d_ptr;

	public:
		/** Field accessors. **/

		/**
		 * Get the number of fields.
		 * @return Number of fields.
		 */
		int count(void) const;

		/**
		 * Get a ROM field.
		 * @param idx Field index.
		 * @return ROM field, or nullptr if the index is invalid.
		 */
		const Field *field(int idx) const;

		/**
		 * Is data loaded?
		 * TODO: Rename to empty() after porting to the new addField() functions?
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
		/**
		 * Get the abbreviation of an age rating organization.
		 * (TODO: Full name function?)
		 * @param country Rating country. (See AgeRatingsCountry.)
		 * @return Abbreviation (in ASCII), or nullptr if invalid.
		 */
		static const char *ageRatingAbbrev(int country);

		/**
		 * Decode an age rating into a human-readable string.
		 * This does not include the name of the rating organization.
		 *
		 * NOTE: The returned string is in UTF-8 in order to
		 * be able to use special characters.
		 *
		 * @param country Rating country. (See AgeRatingsCountry.)
		 * @param rating Rating value.
		 * @return Human-readable string, or empty string if the rating isn't active.
		 */
		static std::string ageRatingDecode(int country, uint16_t rating);

	public:
		/** Convenience functions for RomData subclasses. **/
		/** OLD versions for statically-allocated fields. **/

		/**
		 * Add invalid field data.
		 * This effectively hides the field.
		 * @return Field index.
		 */
		DEPRECATED int addData_invalid(void);

		/**
		 * Add string field data.
		 * @param str String.
		 * @return Field index.
		 */
		DEPRECATED int addData_string(const rp_char *str);

		/**
		 * Add string field data.
		 * @param str String.
		 * @return Field index.
		 */
		DEPRECATED int addData_string(const rp_string &str);

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
		 * @return Field index, or -1 on error.
		 */
		DEPRECATED int addData_string_numeric(uint32_t val, Base base = FB_DEC, int digits = 0);
		
		/**
		 * Add a string field formatted like a hex dump
		 * @param buf Input bytes.
		 * @param size Byte count.
		 * @return Field index, or -1 on error.
		 */
		DEPRECATED int addData_string_hexdump(const uint8_t *buf, size_t size);

		/**
		 * Add bitfield data.
		 * @param bitfield Bitfield.
		 * @return Field index, or -1 on error.
		 */
		DEPRECATED int addData_bitfield(uint32_t bitfield);

		/**
		 * Add ListData.
		 * @param list_data ListData. (must be allocated with new)
		 * @return Field index, or -1 on error.
		 */
		DEPRECATED int addData_listData(ListData *list_data);

		/**
		 * Add DateTime.
		 * @param date_time Date/Time.
		 * @return Field index, or -1 on error.
		 */
		DEPRECATED int addData_dateTime(int64_t date_time);

		/**
		 * Add age ratings.
		 * @param age_ratings Age ratings array. (uint16_t[16])
		 * @return Field index, or -1 on error.
		 */
		DEPRECATED int addData_ageRatings(uint16_t age_ratings[AGE_MAX]);

	public:
		/** Convenience functions for RomData subclasses. **/
		/** NEW versions for dynamically-allocated fields. **/

		/** Tabs **/

		/**
		 * Reserve space for tabs.
		 * @param n Desired tab count.
		 */
		void reserveTabs(int n);

		/**
		 * Set the tab index for new fields.
		 * @param idx Tab index.
		 */
		void setTabIndex(int tabIdx);

		/**
		 * Set a tab name.
		 * NOTE: An empty tab name will hide the tab.
		 * @param tabIdx Tab index.
		 * @param name Tab name.
		 */
		void setTabName(int tabIdx, const rp_char *name);

		/**
		 * Add a tab to the end and select it.
		 * @param name Tab name.
		 * @return Tab index.
		 */
		int addTab(const rp_char *name);

		/**
		 * Get the tab count.
		 * @return Tab count. (highest tab index, plus 1)
		 */
		int tabCount(void) const;

		/**
		 * Get the name of the specified tab.
		 * @param tabIdx Tab index.
		 * @return Tab name, or nullptr if no name is set.
		 */
		const rp_char *tabName(int tabIdx) const;

		/** Fields **/

		/**
		 * Reserve space for fields.
		 * @param n Desired capacity.
		 */
		void reserve(int n);

		/**
		 * Convert an array of rp_char strings to a vector of rp_string.
		 * This can be used for addField_bitfield() and addField_listData().
		 * @param strArray Array of strings.
		 * @param count Number of strings, or -1 for a NULL-terminated array.
		 * NOTE: The array will be terminated at NULL regardless of count,
		 * so a -1 count is only useful if the size isn't known.
		 * @return Allocated std::vector<rp_string>.
		 */
		static std::vector<rp_string> *strArrayToVector(const rp_char *const *strArray, int count = -1);

		/**
		 * Add fields from another RomFields object.
		 * @param other Source RomFields object.
		 * @param tabOffset Tab index to add to the original tabs. (If -1, ignore the original tabs.)
		 * @return Field index of the last field added.
		 */
		int addFields_romFields(const RomFields *other, int tabOffset);

		/**
		 * Add string field data.
		 * @param name Field name.
		 * @param str String.
		 * @param flags Formatting flags.
		 * @return Field index.
		 */
		int addField_string(const rp_char *name, const rp_char *str, int flags = 0);

		/**
		 * Add string field data.
		 * @param name Field name.
		 * @param str String.
		 * @param flags Formatting flags.
		 * @return Field index.
		 */
		int addField_string(const rp_char *name, const rp_string &str, int flags = 0);

#if 0
		enum Base {
			FB_DEC,
			FB_HEX,
			FB_OCT,
		};
#endif

		/**
		 * Add string field data using a numeric value.
		 * @param name Field name.
		 * @param val Numeric value.
		 * @param base Base. If not decimal, a prefix will be added.
		 * @param digits Number of leading digits. (0 for none)
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_string_numeric(const rp_char *name, uint32_t val, Base base = FB_DEC, int digits = 0, int flags = 0);

		/**
		 * Add a string field formatted like a hex dump
		 * @param name Field name.
		 * @param buf Input bytes.
		 * @param size Byte count.
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_string_hexdump(const rp_char *name, const uint8_t *buf, size_t size, int flags = 0);

		/**
		 * Add a string field formatted for an address range.
		 * @param name Field name.
		 * @param start Start address.
		 * @param end End address.
		 * @param suffix Suffix string.
		 * @param digits Number of leading digits. (default is 8 for 32-bit)
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_string_address_range(const rp_char *name,
			uint32_t start, uint32_t end,
			const rp_char *suffix, int digits = 8, int flags = 0);

		/**
		 * Add a string field formatted for an address range.
		 * @param name Field name.
		 * @param start Start address.
		 * @param end End address.
		 * @param digits Number of leading digits. (default is 8 for 32-bit)
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		inline int addField_string_address_range(const rp_char *name,
			uint32_t start, uint32_t end, int digits = 8, int flags = 0)
		{
			return addField_string_address_range(name, start, end, nullptr, digits, flags);
		}

		/**
		 * Add bitfield data.
		 * NOTE: This object takes ownership of the vector.
		 * @param name Field name.
		 * @param bit_names Bit names.
		 * @param elemsPerRow Number of elements per row.
		 * @param bitfield Bitfield.
		 * @return Field index, or -1 on error.
		 */
		int addField_bitfield(const rp_char *name,
			const std::vector<rp_string> *bit_names,
			int elemsPerRow, uint32_t bitfield);

		/**
		 * Add ListData.
		 * NOTE: This object takes ownership of the two vectors.
		 * @param name Field name.
		 * @param headers Vector of column names.
		 * @param list_data ListData.
		 * @return Field index, or -1 on error.
		 */
		int addField_listData(const rp_char *name,
			const std::vector<rp_string> *headers,
			const std::vector<std::vector<rp_string> > *list_data);

		/**
		 * Add DateTime.
		 * @param name Field name.
		 * @param date_time Date/Time.
		 * @param flags Date/Time flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_dateTime(const rp_char *name, int64_t date_time, int flags = 0);

		/**
		 * Add age ratings.
		 * The array is copied into the RomFields struct.
		 * @param name Field name.
		 * @param age_ratings Pointer to age ratings array.
		 * @return Field index, or -1 on error.
		 */
		int addField_ageRatings(const rp_char *name, const age_ratings_t &age_ratings);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMFIELDS_HPP__ */
