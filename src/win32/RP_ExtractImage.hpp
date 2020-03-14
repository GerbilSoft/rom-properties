/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage.hpp: IExtractImage implementation.                      *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__

// librpbase
#include "librpbase/config.librpbase.h"
#include "common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "libwin32common/ComBase.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ExtractImage;
}

namespace LibWin32Common {
	class RegKey;
}

class RP_ExtractImage_Private;

class UUID_ATTR("{84573BC0-9502-42F8-8066-CC527D0779E5}")
RP_ExtractImage : public LibWin32Common::ComBase2<IPersistFile, IExtractImage2>
{
	public:
		RP_ExtractImage();
	protected:
		virtual ~RP_ExtractImage();

	private:
		typedef LibWin32Common::ComBase2<IPersistFile, IExtractImage> super;
		RP_DISABLE_COPY(RP_ExtractImage)
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
		 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(LibWin32Common::RegKey &hkcr, LPCTSTR ext);

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
		static LONG UnregisterFileType(LibWin32Common::RegKey &hkcr, LPCTSTR ext);

	public:
		// IPersist (IPersistFile base class)
		IFACEMETHODIMP GetClassID(CLSID *pClassID) final;
		// IPersistFile
		IFACEMETHODIMP IsDirty(void) final;
		IFACEMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) final;
		IFACEMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) final;
		IFACEMETHODIMP SaveCompleted(LPCOLESTR pszFileName) final;
		IFACEMETHODIMP GetCurFile(LPOLESTR *ppszFileName) final;

		// IExtractImage
		IFACEMETHODIMP GetLocation(LPWSTR pszPathBuffer, DWORD cchMax,
			DWORD *pdwPriority, const SIZE *prgSize,
			DWORD dwRecClrDepth, DWORD *pdwFlags) final;
		IFACEMETHODIMP Extract(HBITMAP *phBmpImage) final;
		// IExtractImage2
		IFACEMETHODIMP GetDateStamp(FILETIME *pDateStamp) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractImage, __MSABI_LONG(0x84573bc0), 0x9502, 0x42f8, 0x80,0x66, 0xcc, 0x52, 0x7d, 0x07, 0x79, 0xe5)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_HPP__ */
