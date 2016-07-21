/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData.cpp: ROM data base class.                                       *
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

#include "RomData.hpp"

namespace LibRomData {

/**
 * ROM data base class.
 * Subclass must pass an array of RomField structs.
 */
RomData::RomData(const RomFieldDesc *fields, int fieldCount)
	: m_isValid(false)
	, m_fields(fields)
	, m_fieldCount(fieldCount)
{ }

RomData::~RomData()
{
	// Delete allocated strings in m_fieldData.
	for (int i = (int)(m_fieldData.size() - 1); i >= 0; i--) {
		const RomFieldData &data = m_fieldData.at(i);
		if (data.type == RFT_STRING) {
			// Allocated string. Free it.
			free((rp_char*)data.str);
		}
	}
}

/**
 * Is this ROM valid?
 * @return True if it is; false if it isn't.
 */
bool RomData::isValid(void) const
{
	return m_isValid;
}

/**
 * Get the number of fields.
 * @return Number of fields.
 */
int RomData::count(void) const
{
	return m_fieldCount;
}

/**
 * Get a ROM field.
 * @return ROM field, or nullptr if the index is invalid.
 */
const RomData::RomFieldDesc *RomData::desc(int idx) const
{
	if (idx < 0 || idx >= m_fieldCount)
		return nullptr;
	return &m_fields[idx];
}

/**
 * Get the data for a ROM field.
 * @param idx Field index.
 * @return ROM field data, or nullptr if the index is invalid.
 */
const RomData::RomFieldData *RomData::data(int idx) const
{
	if (idx < 0 || idx >= m_fieldCount || idx >= (int)m_fieldData.size())
		return nullptr;
	return &m_fieldData.at(idx);
}

/**
 * Add a string field.
 * @param str String.
 * @return Field index.
 */
int RomData::addField_string(const rp_char *str)
{
	RomFieldData data;
	data.type = RFT_STRING;
	data.str = rp_strdup(str);
	m_fieldData.push_back(data);
	return (int)(m_fieldData.size() - 1);
}

/**
 * Add a string field.
 * @param str String.
 * @return Field index.
 */
int RomData::addField_string(const rp_string &str)
{
	RomFieldData data;
	data.type = RFT_STRING;
	data.str = rp_strdup(str);
	m_fieldData.push_back(data);
	return (int)(m_fieldData.size() - 1);
}

}
