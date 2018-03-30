/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DllMain.cpp: DLL entry point and COM registration handler.              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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

#include "RP_ExtractIcon.hpp"
#include "RP_ClassFactory.hpp"
#include "RP_ExtractImage.hpp"
#include "RP_ShellPropSheetExt.hpp"
#include "RP_ThumbnailProvider.hpp"

// libwin32common
#include "libwin32common/ComBase.hpp"
#include "libwin32common/RegKey.hpp"
using LibWin32Common::RegKey;

// rp_image backend registration.
#include "librpbase/img/RpGdiplusBackend.hpp"
#include "librpbase/img/rp_image.hpp"
using LibRpBase::RpGdiplusBackend;
using LibRpBase::rp_image;

// Text conversion functions and macros.
#include "librpbase/TextFuncs.hpp"

// For file extensions.
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C++ includes.
#include <memory>
#include <string>
#include <vector>
#include <list>
using std::list;
using std::string;
using std::unique_ptr;
using std::vector;
using std::wstring;

extern wchar_t dll_filename[];
wchar_t dll_filename[MAX_PATH];

// Program ID for COM object registration.
extern const wchar_t RP_ProgID[];
const wchar_t RP_ProgID[] = L"rom-properties";

/**
 * DLL entry point.
 * @param hInstance
 * @param dwReason
 * @param lpReserved
 */
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH: {
			// Get the DLL filename.
			SetLastError(ERROR_SUCCESS);
			DWORD dwResult = GetModuleFileName(hInstance,
				dll_filename, ARRAY_SIZE(dll_filename));
			if (dwResult == 0 || GetLastError() != ERROR_SUCCESS) {
				// Cannot get the DLL filename.
				// TODO: Windows XP doesn't SetLastError() if the
				// filename is too big for the buffer.
				dll_filename[0] = 0;
				return FALSE;
			}

#if !defined(_MSC_VER) || defined(_DLL)
			// Disable thread library calls, since we don't
			// care about thread attachments.
			// NOTE: On MSVC, don't do this if using the static CRT.
			// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms682579(v=vs.85).aspx
			DisableThreadLibraryCalls(hInstance);
#endif /* !defined(_MSC_VER) || defined(_DLL) */

			// Register RpGdiplusBackend.
			// TODO: Static initializer somewhere?
			rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
			break;
		}

		case DLL_PROCESS_DETACH:
			// DLL is being unloaded.
			// TODO: Disable the COM server.
			break;

		default:
			break;
	}

	return TRUE;
}

/**
 * Can the DLL be unloaded?
 * @return S_OK if it can; S_FALSE if it can't.
 */
STDAPI DllCanUnloadNow(void)
{
	return (LibWin32Common::ComBase_isReferenced() ? S_FALSE : S_OK);
}

/**
 * Get a class factory to create an object of the requested type.
 * @param rclsid	[in] CLSID of the object.
 * @param riid		[in] IID_IClassFactory.
 * @param ppv		[out] Pointer that receives the interface pointer requested in riid.
 * @return Error code.
 */
STDAPI _Check_return_ DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
	if (!ppv) {
		// Incorrect parameters.
		return E_INVALIDARG;
	}

	// Clear the interface pointer initially.
	*ppv = nullptr;

	// Check for supported classes.
	HRESULT hr = E_FAIL;
#define CHECK_INTERFACE(Interface) \
	if (IsEqualIID(rclsid, CLSID_##Interface)) { \
		/* Create a new class factory for this Interface. */ \
		RP_ClassFactory<Interface> *pCF = new RP_ClassFactory<Interface>(); \
		hr = pCF->QueryInterface(riid, ppv); \
		pCF->Release(); \
	}
	CHECK_INTERFACE(RP_ExtractIcon)
	else CHECK_INTERFACE(RP_ExtractImage)
	else CHECK_INTERFACE(RP_ShellPropSheetExt)
	else CHECK_INTERFACE(RP_ThumbnailProvider)
	else {
		// Class not available.
		hr = CLASS_E_CLASSNOTAVAILABLE;
	}

	if (hr != S_OK) {
		// Interface not found.
		*ppv = nullptr;
	}
	return hr;
}

/**
 * Register file type handlers.
 * @param hkcr HKEY_CLASSES_ROOT, or user-specific Classes key.
 * @param extInfo RomDataFactory::ExtInfo
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG RegisterFileType(RegKey &hkcr, const RomDataFactory::ExtInfo &extInfo)
{
	LONG lResult;

	// Register the filetype in HKCR.
	wstring ext = U82W_c(extInfo.ext);
	RegKey *pHkey_fileType;
	lResult = RegKey::RegisterFileType(ext.c_str(), &pHkey_fileType);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// If the ProgID was previously set to RP_ProgID,
	// unset it, since we're not using it anymore.
	wstring progID = pHkey_fileType->read(nullptr);
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
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	if (extInfo.hasThumbnail) {
		// Register the thumbnail handlers.
		lResult = RP_ExtractIcon::RegisterFileType(hkcr, ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ExtractImage::RegisterFileType(hkcr, ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ThumbnailProvider::RegisterFileType(hkcr, ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	} else {
		// No thumbnail handlers.
		// Unregister the handlers if they were previously registered.
		lResult = RP_ExtractIcon::UnregisterFileType(hkcr, ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ExtractImage::UnregisterFileType(hkcr, ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ThumbnailProvider::UnregisterFileType(hkcr, ext.c_str());
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}

	// All file type handlers registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister file type handlers.
 * @param hkcr HKEY_CLASSES_ROOT, or user-specific Classes key.
 * @param extInfo RomDataFactory::ExtInfo
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG UnregisterFileType(RegKey &hkcr, const RomDataFactory::ExtInfo &extInfo)
{
	LONG lResult;

	// Open the file type key if it's present.
	wstring ext = U82W_c(extInfo.ext);
	RegKey hkey_fileType(hkcr, ext.c_str(), KEY_READ|KEY_WRITE, false);
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
	wstring progID = hkey_fileType.read(nullptr);
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
	lResult = RP_ExtractIcon::UnregisterFileType(hkcr, ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::UnregisterFileType(hkcr, ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ThumbnailProvider::UnregisterFileType(hkcr, ext.c_str());
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Delete keys if they're empty.
	static const wchar_t *const keysToDel[] = {L"ShellEx", L"RP_Fallback"};
	for (int i = ARRAY_SIZE(keysToDel)-1; i >= 0; i--) {
		// Check if the key is empty.
		RegKey hkey_del(hkey_fileType, keysToDel[i], KEY_READ, false);
		if (!hkey_del.isOpen())
			continue;

		// Check if the key is empty.
		// TODO: Error handling.
		if (hkey_del.isKeyEmpty()) {
			// No subkeys. Delete this key.
			hkey_del.close();
			hkey_fileType.deleteSubKey(keysToDel[i]);
		}
	}

	// Is a custom ProgID registered?
	// If so, we should check for empty keys there, too.
	if (!progID.empty()) {
		// Custom ProgID is registered.
		RegKey hkey_ProgID(hkcr, progID.c_str(), KEY_READ|KEY_WRITE, false);
		for (int i = ARRAY_SIZE(keysToDel)-1; i >= 0; i--) {
			// Check if the key is empty.
			RegKey hkey_del(hkey_ProgID, keysToDel[i], KEY_READ, false);
			if (!hkey_del.isOpen())
				continue;

			// Check if the key is empty.
			// TODO: Error handling.
			if (hkey_del.isKeyEmpty()) {
				// No subkeys. Delete this key.
				hkey_del.close();
				hkey_ProgID.deleteSubKey(keysToDel[i]);
			}
		}
	}

	// All file type handlers unregistered.
	return ERROR_SUCCESS;
}

/**
 * Get the user's overriden file association for the given file extension.
 * @param sid User SID.
 * @param ext File extension.
 * @return Overridden file association ProgID, or empty string if none.
 */
static wstring GetUserFileAssoc(const wstring &sid, const char *ext)
{
	// Check if the user has already associated this file extension.
	// TODO: Check all users.

	// 288-character buffer:
	// - SID length: 184 characters
	//   - Reference: http://stackoverflow.com/questions/1140528/what-is-the-maximum-length-of-a-sid-in-sddl-format
	// - FileExts: 61 characters
	// - Extension: 16 characters
	// - UserChoice: 11 characters
	// - Extra: 16 characters
	wchar_t regPath[288];
	int len = swprintf_s(regPath, ARRAY_SIZE(regPath),
		L"%s\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s\\UserChoice",
		sid.c_str(), U82W_c(ext));
	if (len <= 0 || len >= ARRAY_SIZE(regPath)) {
		// Buffer isn't large enough...
		return wstring();
	}

	// FIXME: This will NOT update profiles that aren't loaded.
	// Other profiles will need to be loaded manually, or those users
	// will have to register the DLL themselves.
	// Reference: http://windowsitpro.com/scripting/how-can-i-update-all-profiles-machine-even-if-theyre-not-currently-loaded
	RegKey hkcu_UserChoice(HKEY_USERS, regPath, KEY_READ, false);
	if (!hkcu_UserChoice.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hkcu_UserChoice.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return wstring();
		}
		// TODO: Return an error.
		//return hkcu_UserChoice.lOpenRes();
		return wstring();
	}

	// Read the user's choice.
	return hkcu_UserChoice.read(L"Progid");
}

/**
 * Register file type handlers for a user's overridden file association.
 * @param sid User SID.
 * @param ext_info File extension information.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG RegisterUserFileType(const wstring &sid, const RomDataFactory::ExtInfo &ext_info)
{
	// Get the ProgID.
	wstring progID = GetUserFileAssoc(sid, ext_info.ext);
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
	string progID_u8 = W2U8(progID);
	const RomDataFactory::ExtInfo progID_info = {
		progID_u8.c_str(),
		ext_info.hasThumbnail
	};

	// Does HKCR\\progID exist?
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_WRITE, false);
	if (hkcr_ProgID.isOpen()) {
		LONG lResult = RegisterFileType(hkcr, progID_info);
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

	// 208-character buffer:
	// - SID length: 184 characters
	//   - Reference: http://stackoverflow.com/questions/1140528/what-is-the-maximum-length-of-a-sid-in-sddl-format
	// - Software\\Classes: 16 characters
	// - Extra: 8 characters
	wchar_t regPath[208];
	int len = swprintf_s(regPath, ARRAY_SIZE(regPath),
		L"%s\\Software\\Classes", sid.c_str());
	if (len <= 0 || len >= ARRAY_SIZE(regPath)) {
		// Buffer isn't large enough...
		return ERROR_BUFFER_OVERFLOW;
	}

	RegKey hku_cr(HKEY_USERS, regPath, KEY_WRITE, false);
	if (hku_cr.isOpen()) {
		LONG lResult = RegisterFileType(hku_cr, progID_info);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hku_cr.lOpenRes() != ERROR_FILE_NOT_FOUND) {
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
static LONG UnregisterUserFileType(const wstring &sid, const RomDataFactory::ExtInfo &ext_info)
{
	// Get the ProgID.
	wstring progID = GetUserFileAssoc(sid, ext_info.ext);
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
	string progID_u8 = W2U8(progID);
	const RomDataFactory::ExtInfo progID_info = {
		progID_u8.c_str(),
		ext_info.hasThumbnail
	};

	// Does HKCR\\progID exist?
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_WRITE, false);
	if (hkcr_ProgID.isOpen()) {
		LONG lResult = UnregisterFileType(hkcr, progID_info);
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

	// 208-character buffer:
	// - SID length: 184 characters
	//   - Reference: http://stackoverflow.com/questions/1140528/what-is-the-maximum-length-of-a-sid-in-sddl-format
	// - Software\\Classes: 16 characters
	// - Extra: 8 characters
	wchar_t regPath[208];
	int len = swprintf_s(regPath, ARRAY_SIZE(regPath),
		L"%s\\Software\\Classes", sid.c_str());
	if (len <= 0 || len >= ARRAY_SIZE(regPath)) {
		// Buffer isn't large enough...
		return ERROR_BUFFER_OVERFLOW;
	}

	RegKey hku_cr(HKEY_USERS, regPath, KEY_WRITE, false);
	if (hku_cr.isOpen()) {
		LONG lResult = UnregisterFileType(hku_cr, progID_info);
		if (lResult != ERROR_SUCCESS) {
			return lResult;
		}
	} else {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hku_cr.lOpenRes() != ERROR_FILE_NOT_FOUND) {
			return hku_cr.lOpenRes();
		}
	}

	return ERROR_SUCCESS;
}

/**
 * Remove HKEY_USERS subkeys from an std::list if we don't want to process them.
 * @param subKey Subkey name.
 * @return True to remove; false to keep.
 */
static inline bool process_HKU_subkey(const wstring& subKey)
{
	if (subKey.size() <= 16) {
		// Subkey name is too small.
		return true;
	}

	// Ignore "_Classes" subkeys.
	// These are virtual subkeys that map to:
	// HKEY_USERS\\[sid]\\Software\\Classes
	const wchar_t *const classes_pos = subKey.c_str() + (subKey.size() - 8);
	return (!wcsicmp(classes_pos, L"_Classes"));
}

/**
 * Register the DLL.
 */
STDAPI DllRegisterServer(void)
{
	LONG lResult;

	// Register the COM objects.
	lResult = RP_ExtractIcon::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ShellPropSheetExt::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ThumbnailProvider::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Enumerate user hives.
	RegKey hku(HKEY_USERS, nullptr, KEY_READ, false);
	if (!hku.isOpen()) return SELFREG_E_CLASS;
	list<wstring> user_SIDs;
	lResult = hku.enumSubKeys(user_SIDs);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	hku.close();

	// Don't check user hives with names that are 16 characters or shorter.
	// These are usually ".DEFAULT" or "well-known" SIDs.
	user_SIDs.remove_if(process_HKU_subkey);

	// Open HKEY_CLASSES_ROOT.
	RegKey hkcr(HKEY_CLASSES_ROOT, nullptr, KEY_READ|KEY_WRITE, false);
	if (!hkcr.isOpen()) return SELFREG_E_CLASS;

	// Register all supported file types and associate them
	// with our ProgID.
	vector<RomDataFactory::ExtInfo> vec_exts = RomDataFactory::supportedFileExtensions();
	for (auto ext_iter = vec_exts.cbegin(); ext_iter != vec_exts.cend(); ++ext_iter) {
		// Register the file type handlers for this file extension globally.
		lResult = RegisterFileType(hkcr, *ext_iter);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

		// Register user file types if necessary.
		for (auto sid_iter = user_SIDs.cbegin(); sid_iter != user_SIDs.cend(); ++sid_iter) {
			lResult = RegisterUserFileType(*sid_iter, *ext_iter);
			if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		}
	}

	// NOTE: "*.vxd" was accidentally used in LibRomData::EXE.
	// Manually deleting it here.
	lResult = hkcr.deleteSubKey(L"*.vxd");
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return SELFREG_E_CLASS;
	for (auto sid_iter = user_SIDs.cbegin(); sid_iter != user_SIDs.cend(); ++sid_iter) {
		wchar_t regPath[288];
		int len = swprintf_s(regPath, ARRAY_SIZE(regPath),
			L"%s\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts",
			sid_iter->c_str());
		if (len <= 0 || len >= ARRAY_SIZE(regPath)) {
			// Buffer isn't large enough...
			continue;
		}

		RegKey hku(HKEY_USERS, regPath, KEY_WRITE, false);
		if (!hku.isOpen()) return SELFREG_E_CLASS;
		lResult = hku.deleteSubKey(L"*.vxd");
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
			return SELFREG_E_CLASS;
	}

	// Register RP_ShellPropSheetExt for all file types.
	// Fixes an issue where it doesn't show up for .dds if
	// Visual Studio 2017 is installed.
	lResult = RP_ShellPropSheetExt::RegisterFileType(hkcr, L"*");
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Register RP_ShellPropSheetExt for disk drives.
	// TODO: Icon/thumbnail handling?
	lResult = RP_ShellPropSheetExt::RegisterFileType(hkcr, L"Drive");
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Notify the shell that file associations have changed.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144148(v=vs.85).aspx
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

	return S_OK;
}

/**
 * Unregister the DLL.
 */
STDAPI DllUnregisterServer(void)
{
	// Unregister the COM objects.
	LONG lResult = RP_ExtractIcon::UnregisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::UnregisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ShellPropSheetExt::UnregisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ThumbnailProvider::UnregisterCLSID();
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Enumerate user hives.
	RegKey hku(HKEY_USERS, nullptr, KEY_READ, false);
	if (!hku.isOpen()) return SELFREG_E_CLASS;
	list<wstring> user_SIDs;
	lResult = hku.enumSubKeys(user_SIDs);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	hku.close();

	// Don't check user hives with names that are 16 characters or shorter.
	// These are usually ".DEFAULT" or "well-known" SIDs.
	user_SIDs.remove_if(process_HKU_subkey);

	// Open HKEY_CLASSES_ROOT.
	RegKey hkcr(HKEY_CLASSES_ROOT, nullptr, KEY_READ|KEY_WRITE, false);
	if (!hkcr.isOpen()) return SELFREG_E_CLASS;

	// Unegister all supported file types.
	vector<RomDataFactory::ExtInfo> vec_exts = RomDataFactory::supportedFileExtensions();
	for (auto ext_iter = vec_exts.cbegin(); ext_iter != vec_exts.cend(); ++ext_iter) {
		// Unregister the file type handlers for this file extension globally.
		lResult = UnregisterFileType(hkcr, *ext_iter);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

		// Unregister user file types if necessary.
		for (auto sid_iter = user_SIDs.cbegin(); sid_iter != user_SIDs.cend(); ++sid_iter) {
			lResult = UnregisterUserFileType(*sid_iter, *ext_iter);
			if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		}
	}

	// NOTE: "*.vxd" was accidentally used in LibRomData::EXE.
	// Manually deleting it here.
	lResult = hkcr.deleteSubKey(L"*.vxd");
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return SELFREG_E_CLASS;
	for (auto sid_iter = user_SIDs.cbegin(); sid_iter != user_SIDs.cend(); ++sid_iter) {
		wchar_t regPath[288];
		int len = swprintf_s(regPath, ARRAY_SIZE(regPath),
			L"%s\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts",
			sid_iter->c_str());
		if (len <= 0 || len >= ARRAY_SIZE(regPath)) {
			// Buffer isn't large enough...
			continue;
		}

		RegKey hku(HKEY_USERS, regPath, KEY_WRITE, false);
		if (!hku.isOpen()) return SELFREG_E_CLASS;
		lResult = hku.deleteSubKey(L"*.vxd");
		if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
			return SELFREG_E_CLASS;
	}

	// Unregister RP_ShellPropSheetExt for all file types.
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, L"*");
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Unregister RP_ShellPropSheetExt for disk drives.
	// TODO: Icon/thumbnail handling?
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkcr, L"Drive");
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Notify the shell that file associations have changed.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144148(v=vs.85).aspx
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

	return S_OK;
}

/**
 * Get the DLL version.
 * Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb776404(v=vs.85).aspx
 */
STDAPI DllGetVersion(_Out_ DLLVERSIONINFO2 *pdvi)
{
	if (!pdvi) {
		// Return E_POINTER since pdvi is an out param.
		// Reference: http://stackoverflow.com/questions/1426672/when-return-e-pointer-and-when-e-invalidarg
		return E_POINTER;
	}

	if (pdvi->info1.cbSize < sizeof(DLLVERSIONINFO)) {
		// Invalid struct...
		return E_INVALIDARG;
	}

	// DLLVERSIONINFO
	pdvi->info1.dwMajorVersion = RP_VERSION_MAJOR;
	pdvi->info1.dwMinorVersion = RP_VERSION_MINOR;
	pdvi->info1.dwBuildNumber = RP_VERSION_PATCH;	// Not technically a build number...
	pdvi->info1.dwPlatformID = DLLVER_PLATFORM_NT;	// TODO: 9x?

	if (pdvi->info1.cbSize >= sizeof(DLLVERSIONINFO2)) {
		// DLLVERSIONINFO2
		pdvi->dwFlags = 0;
		pdvi->ullVersion = MAKEDLLVERULL(
			RP_VERSION_MAJOR, RP_VERSION_MINOR,
			RP_VERSION_PATCH, RP_VERSION_DEVEL);
	}

	return S_OK;
}
