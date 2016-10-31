/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DllMain.cpp: DLL entry point and COM registration handler.              *
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
#include "RegKey.hpp"
#include "RP_ComBase.hpp"
#include "RP_ExtractIcon.hpp"
#include "RP_ClassFactory.hpp"
#include "RP_ExtractImage.hpp"
#include "RP_ShellPropSheetExt.hpp"
#include "RP_ThumbnailProvider.hpp"

// For file extensions.
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// rp_image backend registration.
#include "libromdata/img/RpGdiplusBackend.hpp"
#include "libromdata/img/rp_image.hpp"
using LibRomData::RpGdiplusBackend;
using LibRomData::rp_image;

// C++ includes.
#include <memory>
#include <string>
#include <vector>
#include <list>
using std::list;
using std::unique_ptr;
using std::vector;
using std::wstring;

extern HINSTANCE g_hInstance;
HINSTANCE g_hInstance = nullptr;
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
			// DLL loaded by a process.
			g_hInstance = hInstance;

			// Get the DLL filename.
			DWORD dwResult = GetModuleFileName(g_hInstance,
				dll_filename, sizeof(dll_filename)/sizeof(dll_filename[0]));
			if (dwResult == 0) {
				// FIXME: Handle this.
				dll_filename[0] = 0;
			}

			// Disable thread library calls, since we don't
			// care about thread attachments.
			DisableThreadLibraryCalls(hInstance);

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
	return (RP_ComBase_isReferenced() ? S_OK : S_FALSE);
}

/**
 * Get a class factory to create an object of the requested type.
 * @param rclsid	[in] CLSID of the object.
 * @param riid		[in] IID_IClassFactory.
 * @param ppv		[out] Pointer that receives the interface pointer requested in riid.
 * @return Error code.
 */
__checkReturn
STDAPI DllGetClassObject(__in REFCLSID rclsid, __in REFIID riid, __deref_out LPVOID FAR* ppv)
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
 * @param hkey_Assoc File association key to register under.
 * @param hasThumbnail If true, associate thumbnail handlers.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG RegisterFileType(RegKey &hkey_Assoc, bool hasThumbnail)
{
	LONG lResult;

	lResult = RP_ShellPropSheetExt::RegisterFileType(hkey_Assoc);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	if (hasThumbnail) {
		lResult = RP_ExtractIcon::RegisterFileType(hkey_Assoc);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ExtractImage::RegisterFileType(hkey_Assoc);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ThumbnailProvider::RegisterFileType(hkey_Assoc);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	} else {
		// No thumbnail handlers.
		// Unregister the handlers if they were previously registered.
		lResult = RP_ExtractIcon::UnregisterFileType(hkey_Assoc);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ExtractImage::UnregisterFileType(hkey_Assoc);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		lResult = RP_ThumbnailProvider::UnregisterFileType(hkey_Assoc);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}

	// All file types handlers registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister file type handlers.
 * @param hkey_Assoc File association key to register under.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG UnregisterFileType(RegKey &hkey_Assoc)
{
	LONG lResult;

	lResult = RP_ExtractIcon::UnregisterFileType(hkey_Assoc);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::UnregisterFileType(hkey_Assoc);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ShellPropSheetExt::UnregisterFileType(hkey_Assoc);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	lResult = RP_ThumbnailProvider::UnregisterFileType(hkey_Assoc);
	if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;

	// Check if the "ShellEx" key is empty.
	RegKey hkey_ShellEx(hkey_Assoc, L"ShellEx", KEY_READ, false);
	if (!hkey_ShellEx.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable here.
		if (hkey_ShellEx.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkey_ShellEx.lOpenRes();
	}

	// Check if ShellEx is empty.
	// TODO: Error handling.
	if (hkey_ShellEx.isKeyEmpty()) {
		// No subkeys. Delete this key.
		hkey_ShellEx.close();
		hkey_Assoc.deleteSubKey(L"ShellEx");
	}

	// All file types handlers registered.
	return ERROR_SUCCESS;
}

/**
 * Get the user's overriden file association for the given file extension.
 * @param sid User SID.
 * @param ext File extension.
 * @return Overridden file association ProgID, or empty string if none.
 */
static wstring GetUserFileAssoc(const wstring &sid, const rp_char *ext)
{
	// Check if the user has already associated this file extension.
	// TODO: Check all users.
	wstring regPath;
	regPath.reserve(128);
	regPath  = sid;
	regPath += L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\";
	regPath += RP2W_c(ext);
	regPath += L"\\UserChoice";

	// FIXME: This will NOT update profiles that aren't loaded.
	// Other profiles will need to be loaded manually, or those users
	// will have to register the DLL themselves.
	// Reference: http://windowsitpro.com/scripting/how-can-i-update-all-profiles-machine-even-if-theyre-not-currently-loaded
	RegKey hkcu_UserChoice(HKEY_USERS, regPath.c_str(), KEY_READ, false);
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
	LONG lResult;

	// First, check HKCR.
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_WRITE, false);
	if (!hkcr_ProgID.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hkcr_ProgID.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ProgID.lOpenRes();
	}
	lResult = RegisterFileType(hkcr_ProgID, ext_info.hasThumbnail);
	if (lResult != ERROR_SUCCESS) {
		return lResult;
	}

	// Next, check "HKU\\[sid]".
	wstring regPath;
	regPath.reserve(128);
	regPath  = sid;
	regPath += L"\\Software\\Classes\\";
	regPath += progID;
	RegKey hku_ProgID(HKEY_USERS, regPath.c_str(), KEY_WRITE, false);
	if (!hku_ProgID.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hku_ProgID.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hku_ProgID.lOpenRes();
	}
	return RegisterFileType(hku_ProgID, ext_info.hasThumbnail);
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
	LONG lResult;

	// First, check HKCR.
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_WRITE, false);
	if (!hkcr_ProgID.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hkcr_ProgID.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hkcr_ProgID.lOpenRes();
	}
	lResult = UnregisterFileType(hkcr_ProgID);
	if (lResult != ERROR_SUCCESS) return lResult;

	// Next, check "HKU\\[sid]".
	wstring regPath;
	regPath.reserve(128);
	regPath  = sid;
	regPath += L"\\Software\\Classes\\";
	regPath += progID;
	RegKey hku_ProgID(HKEY_USERS, regPath.c_str(), KEY_WRITE, false);
	if (!hku_ProgID.isOpen()) {
		// ERROR_FILE_NOT_FOUND is acceptable.
		// Anything else is an error.
		if (hku_ProgID.lOpenRes() == ERROR_FILE_NOT_FOUND) {
			return ERROR_SUCCESS;
		}
		return hku_ProgID.lOpenRes();
	}
	return UnregisterFileType(hku_ProgID);
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

	// Register all supported file types and associate them
	// with our ProgID.
	vector<RomDataFactory::ExtInfo> vec_exts = RomDataFactory::supportedFileExtensions();
	for (auto ext_iter = vec_exts.cbegin(); ext_iter != vec_exts.cend(); ++ext_iter) {
		// Register user file types if necessary.
		for (auto sid_iter = user_SIDs.cbegin(); sid_iter != user_SIDs.cend(); ++sid_iter) {
			lResult = RegisterUserFileType(*sid_iter, *ext_iter);
			if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		}

		// Register the filetype in KEY_CLASSES_ROOT and use it
		// to register the handlers.
		RegKey *pHkey_fileType;
		lResult = RegKey::RegisterFileType(RP2W_c(ext_iter->ext), &pHkey_fileType);
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

		// Register the file type handlers for this file extension.
		lResult = RegisterFileType(*pHkey_fileType, ext_iter->hasThumbnail);
		delete pHkey_fileType;
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}

	// Delete the rom-properties ProgID,
	// since it's no longer used.
	lResult = RegKey::deleteSubKey(HKEY_CLASSES_ROOT, RP_ProgID);
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
		// Error deleting the ProgID.
		return SELFREG_E_CLASS;
	}

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

	// Register all supported file types and associate them
	// with our ProgID.
	vector<RomDataFactory::ExtInfo> vec_exts = RomDataFactory::supportedFileExtensions();
	for (auto ext_iter = vec_exts.cbegin(); ext_iter != vec_exts.cend(); ++ext_iter) {
		// Unregister user file types if necessary.
		for (auto sid_iter = user_SIDs.cbegin(); sid_iter != user_SIDs.cend(); ++sid_iter) {
			lResult = UnregisterUserFileType(*sid_iter, *ext_iter);
			if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
		}

		// Open the file type key if it's present.
		RegKey hkey_fileType(HKEY_CLASSES_ROOT, RP2W_c(ext_iter->ext), KEY_READ | KEY_WRITE, false);
		if (!hkey_fileType.isOpen()) {
			// Not open...
			if (hkey_fileType.lOpenRes() == ERROR_FILE_NOT_FOUND) {
				// Key not found. Continue.
				continue;
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
				return SELFREG_E_CLASS;
			}
		}

		// Unregister the file type handlers for this file extension.
		lResult = UnregisterFileType(hkey_fileType);
		if (lResult != ERROR_SUCCESS) return SELFREG_E_CLASS;
	}

	// Delete the rom-properties ProgID,
	// since it's no longer used.
	lResult = RegKey::deleteSubKey(HKEY_CLASSES_ROOT, RP_ProgID);
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND) {
		// Error deleting the ProgID.
		return SELFREG_E_CLASS;
	}

	// Notify the shell that file associations have changed.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144148(v=vs.85).aspx
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

	return S_OK;
}
