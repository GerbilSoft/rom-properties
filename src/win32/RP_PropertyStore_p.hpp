/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropertyStore_p.hpp: IPropertyStore implementation. (PRIVATE CLASS)  *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_WIN32_RP_PROPERTYSTORE_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_PROPERTYSTORE_P_HPP__

#include "RP_PropertyStore.hpp"

// librpbase
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/RomData.hpp"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_PropertyStorePrivate RP_PropertyStore_Private

// CLSID
extern const CLSID CLSID_RP_PropertyStore;

// C++ includes.
#include <vector>

class RP_PropertyStore_Private
{
	public:
		RP_PropertyStore_Private();
		~RP_PropertyStore_Private();

	private:
		RP_DISABLE_COPY(RP_PropertyStore_Private)

	public:
		// Set by IInitializeWithStream::Initialize().
		LibRpBase::IRpFile *file;

		// IStream* used by the IRpFile.
		// NOTE: Do NOT Release() this; RpFile_IStream handles it.
		IStream *pstream;
		DWORD grfMode;

		// RomData object.
		LibRpBase::RomData *romData;

		// NOTE: prop_key.pid == index + 2,
		// since pids 0 and 1 are reserved.

		// Property keys.
		std::vector<const PROPERTYKEY*> prop_key;
		// Property values.
		std::vector<PROPVARIANT> prop_val;

		/**
		 * Metadata conversion table.
		 * - Index: LibRpBase::Property
		 * - Value:
		 *   - pkey: PROPERTYKEY (if nullptr, not implemented)
		 *   - vtype: Expected variant type.
		 */
		struct MetaDataConv {
			const PROPERTYKEY *pkey;
			LONG vtype;
		};
		static const MetaDataConv metaDataConv[];

	public:
		/**
		 * Register the file type handler.
		 *
		 * Internal version; this only registers for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to register under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(LibWin32Common::RegKey &hkey_Assoc);

		/**
		 * Unregister the file type handler.
		 *
		 * Internal version; this only unregisters for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to unregister under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(LibWin32Common::RegKey &hkey_Assoc);
};

#endif /* __ROMPROPERTIES_WIN32_RP_PROPERTYSTORE_P_HPP__ */
