/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomFields.cpp: ROM fields class.                                        *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "RomFields.hpp"

#include "common.h"
#include "TextFuncs.hpp"
#include "threads/Atomics.h"
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <limits>
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRpBase {

class RomFieldsPrivate
{
	public:
		RomFieldsPrivate();
	private:
		~RomFieldsPrivate();	// call unref() instead

	private:
		RP_DISABLE_COPY(RomFieldsPrivate)

	public:
		/** Reference count functions. **/

		/**
		 * Create a reference of this object.
		 * @return this
		 */
		RomFieldsPrivate *ref(void);

		/**
		 * Unreference this object.
		 */
		void unref(void);

		/**
		 * Is this object currently shared?
		 * @return True if ref_cnt > 1; false if not.
		 */
		inline bool isShared(void) const;

	private:
		// Current reference count.
		volatile int ref_cnt;

	public:
		// ROM field structs.
		vector<RomFields::Field> fields;

		// Current tab index.
		uint8_t tabIdx;
		// Tab names.
		vector<string> tabNames;

		/**
		 * Deletes allocated strings in this->data.
		 */
		void delete_data(void);

		/**
		 * Remove trailing spaces from a string.
		 * Used for STRF_TRIM_END.
		 * @param str String.
		 */
		void trimEnd(string &str);
};

/** RomFieldsPrivate **/

RomFieldsPrivate::RomFieldsPrivate()
	: ref_cnt(1)
	, tabIdx(0)
{ }

RomFieldsPrivate::~RomFieldsPrivate()
{
	delete_data();
}

/**
 * Create a reference of this object.
 * @return this
 */
RomFieldsPrivate *RomFieldsPrivate::ref(void)
{
	ATOMIC_INC_FETCH(&ref_cnt);
	return this;
}

/**
 * Unreference this object.
 */
void RomFieldsPrivate::unref(void)
{
	assert(ref_cnt > 0);
	if (ATOMIC_DEC_FETCH(&ref_cnt) <= 0) {
		// All references removed.
		delete this;
	}
}

/**
 * Is this object currently shared?
 * @return True if ref_cnt > 1; false if not.
 */
inline bool RomFieldsPrivate::isShared(void) const
{
	assert(ref_cnt > 0);
	return (ref_cnt > 1);
}

/**
 * Deletes allocated strings in this->data.
 */
void RomFieldsPrivate::delete_data(void)
{
	// Delete all of the allocated strings in this->fields.
	for (int i = (int)(fields.size() - 1); i >= 0; i--) {
		RomFields::Field &field = this->fields.at(i);
		if (!field.isValid) {
			// No data here.
			continue;
		}

		switch (field.type) {
			case RomFields::RFT_INVALID:
			case RomFields::RFT_DATETIME:
				// No data here.
				break;

			case RomFields::RFT_STRING:
				delete const_cast<string*>(field.data.str);
				break;
			case RomFields::RFT_BITFIELD:
				delete const_cast<vector<string>*>(field.desc.bitfield.names);
				break;
			case RomFields::RFT_LISTDATA:
				delete const_cast<vector<string>*>(field.desc.list_data.names);
				delete const_cast<vector<vector<string> >*>(field.data.list_data);
				break;
			case RomFields::RFT_AGE_RATINGS:
				delete const_cast<RomFields::age_ratings_t*>(field.data.age_ratings);
				break;
			default:
				// ERROR!
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;
		}
	}

	// Clear the fields vector.
	this->fields.clear();
}

/**
 * Remove trailing spaces from a string.
 * Used for STRF_TRIM_END.
 * @param str String.
 */
void RomFieldsPrivate::trimEnd(string &str)
{
	// NOTE: No str.empty() check because that's usually never the case here.
	// TODO: Check for U+3000? (UTF-8: "\xE3\x80\x80")
	// TODO: Move to TextFuncs and add unit tests?
	size_t sz = str.size();
	const char *p_start = str.c_str();
	for (const char *p = p_start + sz - 1; p >= p_start; p--) {
		if (*p != ' ')
			break;
		sz--;
	}
	str.resize(sz);
}

/** RomFields **/

/**
 * Initialize a ROM Fields class.
 * @param fields Array of fields.
 * @param count Number of fields.
 */
RomFields::RomFields()
	: d_ptr(new RomFieldsPrivate())
{ }

RomFields::~RomFields()
{
	d_ptr->unref();
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RomFields::RomFields(const RomFields &other)
	: d_ptr(other.d_ptr->ref())
{ }

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RomFields &RomFields::operator=(const RomFields &other)
{
	RomFieldsPrivate *const d_old = this->d_ptr;
	this->d_ptr = other.d_ptr->ref();
	d_old->unref();
	return *this;
}

/**
 * Detach this instance from all other instances.
 * TODO: Move to RomFieldsPrivate?
 */
void RomFields::detach(void)
{
	if (!d_ptr->isShared()) {
		// Only one reference.
		// Nothing to detach from.
		return;
	}

	// Need to detach.
	RomFieldsPrivate *const d_new = new RomFieldsPrivate();
	RomFieldsPrivate *const d_old = d_ptr;
	d_new->fields.resize(d_old->fields.size());
	for (int i = (int)(d_old->fields.size() - 1); i >= 0; i--) {
		const Field &field_old = d_old->fields.at(i);
		Field &field_new = d_new->fields.at(i);
		field_new.name = field_old.name;
		field_new.type = field_old.type;
		field_new.isValid = field_old.isValid;
		if (!field_old.isValid) {
			// No data here.
			field_new.desc.flags = 0;
			field_new.data.generic = 0;
			continue;
		}

		switch (field_old.type) {
			case RFT_INVALID:
				// No data here
				field_new.isValid = false;
				field_new.desc.flags = 0;
				field_new.data.generic = 0;
				break;
			case RFT_STRING:
				field_new.desc.flags = field_old.desc.flags;
				if (field_old.data.str) {
					field_new.data.str = new string(*field_old.data.str);
				} else {
					field_new.data.str = nullptr;
				}
				break;
			case RFT_BITFIELD:
				field_new.desc.bitfield.elements = field_old.desc.bitfield.elements;
				field_new.desc.bitfield.elemsPerRow = field_old.desc.bitfield.elemsPerRow;
				if (field_old.desc.bitfield.names) {
					field_new.desc.bitfield.names = new vector<string>(*field_old.desc.bitfield.names);
				} else {
					field_new.desc.bitfield.names = nullptr;
				}
				field_new.data.bitfield = field_old.data.bitfield;
				break;
			case RFT_LISTDATA:
				// Copy the ListData.
				field_new.desc.list_data.flags =
					field_old.desc.list_data.flags;
				field_new.desc.list_data.rows_visible =
					field_old.desc.list_data.rows_visible;
				if (field_old.desc.list_data.names) {
					field_new.desc.list_data.names = new vector<string>(*field_old.desc.list_data.names);
				} else {
					field_new.desc.list_data.names = nullptr;
				}
				if (field_old.data.list_data) {
					field_new.data.list_data = new vector<vector<string> >(*field_old.data.list_data);
				} else {
					field_new.data.list_data = nullptr;
				}
				field_new.data.list_checkboxes =
					field_old.data.list_checkboxes;
				break;
			case RFT_DATETIME:
				field_new.desc.flags = field_old.desc.flags;
				field_new.data.date_time = field_old.data.date_time;
				break;
			case RFT_AGE_RATINGS:
				field_new.data.age_ratings = field_old.data.age_ratings;
				break;
			default:
				// ERROR!
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;
		}
	}

	// Detached.
	d_ptr = d_new;
	d_old->unref();
}

/**
 * Get the abbreviation of an age rating organization.
 * (TODO: Full name function?)
 * @param country Rating country. (See AgeRatingCountry.)
 * @return Abbreviation, or nullptr if invalid.
 */
const char *RomFields::ageRatingAbbrev(int country)
{
	static const char abbrevs[16][8] = {
		"CERO", "ESRB", "",        "USK",
		"PEGI", "MEKU", "PEGI-PT", "BBFC",
		"AGCB", "GRB",  "CGSRR",   "",
		"",     "",     "",        "",
	};

	assert(country >= 0 && country < ARRAY_SIZE(abbrevs));
	if (country < 0 || country >= ARRAY_SIZE(abbrevs)) {
		// Index is out of range.
		return nullptr;
	}

	if (abbrevs[country][0] != 0) {
		return abbrevs[country];
	}
	// Invalid country code.
	return nullptr;
}

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
string RomFields::ageRatingDecode(int country, uint16_t rating)
{
	if (!(rating & AGEBF_ACTIVE)) {
		// Rating isn't active.
		return string();
	}

	// Check for special statuses.
	const char *s_rating = nullptr;
	if (rating & RomFields::AGEBF_PROHIBITED) {
		// TODO: Better description?
		// tr: Prohibited.
		s_rating = C_("RomFields|AgeRating", "No");
	} else if (rating & RomFields::AGEBF_PENDING) {
		// Rating is pending.
		s_rating = "RP";
	} else if (rating & RomFields::AGEBF_NO_RESTRICTION) {
		// tr: No age restriction.
		s_rating = C_("RomFields|AgeRating", "All");
	} else {
		// Use the age rating.
		// TODO: Verify these.
		// TODO: Check for <= instead of exact matches?
		switch (country) {
			case AGE_JAPAN:
				switch (rating & RomFields::AGEBF_MIN_AGE_MASK) {
					case 0:
						s_rating = "A";
						break;
					case 12:
						s_rating = "B";
						break;
					case 15:
						s_rating = "C";
						break;
					case 17:
						s_rating = "D";
						break;
					case 18:
						s_rating = "Z";
						break;
					default:
						// Unknown rating.
						break;
				}
				break;

			case AGE_USA:
				switch (rating & RomFields::AGEBF_MIN_AGE_MASK) {
					case 3:
						s_rating = "eC";
						break;
					case 6:
						s_rating = "E";
						break;
					case 10:
						s_rating = "E10+";
						break;
					case 13:
						s_rating = "T";
						break;
					case 17:
						s_rating = "M";
						break;
					case 18:
						s_rating = "AO";
						break;
					default:
						// Unknown rating.
						break;
				}
				break;

			case AGE_AUSTRALIA:
				switch (rating & RomFields::AGEBF_MIN_AGE_MASK) {
					case 0:
						s_rating = "G";
						break;
					case 7:
						s_rating = "PG";
						break;
					case 14:
						s_rating = "M";
						break;
					case 15:
						s_rating = "MA15+";
						break;
					case 18:
						s_rating = "R18+";
						break;
					default:
						// Unknown rating.
						break;
				}
				break;

			default:
				// No special handling for this country.
				break;
		}
	}

	string str;
	str.reserve(8);
	if (s_rating) {
		str = s_rating;
	} else {
		// No string rating.
		// Print the numeric value.
		str = rp_sprintf("%u", rating & RomFields::AGEBF_MIN_AGE_MASK);
	}

	if (rating & RomFields::AGEBF_ONLINE_PLAY) {
		// Rating may change during online play.
		// TODO: Add a description of this somewhere.
		// Unicode U+00B0, encoded as UTF-8.
		str += "\xC2\xB0";
	}

	return str;
}

/**
 * Decode all age ratings into a human-readable string.
 * This includes the names of the rating organizations.
 * @param age_ratings Age ratings.
 * @param newlines If true, print newlines after every four ratings.
 * @return Human-readable string, or empty string if no ratings.
 */
string RomFields::ageRatingsDecode(const age_ratings_t *age_ratings, bool newlines)
{
	assert(age_ratings != nullptr);
	if (!age_ratings)
		return string();

	// Convert the age ratings field to a string.
	string str;
	str.reserve(64);
	unsigned int ratings_count = 0;
	for (int i = 0; i < (int)age_ratings->size(); i++) {
		const uint16_t rating = age_ratings->at(i);
		if (!(rating & RomFields::AGEBF_ACTIVE))
			continue;

		if (ratings_count > 0) {
			// Append a comma.
			if (newlines && ratings_count % 4 == 0) {
				// 4 ratings per line.
				str += ",\n";
			} else {
				str += ", ";
			}
		}

		const char *const abbrev = RomFields::ageRatingAbbrev(i);
		if (abbrev) {
			str += abbrev;
		} else {
			// Invalid age rating organization.
			// Use the numeric index.
			str += rp_sprintf("%d", i);
		}
		str += '=';
		str += ageRatingDecode(i, rating);
		ratings_count++;
	}

	if (ratings_count == 0) {
		// tr: No age ratings.
		str = C_("RomFields|AgeRating", "None");
	}

	return str;
}

/** Field accessors. **/

/**
 * Get the number of fields.
 * @return Number of fields.
 */
int RomFields::count(void) const
{
	RP_D(const RomFields);
	return (int)d->fields.size();
}

/**
 * Get a ROM field.
 * @param idx Field index.
 * @return ROM field, or nullptr if the index is invalid.
 */
const RomFields::Field *RomFields::field(int idx) const
{
	RP_D(const RomFields);
	if (idx < 0 || idx >= (int)d->fields.size())
		return nullptr;
	return &d->fields[idx];
}

/**
 * Is data loaded?
 * TODO: Rename to empty() after porting to the new addField() functions.
 * @return True if m_data has at least one row; false if m_data is nullptr or empty.
 */
bool RomFields::isDataLoaded(void) const
{
	RP_D(const RomFields);
	return !d->fields.empty();
}

/** Convenience functions for RomData subclasses. **/

/** Tabs **/

/**
 * Reserve space for tabs.
 * @param n Desired tab count.
 */
void RomFields::reserveTabs(int n)
{
	assert(n > 0);
	if (n > 0) {
		RP_D(RomFields);
		d->fields.reserve(n);
	}
}

/**
 * Set the tab index for new fields.
 * @param idx Tab index.
 */
void RomFields::setTabIndex(int tabIdx)
{
	RP_D(RomFields);
	d->tabIdx = tabIdx;
	if ((int)d->tabNames.size() < tabIdx+1) {
		// Need to resize tabNames.
		d->tabNames.resize(tabIdx+1);
	}
}

/**
 * Set a tab name.
 * NOTE: An empty tab name will hide the tab.
 * @param tabIdx Tab index.
 * @param name Tab name.
 */
void RomFields::setTabName(int tabIdx, const char *name)
{
	assert(tabIdx >= 0);
	if (tabIdx < 0)
		return;

	RP_D(RomFields);
	if ((int)d->tabNames.size() < tabIdx+1) {
		// Need to resize tabNames.
		d->tabNames.resize(tabIdx+1);
	}
	d->tabNames[tabIdx] = (name ? name : "");
}

/**
 * Add a tab to the end and select it.
 * @param name Tab name.
 * @return Tab index.
 */
int RomFields::addTab(const char *name)
{
	RP_D(RomFields);
	d->tabNames.push_back(name);
	d->tabIdx = (int)d->tabNames.size() - 1;
	return d->tabIdx;
}

/**
 * Get the tab count.
 * @return Tab count. (highest tab index, plus 1)
 */
int RomFields::tabCount(void) const
{
	// NOTE: d->tabNames might be empty if
	// only a single tab is in use and no
	// tab name has been set.
	RP_D(const RomFields);
	int ret = (int)d->tabNames.size();
	return (ret > 0 ? ret : 1);
}

/**
 * Get the name of the specified tab.
 * @param tabIdx Tab index.
 * @return Tab name, or nullptr if no name is set.
 */
const char *RomFields::tabName(int tabIdx) const
{
	assert(tabIdx >= 0);
	if (tabIdx < 0)
		return nullptr;

	RP_D(const RomFields);
	if (tabIdx >= (int)d->tabNames.size()) {
		// No tab name.
		return nullptr;
	}

	// NOTE: nullptr is returned if the name is empty.
	if (d->tabNames[tabIdx].empty())
		return nullptr;
	return d->tabNames[tabIdx].c_str();
}

/** Fields **/

/**
 * Reserve space for fields.
 * @param n Desired capacity.
 */
void RomFields::reserve(int n)
{
	assert(n > 0);
	if (n > 0) {
		RP_D(RomFields);
		d->fields.reserve(n);
	}
}

/**
 * Convert an array of char strings to a vector of std::string.
 * This can be used for addField_bitfield() and addField_listData().
 * @param strArray Array of strings.
 * @param count Number of strings, or -1 for a NULL-terminated array.
 * NOTE: The array will be terminated at NULL regardless of count,
 * so a -1 count is only useful if the size isn't known.
 * @return Allocated std::vector<std::string>.
 */
vector<string> *RomFields::strArrayToVector(const char *const *strArray, int count)
{
	vector<string> *pVec = new vector<string>();
	if (count < 0) {
		count = std::numeric_limits<int>::max();
	} else {
		pVec->reserve(count);
	}

	for (; strArray != nullptr && count > 0; strArray++, count--) {
		// nullptr will be handled as empty strings.
		pVec->push_back(*strArray ? *strArray : "");
	}

	return pVec;
}

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
vector<string> *RomFields::strArrayToVector_i18n(const char *msgctxt, const char *const *strArray, int count)
{
#ifndef ENABLE_NLS
	// Mark msgctxt as unused here.
	RP_UNUSED(msgctxt);
#endif /* ENABLE_NLS */

	vector<string> *pVec = new vector<string>();
	if (count < 0) {
		count = std::numeric_limits<int>::max();
	} else {
		pVec->reserve(count);
	}

	for (; strArray != nullptr && count > 0; strArray++, count--) {
		// nullptr will be handled as empty strings.
		if (*strArray) {
			pVec->push_back(dpgettext_expr(RP_I18N_DOMAIN, msgctxt, *strArray));
		} else {
			pVec->push_back(string());
		}
	}

	return pVec;
}

/**
 * Add fields from another RomFields object.
 * @param other Source RomFields object.
 * @param tabOffset Tab index to add to the original tabs. (If -1, ignore the original tabs.)
 * @return Field index of the last field added.
 */
int RomFields::addFields_romFields(const RomFields *other, int tabOffset)
{
	RP_D(RomFields);

	assert(other != nullptr);
	if (!other)
		return -1;

	// TODO: More tab options:
	// - Add original tab names if present.
	// - Add all to specified tab or to current tab.
	// - Use absolute or relative tab offset.
	d->fields.reserve(d->fields.size() + other->count());

	for (int i = 0; i < other->count(); i++) {
		const Field *src = other->field(i);
		if (!src)
			continue;

		int idx = (int)d->fields.size();
		d->fields.resize(idx+1);
		Field &field = d->fields.at(idx);
		field.name = src->name;
		field.type = src->type;
		field.tabIdx = (tabOffset != -1 ? (src->tabIdx + tabOffset) : d->tabIdx);
		field.isValid = src->isValid;
		field.desc.flags = src->desc.flags;

		switch (src->type) {
			case RFT_INVALID:
				// No data here...
				break;

			case RFT_STRING:
				field.data.str = (src->data.str ? new string(*src->data.str) : nullptr);
				break;
			case RFT_BITFIELD:
				field.desc.bitfield.elements = src->desc.bitfield.elements;
				field.desc.bitfield.elemsPerRow = src->desc.bitfield.elemsPerRow;
				field.desc.bitfield.names = (src->desc.bitfield.names
						? new vector<string>(*(src->desc.bitfield.names))
						: nullptr);
				field.data.bitfield = src->data.bitfield;
				break;
			case RFT_LISTDATA:
				field.desc.list_data.flags =
					src->desc.list_data.flags;
				field.desc.list_data.rows_visible =
					src->desc.list_data.rows_visible;
				field.desc.list_data.names = (src->desc.list_data.names
						? new vector<string>(*(src->desc.list_data.names))
						: nullptr);
				field.data.list_data = (src->data.list_data
						? new vector<vector<string> >(*(src->data.list_data))
						: nullptr);
				field.data.list_checkboxes =
					src->data.list_checkboxes;
				break;
			case RFT_DATETIME:
				field.data.date_time = src->data.date_time;
				break;
			case RFT_AGE_RATINGS:
				field.data.age_ratings = (src->data.age_ratings
						? new age_ratings_t(*src->data.age_ratings)
						: nullptr);
				break;

			default:
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;
		}
	}

	// Fields added.
	return (int)d->fields.size()-1;
}

/**
 * Add string field data.
 * @param name Field name.
 * @param str String.
 * @param flags Formatting flags.
 * @return Field index.
 */
int RomFields::addField_string(const char *name, const char *str, unsigned int flags)
{
	assert(name != nullptr);
	if (!name)
		return -1;

	// RFT_STRING
	RP_D(RomFields);
	int idx = (int)d->fields.size();
	d->fields.resize(idx+1);
	Field &field = d->fields.at(idx);

	string *const nstr = (str ? new string(str) : nullptr);
	field.name = (name ? name : "");
	field.type = RFT_STRING;
	field.desc.flags = flags;
	field.data.str = nstr;
	field.tabIdx = d->tabIdx;
	field.isValid = (name != nullptr);

	// Handle string trimming flags.
	if (field.data.str && (flags & STRF_TRIM_END)) {
		d->trimEnd(*nstr);
	}
	return idx;
}

/**
 * Add string field data.
 * @param name Field name.
 * @param str String.
 * @param flags Formatting flags.
 * @return Field index.
 */
int RomFields::addField_string(const char *name, const string &str, unsigned int flags)
{
	assert(name != nullptr);
	if (!name)
		return -1;

	// RFT_STRING
	RP_D(RomFields);
	int idx = (int)d->fields.size();
	d->fields.resize(idx+1);
	Field &field = d->fields.at(idx);

	string *const nstr = (!str.empty() ? new string(str) : nullptr);
	field.name = (name ? name : "");
	field.type = RFT_STRING;
	field.desc.flags = flags;
	field.data.str = nstr;
	field.tabIdx = d->tabIdx;
	field.isValid = true;

	// Handle string trimming flags.
	if (nstr && (flags & STRF_TRIM_END)) {
		d->trimEnd(*nstr);
	}
	return idx;
}

/**
 * Add string field data using a numeric value.
 * @param name Field name.
 * @param val Numeric value.
 * @param base Base. If not decimal, a prefix will be added.
 * @param digits Number of leading digits. (0 for none)
 * @param flags Formatting flags.
 * @return Field index, or -1 on error.
 */
int RomFields::addField_string_numeric(const char *name, uint32_t val, Base base, int digits, unsigned int flags)
{
	assert(name != nullptr);
	if (!name)
		return -1;

	const char *fmtstr;
	switch (base) {
		case FB_DEC:
		default:
			fmtstr = "%0*u";
			break;
		case FB_HEX:
			fmtstr = (!(flags & STRF_HEX_LOWER)) ? "0x%0*X" : "0x%0*x";
			break;
		case FB_OCT:
			fmtstr = "0%0*o";
			break;
	}

	const string str = rp_sprintf(fmtstr, digits, val);
	return addField_string(name, str, flags);
}

/**
 * Add a string field formatted like a hex dump
 * @param name Field name.
 * @param buf Input bytes.
 * @param size Byte count.
 * @param flags Formatting flags.
 * @return Field index, or -1 on error.
 */
int RomFields::addField_string_hexdump(const char *name, const uint8_t *buf, size_t size, unsigned int flags)
{
	assert(name != nullptr);
	if (!name)
		return -1;

	if (size == 0) {
		return addField_string(name, nullptr);
	}

	// Reserve 3 characters per byte.
	// (Two hex digits, plus one space.)
	unique_ptr<char[]> str(new char[size*3]);
	char *pStr = str.get();

	// Hexadecimal lookup table.
	static const char hex_lookup[2][16] = {
		// Uppercase
		{'0','1','2','3','4','5','6','7',
		 '8','9','A','B','C','D','E','F'},

		// Lowercase
		{'0','1','2','3','4','5','6','7',
		 '8','9','a','b','c','d','e','f'},
	};
	const char *const tbl = (!(flags & STRF_HEX_LOWER) ? hex_lookup[0] : hex_lookup[1]);

	// Print the hexdump.
	if (!(flags & STRF_HEXDUMP_NO_SPACES)) {
		// Spaces.
		for (; size > 0; size--, buf++, pStr += 3) {
			pStr[0] = tbl[*buf >> 4];
			pStr[1] = tbl[*buf & 0x0F];
			pStr[2] = ' ';
		}
		// Remove the trailing space.
		*(pStr-1) = 0;
	} else {
		// No spaces.
		for (; size > 0; size--, buf++, pStr += 2) {
			pStr[0] = tbl[*buf >> 4];
			pStr[1] = tbl[*buf & 0x0F];
		}
		// NULL-terminate the string.
		*pStr = 0;
	}

	return addField_string(name, str.get(), flags);
}

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
int RomFields::addField_string_address_range(const char *name,
	uint32_t start, uint32_t end,
	const char *suffix, int digits, unsigned int flags)
{
	assert(name != nullptr);
	if (!name)
		return -1;

	// Maximum number of digits is 16. (64-bit)
	assert(digits <= 16);
	if (digits > 16) {
		digits = 16;
	}

	// Address range.
	string str = rp_sprintf(
		(!(flags & STRF_HEX_LOWER)) ? "0x%0*X - 0x%0*X" : "0x%0*x - 0x%0*x",
		digits, start, digits, end);
	if (suffix && suffix[0] != 0) {
		// Append a space and the specified suffix.
		str += ' ';
		str += suffix;
	}

	return addField_string(name, str, flags);
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
int RomFields::addField_bitfield(const char *name,
	const std::vector<string> *bit_names,
	int elemsPerRow, uint32_t bitfield)
{
	assert(name != nullptr);
	assert(bit_names != nullptr);
	if (!name || !bit_names)
		return -1;

	// RFT_BITFIELD
	RP_D(RomFields);
	int idx = (int)d->fields.size();
	d->fields.resize(idx+1);
	Field &field = d->fields.at(idx);

	field.name = (name ? name : "");
	field.type = RFT_BITFIELD;
	field.desc.bitfield.elements = (int)bit_names->size();	// TODO: Remove this.
	field.desc.bitfield.elemsPerRow = elemsPerRow;
	field.desc.bitfield.names = bit_names;
	field.data.bitfield = bitfield;
	field.tabIdx = d->tabIdx;
	field.isValid = true;
	return idx;
}

/**
 * Add ListData.
 * NOTE: This object takes ownership of the two vectors.
 * @param name Field name.
 * @param headers Vector of column names.
 * @param list_data ListData.
 * @param rows_visible Number of visible rows, (0 for "default")
 * @param flags Flags.
 * @param checkboxes Checkbox bitfield. (requires RFT_LISTVIEW_CHECKBOXES)
 * @return Field index, or -1 on error.
 */
int RomFields::addField_listData(const char *name,
	const std::vector<string> *headers,
	const std::vector<std::vector<string> > *list_data,
	int rows_visible, unsigned int flags, uint32_t checkboxes)
{
	assert(name != nullptr);
	assert(rows_visible >= 0);
	if (!name || rows_visible < 0)
		return -1;

	// RFT_LISTDATA
	RP_D(RomFields);
	int idx = (int)d->fields.size();
	d->fields.resize(idx+1);
	Field &field = d->fields.at(idx);

	field.name = (name ? name : "");
	field.type = RFT_LISTDATA;
	field.desc.list_data.flags = flags;
	field.desc.list_data.rows_visible = rows_visible;
	field.desc.list_data.names = headers;
	field.data.list_data = list_data;
	field.data.list_checkboxes = checkboxes;
	field.tabIdx = d->tabIdx;
	field.isValid = true;
	return idx;
}

/**
 * Add DateTime.
 * @param name Field name.
 * @param date_time Date/Time.
 * @param flags Date/Time flags.
 * @return Field index, or -1 on error.
 */
int RomFields::addField_dateTime(const char *name, time_t date_time, unsigned int flags)
{
	assert(name != nullptr);
	if (!name)
		return -1;

	// RFT_DATETIME
	RP_D(RomFields);
	int idx = (int)d->fields.size();
	d->fields.resize(idx+1);
	Field &field = d->fields.at(idx);

	field.name = (name ? name : "");
	field.type = RFT_DATETIME;
	field.desc.flags = flags;
	field.data.date_time = date_time;
	field.tabIdx = d->tabIdx;
	field.isValid = true;
	return idx;
}

/**
 * Add age ratings.
 * The array is copied into the RomFields struct.
 * @param name Field name.
 * @param age_ratings Pointer to age ratings array.
 * @return Field index, or -1 on error.
 */
int RomFields::addField_ageRatings(const char *name, const age_ratings_t &age_ratings)
{
	assert(name != nullptr);
	if (!name)
		return -1;

	// RFT_AGE_RATINGS
	RP_D(RomFields);
	int idx = (int)d->fields.size();
	d->fields.resize(idx+1);
	Field &field = d->fields.at(idx);

	field.name = (name ? name : "");
	field.type = RFT_AGE_RATINGS;
	field.data.age_ratings = new age_ratings_t(age_ratings);
	field.tabIdx = d->tabIdx;
	field.isValid = true;
	return idx;
}

}
