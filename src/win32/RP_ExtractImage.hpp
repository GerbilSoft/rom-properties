/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage.hpp: IExtractImage implementation.                      *
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

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C

#include "RP_ComBase.hpp"
#include "libromdata/config.libromdata.h"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ExtractImage;
}

namespace LibRomData {
	class rp_image;
}

// C++ includes.
#include <string>

class UUID_ATTR("{84573BC0-9502-42F8-8066-CC527D0779E5}")
RP_ExtractImage : public RP_ComBase2<IExtractImage, IPersistFile>
{
	public:
		RP_ExtractImage();
	private:
		typedef RP_ComBase2<IExtractImage, IPersistFile> super;

	public:
		// IUnknown
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj) override;

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

		// Requested thumbnail size from IExtractImage::GetLocation().
		SIZE m_bmSize;

	public:
		// IExtractImage
		STDMETHOD(GetLocation)(LPWSTR pszPathBuffer, DWORD cchMax,
			DWORD *pdwPriority, const SIZE *prgSize,
			DWORD dwRecClrDepth, DWORD *pdwFlags) override;
		STDMETHOD(Extract)(HBITMAP *phBmpImage) override;

		// IPersist (IPersistFile base class)
		STDMETHOD(GetClassID)(CLSID *pClassID) override;
		// IPersistFile
		STDMETHOD(IsDirty)(void) override;
		STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode) override;
		STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember) override;
		STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName) override;
		STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName) override;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractImage, 0x84573bc0, 0x9502, 0x42f8, 0x80, 0x66, 0xcc, 0x52, 0x7d, 0x07, 0x79, 0xe5)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__ */
