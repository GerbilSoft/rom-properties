/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.hpp: IExtractIcon implementation.                        *
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

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__

#include "libromdata/config.libromdata.h"
#include "libromdata/common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "RP_ComBase.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ExtractIcon;
}

namespace LibRomData {
	class rp_image;
	class RomData;
}

class RegKey;
class RP_ExtractIcon_Private;

class UUID_ATTR("{E51BC107-E491-4B29-A6A3-2A4309259802}")
RP_ExtractIcon : public RP_ComBase3<IPersistFile, IExtractIconW, IExtractIconA>
{
	public:
		RP_ExtractIcon();
		virtual ~RP_ExtractIcon();

	private:
		typedef RP_ComBase3<IPersistFile, IExtractIconW, IExtractIconA> super;
		RP_DISABLE_COPY(RP_ExtractIcon)
	private:
		friend class RP_ExtractIcon_Private;
		RP_ExtractIcon_Private *const d_ptr;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override final;

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterCLSID(void);

		/**
		 * Register the file type handler.
		 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(RegKey &hkcr, LPCWSTR ext);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterCLSID(void);

		/**
		 * Unregister the file type handler.
		 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(RegKey &hkcr, LPCWSTR ext);

	public:
		// IPersist (IPersistFile base class)
		IFACEMETHODIMP GetClassID(CLSID *pClassID) override final;
		// IPersistFile
		IFACEMETHODIMP IsDirty(void) override final;
		IFACEMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) override final;
		IFACEMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) override final;
		IFACEMETHODIMP SaveCompleted(LPCOLESTR pszFileName) override final;
		IFACEMETHODIMP GetCurFile(LPOLESTR *ppszFileName) override final;

		// IExtractIconW
		IFACEMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pszIconFile,
			UINT cchMax, int *piIndex, UINT *pwFlags) override final;
		IFACEMETHODIMP Extract(LPCWSTR pszFile, UINT nIconIndex,
			HICON *phiconLarge, HICON *phiconSmall,
			UINT nIconSize) override final;

		// IExtractIconA
		IFACEMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile,
			UINT cchMax, int *piIndex, UINT *pwFlags) override final;
		IFACEMETHODIMP Extract(LPCSTR pszFile, UINT nIconIndex,
			HICON *phiconLarge, HICON *phiconSmall,
			UINT nIconSize) override final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGw-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractIcon, 0xe51bc107, 0xe491, 0x4b29, 0xa6, 0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__ */
