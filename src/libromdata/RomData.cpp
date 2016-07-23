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
#include "rp_image.hpp"

// C includes.
#include <unistd.h>

namespace LibRomData {

/**
 * ROM data base class.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * In addition, subclasses must pass an array of RomFielDesc structs.
 *
 * @param file ROM file.
 * @param fields Array of ROM Field descriptions.
 * @param count Number of ROM Field descriptions.
 */
RomData::RomData(FILE *file, const RomFields::Desc *fields, int count)
	: m_isValid(false)
	, m_file(nullptr)
	, m_fields(new RomFields(fields, count))
	, m_icon(nullptr)
{
	// TODO: Windows version.
	// dup() the file.
	int fd_old = fileno(file);
	int fd_new = dup(fd_old);
	if (fd_new >= 0) {
		m_file = fdopen(fd_new, "rb");
		if (m_file) {
			// Make sure we're at the beginning of the file.
			rewind(m_file);
			fflush(m_file);
		}
	}
}

RomData::~RomData()
{
	this->close();
	delete m_fields;
	delete m_icon;
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
 * Close the opened file.
 */
void RomData::close(void)
{
	if (m_file) {
		fclose(m_file);
		m_file = nullptr;
	}
}

/**
 * Load the internal icon.
 * Called by RomData::icon() if the icon data hasn't been loaded yet.
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::loadInternalIcon(void)
{
	// No icon by default.
	return -ENOENT;
}

/**
 * Get the ROM Fields object.
 * @return ROM Fields object.
 */
const RomFields *RomData::fields(void) const
{
	if (!m_fields->isDataLoaded()) {
		// Data has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadFieldData();
		if (ret < 0)
			return nullptr;
	}
	return m_fields;
}

/**
 * Get the ROM's internal icon.
 * @return Internal icon, or nullptr if the ROM doesn't have one.
 */
const rp_image *RomData::icon(void) const
{
	if (!m_icon) {
		// Internal icon has not been loaded.
		// Load it now.
		// TODO: Some flag to indicate if a class supports it?
		int ret = const_cast<RomData*>(this)->loadInternalIcon();
		if (ret < 0)
			return nullptr;
	}

	return m_icon;
}

}
