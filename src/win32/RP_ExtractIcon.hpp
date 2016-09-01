/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon.hpp: IExtractIcon implementation.                        *
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

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C

#include "RP_ComBase.hpp"
#include "libromdata/config.libromdata.h"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ExtractIcon;
}

namespace LibRomData {
	class rp_image;
}

// C++ includes.
#include <string>

class UUID_ATTR("{E51BC107-E491-4B29-A6A3-2A4309259802}")
RP_ExtractIcon : public RP_ComBase2<IExtractIcon, IPersistFile>
{
	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override;
	private:
		typedef RP_ComBase2<IExtractIcon, IPersistFile> super;

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG Register(void);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG Unregister(void);

 	protected:
		// ROM filename from IPersistFile::Load().
		LibRomData::rp_string m_filename;

	public:
		// IExtractIcon
		IFACEMETHODIMP GetIconLocation(UINT uFlags, LPTSTR pszIconFile,
			UINT cchMax, int *piIndex, UINT *pwFlags) override;
		IFACEMETHODIMP Extract(LPCTSTR pszFile, UINT nIconIndex,
			HICON *phiconLarge, HICON *phiconSmall,
			UINT nIconSize) override;

		// IPersist (IPersistFile base class)
		IFACEMETHODIMP GetClassID(CLSID *pClassID) override;
		// IPersistFile
		IFACEMETHODIMP IsDirty(void) override;
		IFACEMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) override;
		IFACEMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) override;
		IFACEMETHODIMP SaveCompleted(LPCOLESTR pszFileName) override;
		IFACEMETHODIMP GetCurFile(LPOLESTR *ppszFileName) override;
};

#ifdef __CRT_UUID_DECL
// Required for MinGw-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractIcon, 0xe51bc107, 0xe491, 0x4b29, 0xa6, 0xa3, 0x2a, 0x43, 0x09, 0x25, 0x98, 0x02)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__ */
