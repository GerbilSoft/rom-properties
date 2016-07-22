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
 * Subclass must pass an array of RomFieldDesc structs.
 * @param fields Array of ROM Field descriptions.
 * @param count Number of ROM Field descriptions.
 */
RomData::RomData(const RomFields::Desc *fields, int count)
	: m_isValid(false)
	, m_fields(new RomFields(fields, count))
{ }

RomData::~RomData()
{
	delete m_fields;
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
 * Get the ROM Fields object.
 * @return ROM Fields object.
 */
const RomFields *RomData::fields(void) const
{
	return m_fields;
}

}
