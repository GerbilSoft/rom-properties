/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomFields.hpp: ROM fields class.                                        *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_ROMFIELDS_HPP__
#define __ROMPROPERTIES_LIBRPBASE_ROMFIELDS_HPP__

#include "common.h"

// C includes.
#include <stddef.h>	/* size_t */
#include <stdint.h>

// C includes. (C++ namespace)
#include <ctime>

// C++ includes.
#include <array>
#include <map>
#include <string>
#include <vector>

namespace LibRpTexture {
	class rp_image;
}

namespace LibRpBase {

// RFT_LISTDATA alignment macros.
// - # indicates number of columns.
// - Parameters are for columns 0, 1, 2, 3, etc.
// - This does NOT include checkboxes or icons.
#define TXA_D	(RomFields::TextAlign::TXA_DEFAULT)
#define TXA_L	(RomFields::TextAlign::TXA_LEFT)
#define TXA_C	(RomFields::TextAlign::TXA_CENTER)
#define TXA_R	(RomFields::TextAlign::TXA_RIGHT)
#define AFLD_ALIGN1(a)				((a)&3U)
#define AFLD_ALIGN2(a,b)			(AFLD_ALIGN1(a)|(((b)&3U)<<2U))
#define AFLD_ALIGN3(a,b,c)			(AFLD_ALIGN2(a,b)|(((c)&3U)<<4U))
#define AFLD_ALIGN4(a,b,c,d)			(AFLD_ALIGN3(a,b,c)|(((d)&3U)<<6U))
#define AFLD_ALIGN5(a,b,c,d,e)			(AFLD_ALIGN4(a,b,c,d)|(((e)&3U)<<8U))
#define AFLD_ALIGN6(a,b,c,d,e,f)		(AFLD_ALIGN5(a,b,c,d,e)|(((f)&3U)<<10U))
#define AFLD_ALIGN7(a,b,c,d,e,f,g)		(AFLD_ALIGN6(a,b,c,d,e,f)|(((g)&3U)<<12U))
#define AFLD_ALIGN8(a,b,c,d,e,f,g,h)		(AFLD_ALIGN7(a,b,c,d,e,f,g)|(((h)&3U)<<14U))
#define AFLD_ALIGN9(a,b,c,d,e,f,g,h,i)		(AFLD_ALIGN8(a,b,c,d,e,f,g,h)|(((i)&3U)<<16U))
#define AFLD_ALIGN10(a,b,c,d,e,f,g,h,i,j)	(AFLD_ALIGN9(a,b,c,d,e,f,g,h,i)|(((j)&3U)<<18U))

class RomFieldsPrivate;
class RomFields
{
	public:
		// ROM field types.
		enum RomFieldType : uint8_t {
			RFT_INVALID,		// Invalid. (skips the field)
			RFT_STRING,		// Basic string.
			RFT_BITFIELD,		// Bitfield.
			RFT_LISTDATA,		// ListData.
			RFT_DATETIME,		// Date/Time.
			RFT_AGE_RATINGS,	// Age ratings.
			RFT_DIMENSIONS,		// Image dimensions.
			RFT_STRING_MULTI,	// Multi-language string.
		};

		// String format flags. (RFT_STRING)
		enum StringFormat : unsigned int {
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

			// Trim spaces from the end of strings.
			STRF_TRIM_END	= (1 << 3),

			// Numeric formatting: Use lowercase letters for hexadecimal.
			STRF_HEX_LOWER	= (1 << 4),

			// Hexdump: No spaces.
			STRF_HEXDUMP_NO_SPACES	= (1 << 5),
		};

		// Display flags for RFT_LISTDATA.
		enum ListDataFlags : unsigned int {
			// Show the ListView on a separate row
			// from the description label.
			RFT_LISTDATA_SEPARATE_ROW = (1 << 0),

			// Enable checkboxes.
			// NOTE: Mutually exclusive with icons.
			RFT_LISTDATA_CHECKBOXES = (1 << 1),

			// Enable icons.
			// NOTE: Mutually exclusive with checkboxes.
			RFT_LISTDATA_ICONS	= (1 << 2),

			// String data is multi-lingual.
			// NOTE: This changes the structure of the
			// data field!
			RFT_LISTDATA_MULTI	= (1 << 3),
		};

		// Display flags for RFT_DATETIME.
		enum DateTimeFlags : unsigned int {
			// Show the date value.
			RFT_DATETIME_HAS_DATE = (1 << 0),

			// Show the time value.
			RFT_DATETIME_HAS_TIME = (1 << 1),

			// Date does not have a valid year value.
			RFT_DATETIME_NO_YEAR = (1 << 2),

			// Mask for date/time display values.
			RFT_DATETIME_HAS_DATETIME_MASK = RFT_DATETIME_HAS_DATE | RFT_DATETIME_HAS_TIME,
			RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK = RFT_DATETIME_HAS_DATE | RFT_DATETIME_HAS_TIME | RFT_DATETIME_NO_YEAR,

			// Show the timestamp as UTC instead of the local timezone.
			// This is useful for timestamps that aren't actually
			// adjusted for the local timezone.
			RFT_DATETIME_IS_UTC = (1 << 3),
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
			AGE_AUSTRALIA	= 8,	// Australia (ACB)
			AGE_SOUTH_KOREA	= 9,	// South Korea (GRB, formerly KMRB)
			AGE_TAIWAN	= 10,	// Taiwan (CGSRR)

			AGE_MAX		= 16	// Maximum number of age rating fields
		};

		// Age Ratings bitfields.
		enum AgeRatingsBitfield : uint16_t {
			AGEBF_MIN_AGE_MASK	= 0x001F,	// Low 5 bits indicate the minimum age.
			AGEBF_ACTIVE		= 0x0020,	// Rating is only valid if this is set.
			AGEBF_PENDING		= 0x0040,	// Rating is pending.
			AGEBF_NO_RESTRICTION	= 0x0080,	// No age restriction.
			AGEBF_ONLINE_PLAY	= 0x0100,	// Rating may change due to online play.
			AGEBF_PROHIBITED	= 0x0200,	// Game is specifically prohibited.
		};

		// Age Ratings type.
		typedef std::array<uint16_t, AGE_MAX> age_ratings_t;

		// Text alignment for RFT_LISTDATA.
		enum TextAlign : uint32_t {
			TXA_DEFAULT	= 0,	// OS default
			TXA_LEFT	= 1,
			TXA_CENTER	= 2,
			TXA_RIGHT	= 3,
		};

		// Typedefs for various containers.
		typedef std::map<uint32_t, std::string> StringMultiMap_t;
		typedef std::vector<std::vector<std::string> > ListData_t;
		typedef std::map<uint32_t, ListData_t> ListDataMultiMap_t;
		typedef std::vector<const LibRpTexture::rp_image*> ListDataIcons_t;

		// ROM field struct.
		// Dynamically allocated.
		struct Field {
			std::string name;	// Field name.
			RomFieldType type;	// ROM field type.
			uint8_t tabIdx;		// Tab index. (0 for default)
			bool isValid;		// True if this field has valid data.

			// Field description.
			union _desc {
				unsigned int flags;	// Generic flags. (string, date)

				// TODO: Reorder to reduce wasted space on 64-bit?
				struct _bitfield {
					// Bit flags per row. (3 or 4 is usually good)
					int elemsPerRow;
					// Bit flag names.
					// Must be a vector of at least 'elements' strings.
					// If a name is nullptr, that element is skipped.
					const std::vector<std::string> *names;
				} bitfield;
				struct _list_data {
					// Flags.
					unsigned int flags;

					// Number of visible rows. (0 for "default")
					int rows_visible;

					// List field names. (headers)
					// Must be a vector of at least 'fields' strings.
					// If a name is nullptr, that field is skipped.
					const std::vector<std::string> *names;

					// Column text alignment.
					// Up to 16 columns can be specified using
					// two bits each, with the two LSBs indicating
					// column 0, next two bits column 1, etc.
					// See the TextAlign enum.
					struct {
						// TODO: Reduce to uint16_t?
						uint32_t headers;	// Header alignment
						uint32_t data;		// Data alignment
					} alignment;
				} list_data;
			} desc;

			// Field data.
			union _data {
				// Generic data for NULL.
				uint64_t generic;

				// RFT_STRING
				const std::string *str;

				// RFT_BITFIELD
				uint32_t bitfield;

				// RFT_LISTDATA
				struct {
					union {
						// Standard RFT_LISTDATA
						const ListData_t *single;

						// RFT_LISTDATA_MULTI
						// - Key: Language code
						// - Value: Vector of rows.
						const ListDataMultiMap_t *multi;
					} data;

					union {
						// Checkbox bitfield.
						// Requires RFT_LISTDATA_CHECKBOXES.
						uint32_t checkboxes;

						// Icons vector.
						// Requires RFT_LISTDATA_ICONS.
						const ListDataIcons_t *icons;
					} mxd;
				} list_data;

				// RFT_DATETIME (UNIX format)
				// NOTE: -1 is used to indicate
				// an invalid date/time.
				time_t date_time;

				// RFT_AGE_RATINGS
				// See AgeRatingsCountry for field indexes.
				const age_ratings_t *age_ratings;

				// RFT_DIMENSIONS
				// Up to three image dimensions.
				// If a dimension is not present, set to 0.
				int dimensions[3];

				// RFT_STRING_MULTI
				// - Key: Language code ('en', 'es', etc; multi-char constant)
				// - Value: String
				const StringMultiMap_t *str_multi;
			} data;
		};

	public:
		/**
		 * Initialize a ROM Fields class.
		 */
		RomFields();
		~RomFields();

	private:
		RP_DISABLE_COPY(RomFields)
	private:
		friend class RomFieldsPrivate;
		RomFieldsPrivate *const d_ptr;

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
		 * Is this RomFields empty?
		 * @return True if empty; false if not.
		 */
		bool empty(void) const;

	public:
		/**
		 * Get the abbreviation of an age rating organization.
		 * (TODO: Full name function?)
		 * @param country Rating country. (See AgeRatingsCountry.)
		 * @return Abbreviation, or nullptr if invalid.
		 */
		static const char *ageRatingAbbrev(int country);

		/**
		 * Decode an age rating into a human-readable string.
		 * This does not include the name of the rating organization.
		 *
		 * @param country Rating country. (See AgeRatingsCountry.)
		 * @param rating Rating value.
		 * @return Human-readable string, or empty string if the rating isn't active.
		 */
		static std::string ageRatingDecode(int country, uint16_t rating);

		/**
		 * Decode all age ratings into a human-readable string.
		 * This includes the names of the rating organizations.
		 * @param age_ratings Age ratings.
		 * @param newlines If true, print newlines after every four ratings.
		 * @return Human-readable string, or empty string if no ratings.
		 */
		static std::string ageRatingsDecode(const age_ratings_t *age_ratings, bool newlines = true);

	public:
		/** Convenience functions for RomData subclasses. **/

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
		void setTabName(int tabIdx, const char *name);

		/**
		 * Add a tab to the end and select it.
		 * @param name Tab name.
		 * @return Tab index.
		 */
		int addTab(const char *name);

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
		const char *tabName(int tabIdx) const;

		/**
		 * Get the default language code for RFT_STRING_MULTI and RFT_LISTDATA_MULTI.
		 * @return Default language code, or 0 if not set.
		 */
		uint32_t defaultLanguageCode(void) const;

		/** Fields **/

		/**
		 * Reserve space for fields.
		 * @param n Desired capacity.
		 */
		void reserve(int n);

		/**
		 * Convert an array of char strings to a vector of std::string.
		 * This can be used for addField_bitfield() and addField_listData().
		 * @param strArray Array of strings.
		 * @param count Number of strings, or -1 for a NULL-terminated array.
		 * NOTE: The array will be terminated at NULL regardless of count,
		 * so a -1 count is only useful if the size isn't known.
		 * @return Allocated std::vector<std::string>.
		 */
		static std::vector<std::string> *strArrayToVector(const char *const *strArray, int count = -1);

		/**
		 * Convert an array of char strings to a vector of std::string.
		 * This can be used for addField_bitfield() and addField_listData().
		 * @param msgctxt i18n context.
		 * @param strArray Array of strings.
		 * @param count Number of strings, or -1 for a NULL-terminated array.
		 * NOTE: The array will be terminated at NULL regardless of count,
		 * so a -1 count is only useful if the size isn't known.
		 * @return Allocated std::vector<std::string>.
		 */
		static std::vector<std::string> *strArrayToVector_i18n(const char *msgctxt, const char *const *strArray, int count = -1);

		enum {
			TabOffset_Ignore = -1,
			TabOffset_AddTabs = -2,
		};

		/**
		 * Add fields from another RomFields object.
		 * @param other Source RomFields object.
		 * @param tabOffset Tab index to add to the original tabs.
		 *
		 * Special tabOffset values:
		 * - -1: Ignore the original tab indexes.
		 * - -2: Add tabs from the original RomFields.
		 *
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
		int addField_string(const char *name, const char *str, unsigned int flags = 0);

		/**
		 * Add string field data.
		 * @param name Field name.
		 * @param str String.
		 * @param flags Formatting flags.
		 * @return Field index.
		 */
		int addField_string(const char *name, const std::string &str, unsigned int flags = 0);

		enum Base {
			FB_DEC,
			FB_HEX,
			FB_OCT,
		};

		/**
		 * Add string field data using a numeric value.
		 * @param name Field name.
		 * @param val Numeric value.
		 * @param base Base. If not decimal, a prefix will be added.
		 * @param digits Number of leading digits. (0 for none)
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_string_numeric(const char *name, uint32_t val, Base base = FB_DEC, int digits = 0, unsigned int flags = 0);

		/**
		 * Add a string field formatted like a hex dump
		 * @param name Field name.
		 * @param buf Input bytes.
		 * @param size Byte count.
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_string_hexdump(const char *name, const uint8_t *buf, size_t size, unsigned int flags = 0);

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
		int addField_string_address_range(const char *name,
			uint32_t start, uint32_t end,
			const char *suffix, int digits = 8, unsigned int flags = 0);

		/**
		 * Add a string field formatted for an address range.
		 * @param name Field name.
		 * @param start Start address.
		 * @param end End address.
		 * @param digits Number of leading digits. (default is 8 for 32-bit)
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		inline int addField_string_address_range(const char *name,
			uint32_t start, uint32_t end, int digits = 8, unsigned int flags = 0)
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
		int addField_bitfield(const char *name,
			const std::vector<std::string> *bit_names,
			int elemsPerRow, uint32_t bitfield);

		/**
		 * addField_listData() parameter struct.
		 */
		struct AFLD_PARAMS {
			AFLD_PARAMS()
				: flags(0), rows_visible(0)
				, def_lc(0), headers(nullptr)
			{
				alignment.headers = 0;
				alignment.data = 0;
				data.single = nullptr;
				mxd.icons = nullptr;
			}
			AFLD_PARAMS(unsigned int flags, int rows_visible)
				: flags(flags), rows_visible(rows_visible)
				, def_lc(0), headers(nullptr)
			{
				alignment.headers = 0;
				alignment.data = 0;
				data.single = nullptr;
				mxd.icons = nullptr;
			}

			// Formatting
			unsigned int flags;
			int rows_visible;

			// Column text alignment.
			// Up to 16 columns can be specified using
			// two bits each, with the two LSBs indicating
			// column 0, next two bits column 1, etc.
			// See the TextAlign enum.
			struct {
				// TODO: Reduce to uint16_t?
				uint32_t headers;	// Header alignment
				uint32_t data;		// Data alignment
			} alignment;

			// Default language code. (RFT_LISTDATA_MULTI)
			uint32_t def_lc;

			// Data
			const std::vector<std::string> *headers;
			union {
				const ListData_t *single;
				const ListDataMultiMap_t *multi;
			} data;

			// Mutually-exclusive data.
			union {
				// Checkbox bitfield.
				// Requires RFT_LISTDATA_CHECKBOXES.
				uint32_t checkboxes;

				// Icons vector.
				// Requires RFT_LISTDATA_ICONS.
				const std::vector<const LibRpTexture::rp_image*> *icons;
			} mxd;
		};

		/**
		 * Add ListData.
		 * NOTE: This object takes ownership of the vectors.
		 * @param name Field name.
		 * @param params Parameters.
		 *
		 * NOTE: If headers is nullptr, the column count will be
		 * determined using the first row in list_data.
		 *
		 * @return Field index, or -1 on error.
		 */
		int addField_listData(const char *name, const AFLD_PARAMS *params);

		/**
		 * Add DateTime.
		 * @param name Field name.
		 * @param date_time Date/Time.
		 * @param flags Date/Time flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_dateTime(const char *name, time_t date_time, unsigned int flags = 0);

		/**
		 * Add age ratings.
		 * The array is copied into the RomFields struct.
		 * @param name Field name.
		 * @param age_ratings Pointer to age ratings array.
		 * @return Field index, or -1 on error.
		 */
		int addField_ageRatings(const char *name, const age_ratings_t &age_ratings);

		/**
		 * Add image dimensions.
		 * @param name Field name.
		 * @param dimX X dimension.
		 * @param dimY Y dimension.
		 * @param dimZ Z dimension.
		 * @return Field index, or -1 on error.
		 */
		int addField_dimensions(const char *name, int dimX, int dimY = 0, int dimZ = 0);

		/**
		 * Add a multi-language string.
		 * NOTE: This object takes ownership of the map.
		 * @param name Field name.
		 * @param str_multi Map of strings with language codes.
		 * @param def_lc Default language code.
		 * @param flags Formatting flags.
		 * @return Field index, or -1 on error.
		 */
		int addField_string_multi(const char *name, const StringMultiMap_t *str_multi,
			uint32_t def_lc = 'en', unsigned int flags = 0);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_ROMFIELDS_HPP__ */
