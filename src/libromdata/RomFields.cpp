/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomFields.cpp: ROM fields class.                                        *
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

#include "RomFields.hpp"
#include "TextFuncs.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class RomFieldsPrivate
{
	public:
		RomFieldsPrivate(const RomFields::Desc *fields, int count);
	private:
		~RomFieldsPrivate();	// call unref() instead

	private:
		RomFieldsPrivate(const RomFieldsPrivate &other);
		RomFieldsPrivate &operator=(const RomFieldsPrivate &other);

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
		 * @return True if refCount > 1; false if not.
		 */
		inline bool isShared(void) const;

	private:
		// Current reference count.
		int refCount;

	public:
		// ROM field descriptions.
		const RomFields::Desc *const fields;
		const int count;

		/**
		 * ROM field data.
		 *
		 * This must be filled in by a RomData class using the
		 * convenience functions.
		 *
		 * NOTE: Strings are *copied* into this vector (to prevent
		 * std::string issues) and are deleted by the destructor.
		 *
		 * NOTE: This RomFieldsPrivate object is shared by all
		 * instances of this RomFields until detach().
		 */
		vector<RomFields::Data> data;

		/**
		 * Deletes allocated strings in this->data.
		 */
		void delete_data(void);
};

/** RomFieldsPrivate **/

RomFieldsPrivate::RomFieldsPrivate(const RomFields::Desc *fields, int count)
	: refCount(1)
	, fields(fields)
	, count(count)
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
	refCount++;
	return this;
}

/**
 * Unreference this object.
 */
void RomFieldsPrivate::unref(void)
{
	assert(refCount > 0);
	if (--refCount == 0) {
		// All references deleted.
		delete this;
	}
}

/**
 * Is this object currently shared?
 * @return True if refCount > 1; false if not.
 */
inline bool RomFieldsPrivate::isShared(void) const
{
	assert(refCount > 0);
	return (refCount > 1);
}

/**
 * Deletes allocated strings in this->data.
 */
void RomFieldsPrivate::delete_data(void)
{
	// Delete all of the allocated strings in this->data.
	for (int i = (int)(data.size() - 1); i >= 0; i--) {
		const RomFields::Data &data = this->data.at(i);
		switch (data.type) {
			case RomFields::RFT_INVALID:
				// No data here.
				break;
			case RomFields::RFT_STRING:
				// Allocated string. Free it.
				free((rp_char*)data.str);
				break;
			case RomFields::RFT_BITFIELD:
				// Nothing needed.
				break;
			case RomFields::RFT_LISTDATA:
				// ListData.
				delete data.list_data;
				break;
			case RomFields::RFT_DATETIME:
				// Nothing needed.
				break;
			default:
				// ERROR!
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;
		}
	}

	// Clear the data vector.
	this->data.clear();
}

/** RomFields **/

/**
 * Initialize a ROM Fields class.
 * @param fields Array of fields.
 * @param count Number of fields.
 */
RomFields::RomFields(const Desc *fields, int count)
	: d(new RomFieldsPrivate(fields, count))
{ }

RomFields::~RomFields()
{
	d->unref();
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RomFields::RomFields(const RomFields &other)
	: d(other.d->ref())
{ }

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RomFields &RomFields::operator=(const RomFields &other)
{
	RomFieldsPrivate *d_old = this->d;
	this->d = other.d->ref();
	d_old->unref();
	return *this;
}

/**
 * Detach this instance from all other instances.
 * TODO: Move to RomFieldsPrivate?
 */
void RomFields::detach(void)
{
	if (!d->isShared()) {
		// Only one reference.
		// Nothing to detach from.
		return;
	}

	// Need to detach.
	RomFieldsPrivate *d_new = new RomFieldsPrivate(d->fields, d->count);
	RomFieldsPrivate *d_old = d;
	d_new->data.reserve(d_old->data.size());
	for (int i = (int)(d_old->data.size() - 1); i >= 0; i--) {
		const Data &data_old = d_old->data.at(i);
		Data &data_new = d_new->data.at(i);
		data_new.type = data_old.type;
		switch (data_old.type) {
			case RFT_INVALID:
				// No data here.
				data_new.str = nullptr;
				break;
			case RFT_STRING:
				// Duplicate the string.
				data_new.str = rp_strdup(data_old.str);
				break;
			case RFT_BITFIELD:
				// Copy the bitfield.
				data_new.bitfield = data_old.bitfield;
				break;
			case RFT_LISTDATA:
				// Copy the ListData.
				data_new.list_data = new ListData(*data_old.list_data);
				break;
			case RFT_DATETIME:
				// Copy the Date/Time.
				data_new.date_time = data_old.date_time;
				break;
			default:
				// ERROR!
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;
		}
	}

	// Detached.
	d = d_new;
	d_old->unref();
}

/** Field accessors. **/

/**
 * Get the number of fields.
 * @return Number of fields.
 */
int RomFields::count(void) const
{
	return d->count;
}

/**
 * Get a ROM field.
 * @return ROM field, or nullptr if the index is invalid.
 */
const RomFields::Desc *RomFields::desc(int idx) const
{
	if (idx < 0 || idx >= d->count)
		return nullptr;
	return &d->fields[idx];
}

/**
 * Get the data for a ROM field.
 * @param idx Field index.
 * @return ROM field data, or nullptr if the index is invalid.
 */
const RomFields::Data *RomFields::data(int idx) const
{
	if (idx < 0 || idx >= d->count ||
	    idx >= (int)d->data.size())
	{
		// Index out of range; or, the index is in range,
		// but no data is available.
		return nullptr;
	}
	return &d->data.at(idx);
}

/**
 * Is data loaded?
 * @return True if m_data has at least one row; false if m_data is nullptr or empty.
 */
bool RomFields::isDataLoaded(void) const
{
	return (!d->data.empty());
}

/** Convenience functions for RomData subclasses. **/

/**
 * Add invalid field data.
 * This effectively hides the field.
 * @return Field index.
 */
int RomFields::addData_invalid(void)
{
	Data data;
	data.type = RFT_INVALID;
	data.str = nullptr;
	d->data.push_back(data);
	return (int)(d->data.size() - 1);
}

/**
 * Add string field data.
 * @param str String.
 * @return Field index.
 */
int RomFields::addData_string(const rp_char *str)
{
	Data data;
	data.type = RFT_STRING;
	data.str = (str ? rp_strdup(str) : nullptr);
	d->data.push_back(data);
	return (int)(d->data.size() - 1);
}

/**
 * Add string field data.
 * @param str String.
 * @return Field index.
 */
int RomFields::addData_string(const rp_string &str)
{
	Data data;
	data.type = RFT_STRING;
	data.str = rp_strdup(str);
	d->data.push_back(data);
	return (int)(d->data.size() - 1);
}

/**
 * Add a string field using a numeric value.
 * @param val Numeric value.
 * @param base Base. If not decimal, a prefix will be added.
 * @param digits Number of leading digits. (0 for none)
 * @return Field index.
 */
int RomFields::addData_string_numeric(uint32_t val, Base base, int digits)
{
	char buf[32];
	int len;
	switch (base) {
		case FB_DEC:
		default:
			len = snprintf(buf, sizeof(buf), "%0*u", digits, val);
			break;
		case FB_HEX:
			len = snprintf(buf, sizeof(buf), "0x%0*X", digits, val);
			break;
		case FB_OCT:
			len = snprintf(buf, sizeof(buf), "0%0*o", digits, val);
			break;
	}

	if (len > (int)sizeof(buf))
		len = sizeof(buf);

	rp_string str = (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
	return addData_string(str);
}

/**
 * Add a string field formatted like a hex dump
 * @param buf Input bytes.
 * @param size Byte count.
 * @return Field index.
 */
int RomFields::addData_string_hexdump(const uint8_t *buf, size_t size)
{
	if (size == 0) {
		return addData_string(_RP(""));
	}

	// Reserve 3 characters per byte.
	// (Two hex digits, plus one space.)
	rp_string rps;
	rps.resize(size * 3);
	size_t rps_pos = 0;

	// Temporary snprintf buffer.
	char hexbuf[8];
	for (; size > 0; size--, buf++, rps_pos += 3) {
		snprintf(hexbuf, sizeof(hexbuf), "%02X ", *buf);
		rps[rps_pos+0] = hexbuf[0];
		rps[rps_pos+1] = hexbuf[1];
		rps[rps_pos+2] = hexbuf[2];
	}

	// Remove the trailing space.
	if (rps.size() > 0) {
		rps.resize(rps.size() - 1);
	}

	return addData_string(rps);
}

/**
 * Add a bitfield.
 * @param bitfield Bitfield.
 * @return Field index.
 */
int RomFields::addData_bitfield(uint32_t bitfield)
{
	Data data;
	data.type = RFT_BITFIELD;
	data.bitfield = bitfield;
	d->data.push_back(data);
	return (int)(d->data.size() - 1);
}

/**
 * Add ListData.
 * @param list_data ListData. (must be allocated with new)
 * @return Field index.
 */
int RomFields::addData_listData(ListData *list_data)
{
	Data data;
	data.type = RFT_LISTDATA;
	data.list_data = list_data;
	d->data.push_back(data);
	return (int)(d->data.size() - 1);
}

/**
 * Add DateTime.
 * @param date_time Date/Time.
 * @return Field index.
 */
int RomFields::addData_dateTime(int64_t date_time)
{
	Data data;
	data.type = RFT_DATETIME;
	data.date_time = date_time;
	d->data.push_back(data);
	return (int)(d->data.size() - 1);
}

}
