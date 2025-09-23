/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DllRegisterServer.cpp: COM registration handler.                        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Based on "The Complete Idiot's Guide to Writing Shell Extensions" - Part V
// http://www.codeproject.com/Articles/463/The-Complete-Idiots-Guide-to-Writing-Shell-Exten
// Demo code was released into the public domain.

// Other references:
// - "A very simple COM server without ATL or MFC"
//   - gcc and MSVC Express do not support ATL, so we can't use ATL here.
//   - http://www.codeproject.com/Articles/665/A-very-simple-COM-server-without-ATL-or-MFC
// - "COM in C++"
//   - http://www.codeproject.com/Articles/338268/COM-in-C

#include "stdafx.h"
#include "config.version.h"
#include "config.win32.h"

#include "RP_ClassFactory.hpp"
#include "RP_ExtractIcon.hpp"
#include "RP_ExtractImage.hpp"
#include "RP_ShellPropSheetExt.hpp"
#include "RP_ThumbnailProvider.hpp"
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
#  include "RP_PropertyStore.hpp"
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
#include "RP_ShellIconOverlayIdentifier.hpp"
#include "RP_ContextMenu.hpp"
#include "RP_XAttrView.hpp"
#include "RP_ColumnProvider.hpp"

// libwin32common
using LibWin32UI::RegKey;

// For file extensions
#include "libromdata/RomDataFactory.hpp"
#include "librptexture/FileFormatFactory.hpp"
using namespace LibRomData;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::forward_list;
using std::string;
using std::vector;
using std::wstring;

// Program ID for COM object registration.
extern const TCHAR RP_ProgID[];
const TCHAR RP_ProgID[] = _T("rom-properties");

/**
 * Register file type handlers.
 * @param hkcr HKEY_CLASSES_ROOT, or user-specific Classes key.
 * @param pHklm HKEY_LOCAL_MACHINE, or user-specific root. (If nullptr, skip RP_PropertyStore.)
 * @param extInfo RomDataFactory::ExtInfo
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG RegisterFileType(RegKey &hkcr, RegKey *pHklm, const RomDataFactory::ExtInfo &extInfo)
{
	LONG lResult;

	// Register the filetype in HKCR.
	const tstring t_ext = U82T_c(extInfo.ext);

	RegKey *pHkey_fileType;
	lResult = RegKey::RegisterFileType(t_ext.c_str(), &pHkey_fileType);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// If the ProgID was previously set to RP_ProgID,
	// unset it, since we're not using it anymore.
	const tstring progID = pHkey_fileType->read(nullptr);
	if (progID == RP_ProgID) {
		// Unset the ProgID.
		lResult = pHkey_fileType->deleteValue(nullptr);
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			delete pHkey_fileType;
			return SELFREG_E_CLASS;
		}
	}
	delete pHkey_fileType;

	// Unregister the property page handler.
	// We're now registering it for all files instead. ("*")
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// If "OpenWithProgids/rom-properties" is set, remove it.
	// We're setting RP_PropertyStore settings per file extension
	// to prevent issues opening .cmd files on some versions of
	// Windows 10. (Works on Win7 SP1 and Win10 LTSC 1809...)
	RegKey hkey_ext(hkcr, t_ext.c_str(), KEY_READ|KEY_WRITE, false);
	if (hkey_ext.isOpen()) {
		RegKey hkey_OpenWithProgids(hkey_ext, _T("OpenWithProgids"), KEY_READ|KEY_WRITE, false);
		if (hkey_OpenWithProgids.isOpen()) {
			hkey_OpenWithProgids.deleteValue(RP_ProgID);
			if (hkey_OpenWithProgids.isKeyEmpty()) {
				// OpenWithProgids is empty. Delete it.
				hkey_OpenWithProgids.close();
				hkey_ext.deleteSubKey(_T("OpenWithProgids"));
			}
		}
	}
	hkey_ext.close();

#ifdef HAVE_RP_PROPERTYSTORE_DEPS
	// Register the property store handler.
	// TODO: Register for all files?
	if (extInfo.attrs & RomDataFactory::RDA_HAS_METADATA) {
		lResult = RP_PropertyStore::RegisterFileType(hkcr, pHklm, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}
#else /* HAVE_RP_PROPERTYSTORE_DEPS */
	// MinGW-w64: Suppress pHklm warnings, since it's unused.
	((void)pHklm);
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */

	if (extInfo.attrs & RomDataFactory::RDA_HAS_THUMBNAIL) {
		// Register the thumbnail handlers.
		lResult = RP_ExtractIcon::RegisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ExtractImage::RegisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ThumbnailProvider::RegisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	} else {
		// No thumbnail handlers.
		// Unregister the handlers if they were previously registered.
		lResult = RP_ExtractIcon::UnregisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ExtractImage::UnregisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ThumbnailProvider::UnregisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}

	// Register the context menu handler.
	// TODO: Better search method?
	const vector<const char*> &texture_exts = FileFormatFactory::supportedFileExtensions();

	bool is_texture = false;
	for (const char *texture_ext : texture_exts) {
		if (!strcasecmp(texture_ext, extInfo.ext)) {
			is_texture = true;
			break;
		}
	}

	if (is_texture) {
		// Register the context menu handler.
		lResult = RP_ContextMenu::RegisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	} else {
		// Not a texture file extension.
		// Unregister the handler if it was previously registered.
		lResult = RP_ContextMenu::UnregisterFileType(hkcr, t_ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}

	// All file type handlers registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister file type handlers.
 * @param hkcr HKEY_CLASSES_ROOT, or user-specific Classes key.
 * @param pHklm HKEY_LOCAL_MACHINE, or user-specific root. (If nullptr, skip RP_PropertyStore.)
 * @param extInfo RomDataFactory::ExtInfo
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG UnregisterFileType(RegKey &hkcr, RegKey *pHklm, const RomDataFactory::ExtInfo &extInfo)
{
	LONG lResult;

	// Open the file type key if it's present.
	const tstring t_ext = U82T_c(extInfo.ext);

	RegKey hkey_fileType(hkcr, t_ext.c_str(), KEY_READ|KEY_WRITE, false);
	if (!hkey_fileType.isOpen()) {
		// Not open...
		if (hkey_fileType.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			// Key not found.
			return ERROR_SUCCESS;
		}
		// Other error.
		return hkey_fileType.lOpenRes();
	}

	// If the ProgID was previously set to RP_ProgID,
	// unset it, since we're not using it anymore.
	tstring progID = hkey_fileType.read(nullptr);
	if (progID == RP_ProgID) {
		// Unset the ProgID.
		lResult = hkey_fileType.deleteValue(nullptr);
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
			return lResult;
		}
		// No need to delete subkeys from the ProgID later.
		progID.clear();
	}

	// Unregister all classes.
	lResult = RP_ExtractIcon::UnregisterFileType(hkcr, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::UnregisterFileType(hkcr, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ThumbnailProvider::UnregisterFileType(hkcr, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
	lResult = RP_PropertyStore::UnregisterFileType(hkcr, pHklm, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
#else /* HAVE_RP_PROPERTYSTORE_DEPS */
	// MinGW-w64: Suppress pHklm warnings, since it's unused.
	((void)pHklm);
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
	lResult = RP_ContextMenu::UnregisterFileType(hkcr, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_XAttrView::UnregisterFileType(hkcr, t_ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Delete keys if they're empty.
	static const array<LPCTSTR, 2> keysToDel = {{_T("ShellEx"), _T("RP_Fallback")}};
	for (LPCTSTR keyToDel : keysToDel) {
		// Check if the key is empty.
		RegKey hkey_del(hkey_fileType, keyToDel, KEY_READ, false);
		if (!hkey_del.isOpen())
			continue;

		// Check if the key is empty.
		// TODO: Error handling.
		if (hkey_del.isKeyEmpty()) {
			// No subkeys. Delete this key.
			hkey_del.close();
			hkey_fileType.deleteSubKey(keyToDel);
		}
	}

	// Remove "OpenWithProgids/rom-properties" if it's present.
	RegKey hkey_ext(hkcr, t_ext.c_str(), KEY_READ|KEY_WRITE, false);
	if (hkey_ext.isOpen()) {
		RegKey hkey_OpenWithProgids(hkey_ext, _T("OpenWithProgids"), KEY_READ|KEY_WRITE, false);
		if (hkey_OpenWithProgids.isOpen()) {
			hkey_OpenWithProgids.deleteValue(RP_ProgID);
			if (hkey_OpenWithProgids.isKeyEmpty()) {
				// OpenWithProgids is empty. Delete it.
				hkey_OpenWithProgids.close();
				hkey_ext.deleteSubKey(_T("OpenWithProgids"));
			}
		}
	}
	hkey_ext.close();

	// Is a custom ProgID registered?
	// If so, we should check for empty keys there, too.
	if (!progID.empty()) {
		// Custom ProgID is registered.
		RegKey hkey_ProgID(hkcr, progID.c_str(), KEY_READ|KEY_WRITE, false);
		for (const TCHAR *keyToDel : keysToDel) {
			// Check if the key is empty.
			RegKey hkey_del(hkey_ProgID, keyToDel, KEY_READ, false);
			if (!hkey_del.isOpen())
				continue;

			// Check if the key is empty.
			// TODO: Error handling.
			if (hkey_del.isKeyEmpty()) {
				// No subkeys. Delete this key.
				hkey_del.close();
				hkey_ProgID.deleteSubKey(keyToDel);
			}
		}
	}

	// All file type handlers unregistered.
	return ERROR_SUCCESS;
}

/**
 * Get the user's overriden file association for the given file extension.
 * @param sid User SID.
 * @param ext File extension. (UTF-8)
 * @return Overridden file association ProgID, or empty string if none.
 */
static tstring GetUserFileAssoc(const tstring &sid, const char *ext)
{
	// Check if the user has already associated this file extension.
	// TODO: Check all users.

	tstring ts_regPath = fmt::format(
		FSTR(_T("{:s}\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\{:s}\\UserChoice")),
			sid, U82T_c(ext));

	// FIXME: This will NOT update profiles that aren't loaded.
	// Other profiles will need to be loaded manually, or those users
	// will have to register the DLL themselves.
	// Reference: http://windowsitpro.com/scripting/how-can-i-update-all-profiles-machine-even-if-theyre-not-currently-loaded
	RegKey hkcu_UserChoice(HKEY_USERS, ts_regPath.c_str(), KEY_READ, false);
	if (!hkcu_UserChoice.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hkcu_UserChoice.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return {};
		}
		// TODO: Return an error.
		//return hkcu_UserChoice.lOpenRes();
		return {};
	}

	// Read the user's choice.
	return hkcu_UserChoice.read(_T("Progid"));
}

/**
 * Register file type handlers for a user's overridden file association.
 * @param sid User SID.
 * @param ext_info File extension information.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG RegisterUserFileType(const tstring &sid, const RomDataFactory::ExtInfo &ext_info)
{
	// NOTE: We might end up registering RP_PropertyStore
	// multiple times due to HKCR vs. HKLM differences.

	// Get the ProgID.
	// NOTE: Skipping "Applications\\" ProgIDs. These are registered
	// applications and are selected using "UserChoice" on Win8+.
	const tstring progID = GetUserFileAssoc(sid, ext_info.ext);
	if (progID.empty() || !_tcsnicmp(progID.c_str(), _T("Applications\\"), 13)) {
		// No ProgID and/or it's "Applications/".
		return ERROR_SUCCESS;
	}

	// Check both "HKCR" and "HKU\\[sid]".
	// It turns out they aren't identical.

	// First, check HKCR.
	RegKey hkcr(HKEY_CLASSES_ROOT, nullptr, KEY_READ|KEY_WRITE, false);
	if (!hkcr.isOpen()) {
		// Error opening HKEY_CLASSES_ROOT.
		return hkcr.lOpenRes();
	}

	// Use an ExtInfo with the progID instead of the extension.
	const string progID_u8 = T2U8(progID);
	const RomDataFactory::ExtInfo progID_info(
		progID_u8.c_str(),
		ext_info.attrs
	);

	// Does HKCR\\progID exist?
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_WRITE, false);
	if (hkcr_ProgID.isOpen()) {
		LONG lResult = RegisterFileType(hkcr, nullptr, progID_info);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hkcr_ProgID.lOpenRes() != ERROR_FILE_NOT_FOUND) {
			return hkcr_ProgID.lOpenRes();
		}
	}

	// Next, check "HKU\\[sid]".
	RegKey hku(HKEY_USERS, sid.c_str(), KEY_READ, false);
	RegKey hku_cr(hku, _T("Software\\Classes"), KEY_WRITE, false);
	if (hku.isOpen() && hku_cr.isOpen()) {
		LONG lResult = RegisterFileType(hku_cr, nullptr, progID_info);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hku.lOpenRes() != ERROR_FILE_NOT_FOUND) {
			return hku.lOpenRes();
		} else if (hku_cr.lOpenRes() != ERROR_FILE_NOT_FOUND) {
			return hku_cr.lOpenRes();
		}
	}

	return ERROR_SUCCESS;
}

/**
 * Unregister file type handlers for a user's overridden file association.
 * @param sid User SID.
 * @param ext_info File extension information.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG UnregisterUserFileType(const tstring &sid, const RomDataFactory::ExtInfo &ext_info)
{
	// NOTE: We might end up registering RP_PropertyStore
	// multiple times due to HKCR vs. HKLM differences.

	// NOTE: Not skipping "Applications\\" ProgIDs, since these may
	// have been registered by older versions of rom-properties.

	// Get the ProgID.
	const tstring progID = GetUserFileAssoc(sid, ext_info.ext);
	if (progID.empty()) {
		// No ProgID.
		return ERROR_SUCCESS;
	}

	// Check both "HKCR" and "HKU\\[sid]".
	// It turns out they aren't identical.

	// First, check HKCR.
	RegKey hkcr(HKEY_CLASSES_ROOT, nullptr, KEY_READ|KEY_WRITE, false);
	if (!hkcr.isOpen()) {
		// Error opening HKEY_CLASSES_ROOT.
		return hkcr.lOpenRes();
	}

	// Use an ExtInfo with the progID instead of the extension.
	const string progID_u8 = T2U8(progID);
	const RomDataFactory::ExtInfo progID_info(
		progID_u8.c_str(),
		ext_info.attrs
	);

	// Does HKCR\\progID exist?
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_WRITE, false);
	if (hkcr_ProgID.isOpen()) {
		LONG lResult = UnregisterFileType(hkcr, nullptr, progID_info);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hkcr_ProgID.lOpenRes() != ERROR_FILE_NOT_FOUND) {
			return hkcr_ProgID.lOpenRes();
		}
	}

	// Next, check "HKU\\[sid]".
	RegKey hku(HKEY_USERS, sid.c_str(), KEY_READ, false);
	RegKey hku_cr(HKEY_USERS, _T("Software\\Classes"), KEY_WRITE, false);
	if (hku.isOpen() && hku_cr.isOpen()) {
		LONG lResult = UnregisterFileType(hku_cr, nullptr, progID_info);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hku.lOpenRes() != ERROR_FILE_NOT_FOUND) {
			return hku.lOpenRes();
		} else if (hku_cr.lOpenRes() != ERROR_FILE_NOT_FOUND) {
			return hku_cr.lOpenRes();
		}
	}

	return ERROR_SUCCESS;
}

/**
 * Unregister ourselves in any "HKCR\\Applications" entries.
 * This was an error that caused various brokenness with
 * UserChoice on Windows 8+.
 *
 * @param hkcr "HKCR\\Applications" or "HKU\\xxx\\SOFTWARE\\Classes\\Applications".
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG UnregisterFromApplications(RegKey& hkcr)
{
	// Enumerate the subkeys and unregister from each of them.
	forward_list<tstring> lstSubKeys;
	LONG lResult = hkcr.enumSubKeys(lstSubKeys);
	if (lResult != ERROR_SUCCESS || lstSubKeys.empty()) {
		return lResult;
	}

	for (const tstring &subKey : lstSubKeys) {
		RegKey hkey_app(hkcr, subKey.c_str(), KEY_READ|KEY_WRITE, false);
		if (!hkey_app.isOpen())
			continue;

		// Unregister from this Application.
		// NOTE: Not checking results.
		// NOTE: No RP_ShellPropSheetExt unregistration is needed here.
		RP_ExtractIcon::UnregisterFileType(hkey_app, nullptr);
		RP_ExtractImage::UnregisterFileType(hkey_app, nullptr);
		RP_ThumbnailProvider::UnregisterFileType(hkey_app, nullptr);
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
		RP_PropertyStore::UnregisterFileType(hkey_app, nullptr, nullptr);
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
	}

	return ERROR_SUCCESS;
}

/**
 * Remove HKEY_USERS subkeys from an std::list if we don't want to process them.
 * @param subKey Subkey name.
 * @return True to remove; false to keep.
 */
static inline bool process_HKU_subkey(const tstring &subKey)
{
	if (subKey.size() <= 16) {
		// Subkey name is too small.
		return true;
	}

	// Ignore "_Classes" subkeys.
	// These are virtual subkeys that map to:
	// HKEY_USERS\\[sid]\\Software\\Classes
	const TCHAR *const classes_pos = subKey.c_str() + (subKey.size() - 8);
	return (!_tcsicmp(classes_pos, _T("_Classes")));
}

/**
 * Check if the DLL is located in %SystemRoot%.
 * @return True if it is; false if it isn't.
 */
static inline bool checkDirectory(void)
{
	TCHAR szDllFilename[MAX_PATH];
	TCHAR szWinPath[MAX_PATH];

	SetLastError(ERROR_SUCCESS);	// required for XP
	DWORD dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		szDllFilename, _countof(szDllFilename));
	if (dwResult == 0 || dwResult >= _countof(szDllFilename) || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the DLL filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		return FALSE;
	}

	UINT len = GetWindowsDirectory(szWinPath, _countof(szWinPath));
	return !_tcsnicmp(szWinPath, szDllFilename, len);
}

/**
 * Register a COM object.
 * This will also add the COM object to the list of "approved" shell extensions.
 * @param rclsid CLSID
 * @param description COM object description
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG RegisterCLSID(_In_ REFCLSID rclsid, _In_ LPCTSTR description)
{
	// Register the COM object.
	LONG lResult = RegKey::RegisterComObject(HINST_THISCOMPONENT,
		rclsid, RP_ProgID, description);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Register as an "approved" shell extension.
	// NOTE: Only checked by NT 4.0-6.0. Win7 and later ignores it.
	return RegKey::RegisterApprovedExtension(rclsid, description);
}

/**
 * Unregister a COM object.
 * This will also remove the COM object from the list of "approved" shell extensions.
 * @param rclsid CLSID
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG UnregisterCLSID(_In_ REFCLSID rclsid)
{
	// TODO: Split out removal of the "approved" shell extension from UnregisterComObject()?
	return RegKey::UnregisterComObject(rclsid, RP_ProgID);
}

/**
 * Table of CLSIDs to register and/or unregister.
 */
struct CLSID_tbl_t {
	REFCLSID rclsid;
	LPCTSTR description;
};
static const CLSID_tbl_t CLSID_tbl[] = {
	{__uuidof(RP_ExtractIcon),		_T("ROM Properties Page - Icon Extractor")},
	{__uuidof(RP_ExtractImage),		_T("ROM Properties Page - Image Extractor")},
	{__uuidof(RP_ShellPropSheetExt),	_T("ROM Properties Page - Property Sheet")},
	{__uuidof(RP_ThumbnailProvider),	_T("ROM Properties Page - Thumbnail Provider")},
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
	{__uuidof(RP_PropertyStore),		_T("ROM Properties Page - Property Store")},
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
#ifdef ENABLE_OVERLAY_ICON_HANDLER
	{__uuidof(RP_ShellIconOverlayIdentifier), _T("ROM Properties Page - Shell Icon Overlay Identifier")},
#endif /* ENABLE_OVERLAY_ICON_HANDLER */
	{__uuidof(RP_ContextMenu),		_T("ROM Properties Page - Context Menu")},
	{__uuidof(RP_XAttrView),		_T("ROM Properties Page - Extended Attribute viewer")},
	{__uuidof(RP_ColumnProvider),		_T("ROM Properties Page - Column Provider")},
};

/**
 * Register the DLL.
 */
STDAPI DllRegisterServer(void)
{
	LONG lResult;

	if (checkDirectory()) {
		// DLL is in %SystemRoot%. This isn't allowed.
		return E_FAIL;
	}

	// Register the COM objects.
	for (const auto &p : CLSID_tbl) {
		lResult = RegisterCLSID(p.rclsid, p.description);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
	// Unregister the Property Description Schema first before re-registering.
	lResult = RP_PropertyStore::UnregisterPropertyDescriptionSchema();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_PropertyStore::RegisterPropertyDescriptionSchema();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
#ifdef ENABLE_OVERLAY_ICON_HANDLER
	lResult = RP_ShellIconOverlayIdentifer::RegisterShellIconOverlayIdentifier();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
#endif /* ENABLE_OVERLAY_ICON_HANDLER */

	// Enumerate user hives.
	RegKey hku(HKEY_USERS, nullptr, KEY_READ, false);
	if (!hku.isOpen()) return SELFREG_E_CLASS;
	forward_list<tstring> user_SIDs;
	lResult = hku.enumSubKeys(user_SIDs);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	hku.close();

	// Don't check user hives with names that are 16 characters or shorter.
	// These are usually ".DEFAULT" or "well-known" SIDs.
	user_SIDs.remove_if(process_HKU_subkey);

	// Open HKEY_CLASSES_ROOT and HKEY_LOCAL_MACHINE.
	RegKey hkcr(HKEY_CLASSES_ROOT, nullptr, KEY_READ|KEY_WRITE, false);
	if (!hkcr.isOpen()) return SELFREG_E_CLASS;
	RegKey hklm(HKEY_LOCAL_MACHINE, nullptr, KEY_READ, false);
	if (!hklm.isOpen()) return SELFREG_E_CLASS;

	// Remove the ProgID if it exists, since we aren't using it anymore.
	hkcr.deleteSubKey(RP_ProgID);

	// Register all supported file extensions.
	const vector<RomDataFactory::ExtInfo> &vec_exts = RomDataFactory::supportedFileExtensions();
	for (const auto &ext : vec_exts) {
		// Register the file type handlers for this file extension globally.
		lResult = RegisterFileType(hkcr, &hklm, ext);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

		// Register user file types if necessary.
		for (const auto &sid : user_SIDs) {
			lResult = RegisterUserFileType(sid, ext);
			if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		}
	}

	// Register RP_ShellPropSheetExt for all file types.
	// Fixes an issue where it doesn't show up for .dds if
	// Visual Studio 2017 is installed.
	lResult = RP_ShellPropSheetExt::RegisterFileType(hkcr, _T("*"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Register RP_ShellPropSheetExt for disk drives.
	// TODO: Icon/thumbnail handling?
	lResult = RP_ShellPropSheetExt::RegisterFileType(hkcr, _T("Drive"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Register RP_ShellPropSheetExt and thumbnailers for directories.
	lResult = RP_ShellPropSheetExt::RegisterFileType(hkcr, _T("Directory"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractIcon::RegisterFileType(hkcr, _T("Directory"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::RegisterFileType(hkcr, _T("Directory"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	// NOTE: IThumbnailProvider does not work on directories.
	// Unregistering it in case it was registered before.
	lResult = RP_ThumbnailProvider::UnregisterFileType(hkcr, _T("Directory"));
	//if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Register RP_XAttrView for all file types.
	// TODO: Also for drives?
	lResult = RP_XAttrView::RegisterFileType(hkcr, _T("*"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Register RP_ColumnProvider for "Folder".
	// It doesn't work for anything else, contrary to almost all documentation...
	// NOTE: "Folder" == file folder; "Directory" == *all* folders.
	// Reference: https://web.archive.org/web/20071213223408/https://www.codeproject.com/KB/shell/shellextguide8.aspx
	lResult = RP_ColumnProvider::RegisterFileType(hkcr, _T("Folder"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	/** Fixes for previous versions **/

	// NOTE: Some extensions were accidentally registered in previous versions:
	// - LibRomData::EXE: "*.vxd"
	// - LibRomData::MachO: ".dylib.bundle" [v1.4]
	// These extensions will be explicitly deleted here.
	// NOTE: Ignoring any errors to prevent `regsvr32` from failing.
	hkcr.deleteSubKey(_T("*.vxd"));
	hkcr.deleteSubKey(_T(".dylib.bundle"));

	// Unregister ourselves in any "HKCR\\Applications" entries,
	// and similarly for users. This was an error that caused
	// various brokenness with UserChoice on Windows 8+.
	RegKey hkcr_Applications(HKEY_CLASSES_ROOT, _T("Applications"), KEY_READ, false);
	if (hkcr_Applications.isOpen()) {
		UnregisterFromApplications(hkcr_Applications);
	}

	// Per-user versions of the above.
	tstring ts_regPath;
	for (const auto &sid : user_SIDs) {
		// Incorrect file extension registrations.
		ts_regPath = fmt::format(
			FSTR(_T("{:s}\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts")), sid);

		RegKey hkuvxd(HKEY_USERS, ts_regPath.c_str(), KEY_WRITE, false);
		if (hkuvxd.isOpen()) {
			hkuvxd.deleteSubKey(_T("*.vxd"));
			hkuvxd.deleteSubKey(_T(".dylib.bundle"));
		}

		// "HKU\\xxx\\SOFTWARE\\Classes\\Applications" entries
		ts_regPath = fmt::format(FSTR(_T("{:s}\\SOFTWARE\\Classes\\Applications")), sid);
		RegKey hku_Applications(HKEY_USERS, ts_regPath.c_str(), KEY_READ|KEY_WRITE, false);
		if (hku_Applications.isOpen()) {
			UnregisterFromApplications(hku_Applications);
		}
	}

	// Notify the shell that file associations have changed.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/shell/fa-file-types
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

	return S_OK;
}

/**
 * Unregister the DLL.
 */
STDAPI DllUnregisterServer(void)
{
	LONG lResult;

	// Unregister the COM objects.
	for (const auto &p : CLSID_tbl) {
		lResult = UnregisterCLSID(p.rclsid);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}
#ifdef HAVE_RP_PROPERTYSTORE_DEPS
	lResult = RP_PropertyStore::UnregisterPropertyDescriptionSchema();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
#endif /* HAVE_RP_PROPERTYSTORE_DEPS */
#ifdef ENABLE_OVERLAY_ICON_HANDLER
	lResult = RP_ShellIconOverlayIdentifer::UnregisterShellIconOverlayIdentifier();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
#endif /* ENABLE_OVERLAY_ICON_HANDLER */

	// Enumerate user hives.
	RegKey hku(HKEY_USERS, nullptr, KEY_READ, false);
	if (!hku.isOpen()) return SELFREG_E_CLASS;
	forward_list<tstring> user_SIDs;
	lResult = hku.enumSubKeys(user_SIDs);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	hku.close();

	// Don't check user hives with names that are 16 characters or shorter.
	// These are usually ".DEFAULT" or "well-known" SIDs.
	user_SIDs.remove_if(process_HKU_subkey);

	// Open HKEY_CLASSES_ROOT and HKEY_LOCAL_MACHINE.
	RegKey hkcr(HKEY_CLASSES_ROOT, nullptr, KEY_READ|KEY_WRITE, false);
	if (!hkcr.isOpen()) return SELFREG_E_CLASS;
	RegKey hklm(HKEY_LOCAL_MACHINE, nullptr, KEY_READ, false);
	if (!hklm.isOpen()) return SELFREG_E_CLASS;

	// Unregister all supported file types.
	const vector<RomDataFactory::ExtInfo> vec_exts = RomDataFactory::supportedFileExtensions();
	for (const auto &ext : vec_exts) {
		// Unregister the file type handlers for this file extension globally.
		lResult = UnregisterFileType(hkcr, &hklm, ext);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

		// Unregister user file types if necessary.
		for (const auto &sid : user_SIDs) {
			lResult = UnregisterUserFileType(sid, ext);
			if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		}
	}

	// Unregister RP_ShellPropSheetExt for all file types.
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, _T("*"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Unregister RP_ShellPropSheetExt for disk drives.
	// TODO: Icon/thumbnail handling?
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, _T("Drive"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Unregister RP_ShellPropSheetExt and thumbnailers for directories.
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, _T("Directory"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractIcon::UnregisterFileType(hkcr, _T("Directory"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::UnregisterFileType(hkcr, _T("Directory"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	// NOTE: IThumbnailProvider does not work on directories.
	// Unregistering it in case it was registered before.
	lResult = RP_ThumbnailProvider::UnregisterFileType(hkcr, _T("Directory"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Unregister RP_XAttrView for all file types.
	// TODO: Also for drives, if we add registration for it.
	lResult = RP_XAttrView::UnregisterFileType(hkcr, _T("*"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Unregister RP_ColumnProvider for "Folder".
	// It doesn't work for anything else, contrary to almost all documentation...
	// NOTE: "Folder" == file folder; "Directory" == *all* folders.
	// Reference: https://web.archive.org/web/20071213223408/https://www.codeproject.com/KB/shell/shellextguide8.aspx
	lResult = RP_ColumnProvider::UnregisterFileType(hkcr, _T("Folder"));
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Remove the ProgID.
	hkcr.deleteSubKey(RP_ProgID);

	/** Fixes for previous versions **/

	// NOTE: Some extensions were accidentally registered in previous versions:
	// - LibRomData::EXE: "*.vxd"
	// - LibRomData::MachO: ".dylib.bundle" [v1.4]
	// These extensions will be explicitly deleted here.
	// NOTE: Ignoring any errors to prevent `regsvr32` from failing.
	hkcr.deleteSubKey(_T("*.vxd"));
	hkcr.deleteSubKey(_T(".dylib.bundle"));

	// Unregister ourselves in any "HKCR\\Applications" entries,
	// and similarly for users. This was an error that caused
	// various brokenness with UserChoice on Windows 8+.
	RegKey hkcr_Applications(HKEY_CLASSES_ROOT, _T("Applications"), KEY_READ, false);
	if (hkcr_Applications.isOpen()) {
		UnregisterFromApplications(hkcr_Applications);
	}

	// Per-user versions of the above.
	tstring ts_regPath;
	for (const auto &sid : user_SIDs) {
		// Incorrect file extension registrations.
		ts_regPath = fmt::format(
			FSTR(_T("{:s}\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts")), sid);
		RegKey hkuvxd(HKEY_USERS, ts_regPath.c_str(), KEY_WRITE, false);
		if (hkuvxd.isOpen()) {
			hkuvxd.deleteSubKey(_T("*.vxd"));
			hkuvxd.deleteSubKey(_T(".dylib.bundle"));
		}

		// "HKU\\xxx\\SOFTWARE\\Classes\\Applications" entries
		ts_regPath = fmt::format(
			FSTR(_T("{:s}\\SOFTWARE\\Classes\\Applications")), sid);
		RegKey hku_Applications(HKEY_USERS, ts_regPath.c_str(), KEY_READ | KEY_WRITE, false);
		if (hku_Applications.isOpen()) {
			UnregisterFromApplications(hku_Applications);
		}
	}

	// Notify the shell that file associations have changed.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/shell/fa-file-types
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

	return S_OK;
}
