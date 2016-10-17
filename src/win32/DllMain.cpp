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
#include <string>
#include <vector>
using std::wstring;
using std::vector;

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
 * @param rclsid CLSID of the object.
 * @param riid IID_IClassFactory.
 * @param ppvOut Pointer that receives the interface pointer requested in riid.
 * @return Error code.
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
	*ppvOut = nullptr;

	// Check for supported classes.
	HRESULT hr = E_FAIL;
	if (IsEqualIID(rclsid, CLSID_RP_ExtractIcon)) {
		// Create a new class factory for RP_ExtractIcon.
		RP_ClassFactory<RP_ExtractIcon> *pCF = new RP_ClassFactory<RP_ExtractIcon>();
		hr = pCF->QueryInterface(riid, ppvOut);
		pCF->Release();
	} else if (IsEqualIID(rclsid, CLSID_RP_ExtractImage)) {
		// Create a new class factory for RP_ExtractImage.
		RP_ClassFactory<RP_ExtractImage> *pCF = new RP_ClassFactory<RP_ExtractImage>();
		hr = pCF->QueryInterface(riid, ppvOut);
		pCF->Release();
	} else if (IsEqualIID(rclsid, CLSID_RP_ShellPropSheetExt)) {
		// Create a new class factory for RP_ShellPropSheetExt.
		RP_ClassFactory<RP_ShellPropSheetExt> *pCF = new RP_ClassFactory<RP_ShellPropSheetExt>();
		hr = pCF->QueryInterface(riid, ppvOut);
		pCF->Release();
	} else if (IsEqualIID(rclsid, CLSID_RP_ThumbnailProvider)) {
		// Create a new class factory for RP_ThumbnailProvider.
		RP_ClassFactory<RP_ThumbnailProvider> *pCF = new RP_ClassFactory<RP_ThumbnailProvider>();
		hr = pCF->QueryInterface(riid, ppvOut);
		pCF->Release();
	} else {
		// Class not available.
		hr = CLASS_E_CLASSNOTAVAILABLE;
	}

	if (hr != S_OK) {
		// Interface not found.
		*ppvOut = nullptr;
	}
	return hr;
}

/**
 * Register file type handlers.
 * @param progID ProgID to register under, or nullptr for the default.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
static LONG RegisterFileType(LPCWSTR progID = nullptr)
{
	LONG lResult;

	lResult = RP_ExtractIcon::RegisterFileType(progID);
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}

	lResult = RP_ExtractImage::RegisterFileType(progID);
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}

	lResult = RP_ShellPropSheetExt::RegisterFileType(progID);
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}

	lResult = RP_ThumbnailProvider::RegisterFileType(progID);
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}

	// All file types handlers registered.
	return ERROR_SUCCESS;
}

/**
 * Register the DLL.
 */
STDAPI DllRegisterServer(void)
{
	LONG lResult;

	// Register the COM objects.
	lResult = RP_ExtractIcon::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}
	lResult = RP_ExtractImage::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}
	lResult = RP_ShellPropSheetExt::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}
	lResult = RP_ThumbnailProvider::RegisterCLSID();
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}

	// Register file type information with the default ProgID.
	lResult = RegisterFileType(nullptr);
	if (lResult != ERROR_SUCCESS) {
		return SELFREG_E_CLASS;
	}

	// Register all supported file types and associate them
	// with our ProgID.
	vector<const rp_char*> vec_exts = RomDataFactory::supportedFileExtensions();
	for (vector<const rp_char*>::const_iterator iter = vec_exts.begin();
	     iter != vec_exts.end(); ++iter)
	{
		// Check if the user has already associated this file extension.
		// TODO: Check all users.
		wstring regPath;
		regPath.reserve(128);
		regPath  = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\";
		regPath += RP2W_c(*iter);
		regPath += L"\\UserChoice";

		RegKey hkcu_UserChoice(HKEY_CURRENT_USER, regPath.c_str(), KEY_READ, false);
		if (hkcu_UserChoice.isOpen()) {
			// Read the user's choice.
			wstring progID = hkcu_UserChoice.read(L"Progid");
			if (!progID.empty()) {
				// Register the file type handlers under this ProgID.
				lResult = RegisterFileType(progID.c_str());
				if (lResult != ERROR_SUCCESS) {
					return SELFREG_E_CLASS;
				}
			}
		}

		// NOTE: Assuming rp_char is UTF-16.
		static_assert(sizeof(rp_char) == sizeof(wchar_t), "rp_char != wchar_t");
		const wchar_t *filetype = reinterpret_cast<const wchar_t*>(*iter);
		lResult = RegKey::RegisterFileType(filetype, RP_ProgID);
		if (lResult != ERROR_SUCCESS)
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
	if (lResult != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	lResult = RP_ExtractImage::UnregisterCLSID();
	if (lResult != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	lResult = RP_ShellPropSheetExt::UnregisterCLSID();
	if (lResult != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	lResult = RP_ThumbnailProvider::UnregisterCLSID();
	if (lResult != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	// NOTE: Do NOT unregister file types.
	// MSDN says to leave file type mappings unchanged when uninstalling.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/hh127451(v=vs.85).aspx

	// Delete the rom-properties ProgID.
	lResult = RegKey::deleteSubKey(HKEY_CLASSES_ROOT, RP_ProgID);
	if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
		return lResult;

	// Notify the shell that file associations have changed.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144148(v=vs.85).aspx
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

	return S_OK;
}
