/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage.hpp: IExtractImage implementation.                      *
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

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__

#include "libromdata/config.libromdata.h"
#include "libromdata/common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "RP_ComBase.hpp"

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
RP_ExtractImage : public RP_ComBase2<IPersistFile, IExtractImage2>
{
	public:
		RP_ExtractImage();
		virtual ~RP_ExtractImage();

	private:
		typedef RP_ComBase2<IPersistFile, IExtractImage> super;
		RP_DISABLE_COPY(RP_ExtractImage)
	private:
		friend class RP_ExtractImage_Private;
		RP_ExtractImage_Private *const d_ptr;

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

		// IExtractImage
		IFACEMETHODIMP GetLocation(LPWSTR pszPathBuffer, DWORD cchMax,
			DWORD *pdwPriority, const SIZE *prgSize,
			DWORD dwRecClrDepth, DWORD *pdwFlags) override final;
		IFACEMETHODIMP Extract(HBITMAP *phBmpImage) override final;
		// IExtractImage2
		IFACEMETHODIMP GetDateStamp(FILETIME *pDateStamp) override final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractImage, 0x84573bc0, 0x9502, 0x42f8, 0x80, 0x66, 0xcc, 0x52, 0x7d, 0x07, 0x79, 0xe5)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__ */
