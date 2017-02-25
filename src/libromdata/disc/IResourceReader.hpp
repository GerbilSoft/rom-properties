/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IResourceReader.hpp: Interface for Windows resource readers.            *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_IRESOURCEREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_IRESOURCEREADER_HPP__

#include "IPartition.hpp"
#include "../exe_structs.h"

// C++ includes.
#include <string>
#include <unordered_map>
#include <vector>

namespace LibRomData {

class IResourceReader : public IPartition
{
	protected:
		IResourceReader() { }
	public:
		virtual ~IResourceReader() = 0;

	private:
		RP_DISABLE_COPY(IResourceReader)

	public:
		/** Resource access functions. **/

		/**
		 * Open a resource.
		 * @param type Resource type ID.
		 * @param id Resource ID. (-1 for "first entry")
		 * @param lang Language ID. (-1 for "first entry")
		 * @return IRpFile*, or nullptr on error.
		 */
		virtual IRpFile *open(uint16_t type, int id, int lang) = 0;

		// StringTable.
		// - Element 1: Key
		// - Element 2: Value
		typedef std::vector<std::pair<rp_string, rp_string> > StringTable;

		// StringFileInfo section.
		// - Key: Langauge ID. (LOWORD = charset, HIWORD = language)
		// - Value: String table.
		typedef std::unordered_map<uint32_t, StringTable> StringFileInfo;

		/**
		 * Load a VS_VERSION_INFO resource.
		 * @param id		[in] Resource ID. (-1 for "first entry")
		 * @param lang		[in] Language ID. (-1 for "first entry")
		 * @param pVsFfi	[out] VS_FIXEDFILEINFO (host-endian)
		 * @param pVsSfi	[out] StringFileInfo section.
		 * @return 0 on success; non-zero on error.
		 */
		virtual int load_VS_VERSION_INFO(int id, int lang, VS_FIXEDFILEINFO *pVsFfi, StringFileInfo *pVsSfi) = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IResourceReader::~IResourceReader() { }

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_IRESOURCEREADER_HPP__ */
