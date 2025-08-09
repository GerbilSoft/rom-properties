/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * clsid_common.hpp: CLSID common macros.                                  *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"
#include "libwin32ui/RegKey.hpp"

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
