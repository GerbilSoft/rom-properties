/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * clsid_common.hpp: CLSID common macros.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"
#include "libwin32ui/RegKey.hpp"

// CLSID register/unregister function declarations
#define CLSID_DECL(klass) \
	/** \
	 * Register the COM object. \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG RegisterCLSID(void); \
\
	/** \
	 * Unregister the COM object. \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG UnregisterCLSID(void);

// CLSID register/unregister function declarations (no inline)
#define CLSID_DECL_NOINLINE(klass) \
	/** \
	 * Register the COM object. \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG RegisterCLSID(void); \
\
	/** \
	 * Unregister the COM object. \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG UnregisterCLSID(void);

// CLSID register/unregister function implementations
#define CLSID_IMPL(klass, description) \
/** \
 * Register the COM object. \
 * @return ERROR_SUCCESS on success; Win32 error code on error. \
 */ \
LONG klass::RegisterCLSID(void) \
{ \
	extern const TCHAR RP_ProgID[]; \
\
	/* Register the COM object. */ \
	LONG lResult = LibWin32UI::RegKey::RegisterComObject(HINST_THISCOMPONENT, \
		__uuidof(klass), RP_ProgID, description); \
	if (lResult != ERROR_SUCCESS) { \
		return lResult; \
	} \
\
	/* Register as an "approved" shell extension. */ \
	return LibWin32UI::RegKey::RegisterApprovedExtension(__uuidof(klass), description); \
} \
\
/** \
 * Unregister the COM object. \
 * @return ERROR_SUCCESS on success; Win32 error code on error. \
 */ \
LONG klass::UnregisterCLSID(void) \
{ \
	extern const TCHAR RP_ProgID[]; \
	return LibWin32UI::RegKey::UnregisterComObject(__uuidof(klass), RP_ProgID); \
}


// Filetype register/unregister function declarations
#define FILETYPE_HANDLER_DECL(klass) \
	/** \
	 * Register the file type handler. \
	 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root. \
	 * @param ext	[in] File extension, including the leading dot. \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG RegisterFileType(_In_ LibWin32UI::RegKey &hkcr, _In_ LPCTSTR ext); \
\
	/** \
	 * Unregister the file type handler. \
	 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root. \
	 * @param ext	[in,opt] File extension, including the leading dot. \
	 * \
	 * NOTE: ext can be NULL, in which case, hkcr is assumed to be \
	 * the registered file association. \
	 * \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG UnregisterFileType(_In_ LibWin32UI::RegKey &hkcr, _In_opt_ LPCTSTR ext);

// Filetype register/unregister function declarations (with pHklm)
#define FILETYPE_HANDLER_HKLM_DECL(klass) \
	/** \
	 * Register the file type handler. \
	 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root. \
	 * @param pHklm	[in,opt] HKEY_LOCAL_MACHINE or user-specific root, or nullptr to skip. \
	 * @param ext	[in] File extension, including the leading dot. \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG RegisterFileType(_In_ LibWin32UI::RegKey &hkcr, _In_opt_ LibWin32UI::RegKey *pHklm, _In_ LPCTSTR ext); \
\
	/** \
	 * Unregister the file type handler. \
	 * @param hkcr	[in] HKEY_CLASSES_ROOT or user-specific classes root. \
	 * @param pHklm	[in,opt] HKEY_LOCAL_MACHINE or user-specific root, or nullptr to skip. \
	 * @param ext	[in,opt] File extension, including the leading dot. \
	 * \
	 * NOTE: ext can be NULL, in which case, hkcr is assumed to be \
	 * the registered file association. \
	 * \
	 * @return ERROR_SUCCESS on success; Win32 error code on error. \
	 */ \
	static LONG UnregisterFileType(_In_ LibWin32UI::RegKey &hkcr, _In_opt_ LibWin32UI::RegKey *pHklm, _In_opt_ LPCTSTR ext);
