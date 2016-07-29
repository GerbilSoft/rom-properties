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

// CLSID
extern "C" {
	extern const GUID CLSID_RP_ExtractIcon;
	extern const wchar_t CLSID_RP_ExtractIcon_Str[];
}

namespace LibRomData {
	class rp_image;
}

// C++ includes.
#include <string>

// FIXME: Use this GUID for RP_ExtractIcon etc.
class __declspec(uuid("{E51BC107-E491-4B29-A6A3-2A4309259802}"))
RP_ExtractIcon : public RP_ComBase2<IExtractIcon, IPersistFile>
{
	public:
		// IUnknown
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj) override;

	protected:
		/**
		 * Convert an rp_image to HICON.
		 * @param image rp_image.
		 * @return HICON, or nullptr on error.
		 */
		static HICON rpToHICON(const LibRomData::rp_image *image);

		// ROM filename from IPersistFile::Load().
		std::wstring m_filename;

	public:
		// IExtractIcon
		STDMETHOD(GetIconLocation)(UINT uFlags, __out_ecount(cchMax) LPTSTR pszIconFile,
			UINT cchMax, __out int *piIndex, __out UINT *pwFlags) override;
		STDMETHOD(Extract)(LPCTSTR pszFile, UINT nIconIndex,
			__out_opt HICON *phiconLarge,__out_opt HICON *phiconSmall,
			UINT nIconSize) override;

		// IPersist (IPersistFile base class)
		STDMETHOD(GetClassID)(__RPC__out CLSID *pClassID) override;
		// IPersistFile
		STDMETHOD(IsDirty)(void) override;
		STDMETHOD(Load)(__RPC__in LPCOLESTR pszFileName, DWORD dwMode) override;
		STDMETHOD(Save)(__RPC__in_opt LPCOLESTR pszFileName, BOOL fRemember) override;
		STDMETHOD(SaveCompleted)(__RPC__in_opt LPCOLESTR pszFileName) override;
		STDMETHOD(GetCurFile)(__RPC__deref_out_opt LPOLESTR *ppszFileName) override;
};

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTICON_H__ */
