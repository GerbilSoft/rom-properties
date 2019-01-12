/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PropertyStore.hpp: IPropertyStore implementation.                    *
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

#ifndef __ROMPROPERTIES_WIN32_RP_PROPERTYSTORE_HPP__
#define __ROMPROPERTIES_WIN32_RP_PROPERTYSTORE_HPP__

// librpbase
#include "librpbase/config.librpbase.h"
#include "librpbase/common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "libwin32common/ComBase.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_PropertyStore;
}

namespace LibWin32Common {
	class RegKey;
}

class RP_PropertyStore_Private;

class UUID_ATTR("{4A1E3510-50BD-4B03-A801-E4C954F43B96}")
RP_PropertyStore : public LibWin32Common::ComBase3<IInitializeWithStream, IPropertyStore, IPropertyStoreCapabilities>
{
	public:
		RP_PropertyStore();
	protected:
		virtual ~RP_PropertyStore();

	private:
		typedef LibWin32Common::ComBase3<IInitializeWithStream, IPropertyStore, IPropertyStoreCapabilities> super;
		RP_DISABLE_COPY(RP_PropertyStore)
	private:
		friend class RP_PropertyStore_Private;
		RP_PropertyStore_Private *const d_ptr;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterCLSID(void);

		/**
		 * Register the file type handler.
		 * @param hklm HKEY_LOCAL_MACHINE or user-specific root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(LibWin32Common::RegKey &hklm, LPCWSTR ext);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterCLSID(void);

		/**
		 * Unregister the file type handler.
		 * @param hklm HKEY_LOCAL_MACHINE or user-specific root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(LibWin32Common::RegKey &hklm, LPCWSTR ext);

	public:
		// IInitializeWithStream
		IFACEMETHODIMP Initialize(IStream *pstream, DWORD grfMode) final;

		// IPropertyStore
		IFACEMETHODIMP Commit(void) final;
		IFACEMETHODIMP GetAt(_In_ DWORD iProp, _Out_ PROPERTYKEY *pkey) final;
		IFACEMETHODIMP GetCount(_Out_ DWORD *cProps) final;
		IFACEMETHODIMP GetValue(_In_ REFPROPERTYKEY key, _Out_ PROPVARIANT *pv) final;
		IFACEMETHODIMP SetValue(_In_ REFPROPERTYKEY key, _In_ REFPROPVARIANT propvar) final;

		// IPropertyStoreCapabilities
		IFACEMETHODIMP IsPropertyWritable(REFPROPERTYKEY key) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGw-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_PropertyStore, 0x4a1e3510, 0x50bd, 0x4b03, 0xa8, 0x01, 0xe4, 0xc9, 0x54, 0xf4, 0x3b, 0x96)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_PROPERTYSTORE_HPP__ */
