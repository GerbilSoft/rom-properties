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

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

/**
 * Initialize a ROM Fields class.
 * @param fields Array of fields.
 * @param count Number of fields.
 */
RomFields::RomFields(const Desc *fields, int count)
	: m_fields(fields)
	, m_count(count)
	, m_data(nullptr)
	, m_refCount(new int(1))
{ }

RomFields::~RomFields()
{
	assert(*m_refCount > 0);
	if (--(*m_refCount) == 0) {
		// All references deleted.
		delete_data();
		delete m_refCount;
	}
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RomFields::RomFields(const RomFields &other)
	: m_fields(other.m_fields)
	, m_count(other.m_count)
	, m_data(other.m_data)
	, m_refCount(other.m_refCount)
{
	// Increment the reference count.
	(*m_refCount)++;
}

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RomFields &RomFields::operator=(const RomFields &other)
{
	assert(*m_refCount > 0);

	// Discard the existing data.
	if (--(*m_refCount) == 0) {
		// All references deleted.
		delete_data();
		delete m_refCount;
	}

	// Copy the other object's data pointers and
	// increment its reference counter.
	m_data = other.m_data;
	m_refCount = other.m_refCount;
	(*m_refCount)++;

	// Return this object, as per convention.
	return *this;
}

/**
 * Detach this instance from all other instances.
 */
void RomFields::detach(void)
{
	assert(*m_refCount > 0);
	if (*m_refCount <= 1) {
		// Only one reference.
		// Nothing to detach from.
		return;
	}

	// Need to detach.
	if (m_data) {
		vector<Data> *new_data = new vector<Data>(*m_data);
		for (int i = (int)(m_data->size() - 1); i >= 0; i--) {
			Data &data = m_data->at(i);
			switch (data.type) {
				case RFT_STRING:
					// Duplicate the string.
					data.str = rp_strdup(data.str);
					break;
				case RFT_BITFIELD:
					// Nothing needed.
					break;
				default:
					// ERROR!
					assert(false);
					break;
			}
		}

		m_data = new_data;
	}

	// Detached.
	m_refCount = new int(1);
}

/**
 * Delete m_data.
 */
void RomFields::delete_data(void)
{
	if (!m_data)
		return;

	// Delete all of the allocated strings in m_data.
	for (int i = (int)(m_data->size() - 1); i >= 0; i--) {
		const Data &data = m_data->at(i);
		switch (data.type) {
			case RFT_STRING:
				// Allocated string. Free it.
				free((rp_char*)data.str);
				break;
			case RFT_BITFIELD:
				// Nothing needed.
				break;
			default:
				// ERROR!
				assert(false);
				break;
		}
	}

	// Delete the m_data vector.
	delete m_data;
	m_data = nullptr;
}

/** Field accessors. **/

/**
 * Get the number of fields.
 * @return Number of fields.
 */
int RomFields::count(void) const
{
	return m_count;
}

/**
 * Get a ROM field.
 * @return ROM field, or nullptr if the index is invalid.
 */
const RomFields::Desc *RomFields::desc(int idx) const
{
	if (idx < 0 || idx >= m_count)
		return nullptr;
	return &m_fields[idx];
}

/**
 * Get the data for a ROM field.
 * @param idx Field index.
 * @return ROM field data, or nullptr if the index is invalid.
 */
const RomFields::Data *RomFields::data(int idx) const
{
	if (idx < 0 || idx >= m_count ||
	    !m_data || idx >= (int)m_data->size())
	{
		// Index out of range; or, the index is in range,
		// but no data is available.
		return nullptr;
	}
	return &m_data->at(idx);
}

/**
 * Is data loaded?
 * @return True if m_data has at least one row; false if m_data is nullptr or empty.
 */
bool RomFields::isDataLoaded(void) const
{
	return (m_data && !m_data->empty());
}

/** Convenience functions for RomData subclasses. **/

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

	if (!m_data)
		m_data = new vector<Data>();
	m_data->push_back(data);
	return (int)(m_data->size() - 1);
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

	if (!m_data)
		m_data = new vector<Data>();
	m_data->push_back(data);
	return (int)(m_data->size() - 1);
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

	rp_string str = (len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));
	return addData_string(str);
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

	if (!m_data)
		m_data = new vector<Data>();
	m_data->push_back(data);
	return (int)(m_data->size() - 1);
}

}
