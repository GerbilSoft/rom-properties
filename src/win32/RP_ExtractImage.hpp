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

class RegKey;
class RP_ExtractImage_Private;

class UUID_ATTR("{84573BC0-9502-42F8-8066-CC527D0779E5}")
RP_ExtractImage : public RP_ComBase2<IExtractImage2, IPersistFile>
{
	public:
		RP_ExtractImage();
		virtual ~RP_ExtractImage();

	private:
		typedef RP_ComBase2<IExtractImage2, IPersistFile> super;
		RP_ExtractImage(const RP_ExtractImage &other);
		RP_ExtractImage &operator=(const RP_ExtractImage &other);
	private:
		friend class RP_ExtractImage_Private;
		RP_ExtractImage_Private *const d_ptr;

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
		 * @param hkey_Assoc File association key to register under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(RegKey &hkey_Assoc);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterCLSID(void);

		/**
		 * Unregister the file type handler.
		 * @param hkey_Assoc File association key to register under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(RegKey &hkey_Assoc);

	public:
		// IExtractImage
		IFACEMETHODIMP GetLocation(LPWSTR pszPathBuffer, DWORD cchMax,
			DWORD *pdwPriority, const SIZE *prgSize,
			DWORD dwRecClrDepth, DWORD *pdwFlags) final;
		IFACEMETHODIMP Extract(HBITMAP *phBmpImage) final;
		// IExtractImage2
		IFACEMETHODIMP GetDateStamp(FILETIME *pDateStamp) final;

		// IPersist (IPersistFile base class)
		IFACEMETHODIMP GetClassID(CLSID *pClassID) final;
		// IPersistFile
		IFACEMETHODIMP IsDirty(void) final;
		IFACEMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) final;
		IFACEMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) final;
		IFACEMETHODIMP SaveCompleted(LPCOLESTR pszFileName) final;
		IFACEMETHODIMP GetCurFile(LPOLESTR *ppszFileName) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractImage, 0x84573bc0, 0x9502, 0x42f8, 0x80, 0x66, 0xcc, 0x52, 0x7d, 0x07, 0x79, 0xe5)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__ */
