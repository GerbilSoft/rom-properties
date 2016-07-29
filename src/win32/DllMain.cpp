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
#include "RP_ComBase.hpp"
#include "RP_ExtractIcon.hpp"
#include "RP_ClassFactory.hpp"

static HINSTANCE g_hInstance = nullptr;

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
		case DLL_PROCESS_ATTACH:
			// DLL loaded by a process.
			g_hInstance = hInstance;

			// Disable thread library calls, since we don't
			// care about thread attachments.
			DisableThreadLibraryCalls(hInstance);
			break;

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

	// Check for RP_ExtractIcon.
	if (IsEqualIID(rclsid, CLSID_RP_ExtractIcon)) {
		// Create a new class factory.
		RP_ClassFactory<RP_ExtractIcon> *pCF = new RP_ClassFactory<RP_ExtractIcon>();
		HRESULT hr = pCF->QueryInterface(riid, ppvOut);
		if (hr != S_OK) {
			// Interface not found.
			*ppvOut = nullptr;	// TODO: Not needed?
			delete pCF;
		}
		return hr;
	}

	// Class not available.
	return CLASS_E_CLASSNOTAVAILABLE;
}

/**
 * Register the DLL.
 */
STDAPI DllRegisterServer(void)
{
	// Register this DLL as an icon handler for ".nds".
	// TODO: Add helper functions for registry access.

	// Create/open the ".nds" key.
	HKEY hKeyCR_nds;
	LONG ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, L".nds", 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKeyCR_nds, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open the ".nds" key.
		return SELFREG_E_CLASS;
	}

	// Set the default value to "rom-properties".
	static const wchar_t ProgID[] = L"rom-properties";
	ret = RegSetValueEx(hKeyCR_nds, nullptr, 0, REG_SZ,
		(const BYTE*)ProgID, sizeof(ProgID));
	if (ret != ERROR_SUCCESS) {
		// Unable to set the default value.
		RegCloseKey(hKeyCR_nds);
		return SELFREG_E_CLASS;
	}

	// Close the keys.
	RegCloseKey(hKeyCR_nds);

	// Create/open the ProgID key.
	HKEY hKeyCR_ProgID;
	ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, ProgID, 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKeyCR_ProgID, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open the ProgID key.
		return SELFREG_E_CLASS;
	}

	// Create/open the "ShellEx" key.
	HKEY hKey_ShellEx;
	ret = RegCreateKeyEx(hKeyCR_ProgID, L"ShellEx", 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKey_ShellEx, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open the "ShellEx" key.
		RegCloseKey(hKeyCR_ProgID);
		return SELFREG_E_CLASS;
	}

	// Create/open the "IconHandler" key.
	HKEY hKey_IconHandler;
	ret = RegCreateKeyEx(hKey_ShellEx, L"IconHandler", 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKey_IconHandler, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open the "IconHandler" key.
		RegCloseKey(hKey_ShellEx);
		RegCloseKey(hKeyCR_ProgID);
		return SELFREG_E_CLASS;
	}

	// Set the default value to our CLSID.
	ret = RegSetValueEx(hKey_IconHandler, nullptr, 0, REG_SZ,
		(const BYTE*)CLSID_RP_ExtractIcon_Str,
		(wcslen(CLSID_RP_ExtractIcon_Str)+1)*sizeof(wchar_t));
	if (ret != ERROR_SUCCESS) {
		// Unable to set the default value.
		RegCloseKey(hKey_IconHandler);
		RegCloseKey(hKey_ShellEx);
		RegCloseKey(hKeyCR_ProgID);
		return SELFREG_E_CLASS;
	}

	RegCloseKey(hKey_IconHandler);
	RegCloseKey(hKey_ShellEx);

	// Create/open the "DefaultIcon" key.
	HKEY hKey_DefaultIcon;
	ret = RegCreateKeyEx(hKeyCR_ProgID, L"DefaultIcon", 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKey_DefaultIcon, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open the "DefaultIcon" key.
		RegCloseKey(hKeyCR_ProgID);
		return SELFREG_E_CLASS;
	}

	// Set the default value to "%1".
	ret = RegSetValueEx(hKey_DefaultIcon, nullptr, 0, REG_SZ,
		(const BYTE*)L"%1", 3*sizeof(wchar_t));
	if (ret != ERROR_SUCCESS) {
		// Unable to set the default value.
		RegCloseKey(hKey_DefaultIcon);
		RegCloseKey(hKeyCR_ProgID);
		return SELFREG_E_CLASS;
	}

	// Close the registry keys.
	RegCloseKey(hKey_DefaultIcon);
	RegCloseKey(hKeyCR_ProgID);

	/** Register the DLL itself. **/

	// Open HKCR\CLSID.
	HKEY hKeyCLSID;
	ret = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_WRITE, &hKeyCLSID);
	if (ret != ERROR_SUCCESS)
	{
		// Unable to open the "CLSID" key.
		return SELFREG_E_CLASS;
	}

	// Create a key using the CLSID.
	HKEY hKey_OurCLSID;
	ret = RegCreateKeyEx(hKeyCLSID, CLSID_RP_ExtractIcon_Str, 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKey_OurCLSID, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open our CLSID key.
		RegCloseKey(hKeyCLSID);
		return SELFREG_E_CLASS;
	}

	// Set the name of the key.
	ret = RegSetValueEx(hKey_OurCLSID, nullptr, 0, REG_SZ,
		(const BYTE*)ProgID, sizeof(ProgID));
	if (ret != ERROR_SUCCESS) {
		// Unable to set the default value.
		RegCloseKey(hKey_OurCLSID);
		RegCloseKey(hKeyCLSID);
		return SELFREG_E_CLASS;
	}

	// Create an InprocServer32 subkey.
	HKEY hKey_InprocServer32;
	ret = RegCreateKeyEx(hKey_OurCLSID, L"InprocServer32", 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKey_InprocServer32, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open the InprocServer32 subkey.
		RegCloseKey(hKey_OurCLSID);
		RegCloseKey(hKeyCLSID);
		return SELFREG_E_CLASS;
	}

	// Set the default value to the DLL filename.
	wchar_t dll_filename[MAX_PATH];
	int dll_filename_len = GetModuleFileName(g_hInstance, dll_filename, sizeof(dll_filename)/sizeof(dll_filename[0]));
	// TODO: Handle buffer size errors.
	if (dll_filename_len > 0) {
		ret = RegSetValueEx(hKey_InprocServer32, nullptr, 0, REG_SZ,
			(const BYTE*)dll_filename, (dll_filename_len+1)*sizeof(dll_filename[0]));
		if (ret != ERROR_SUCCESS) {
			// Unable to set the default value.
			RegCloseKey(hKey_InprocServer32);
			RegCloseKey(hKey_OurCLSID);
			RegCloseKey(hKeyCLSID);
			return SELFREG_E_CLASS;
		}
	}

	// Set the threading model to Apartment.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144110%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
	ret = RegSetValueEx(hKey_InprocServer32, L"ThreadingModel", 0, REG_SZ,
		(const BYTE*)L"Apartment", 10*sizeof(wchar_t));
	if (ret != ERROR_SUCCESS) {
		// Unable to set the threading model value.
		RegCloseKey(hKey_InprocServer32);
		RegCloseKey(hKey_OurCLSID);
		RegCloseKey(hKeyCLSID);
		return SELFREG_E_CLASS;
	}

	RegCloseKey(hKey_InprocServer32);

	// Create a ProgID subkey.
	HKEY hKey_OurCLSID_ProgID;
	ret = RegCreateKeyEx(hKey_OurCLSID, L"ProgID", 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKey_OurCLSID_ProgID, nullptr);
	if (ret != ERROR_SUCCESS) {
		// Unable to create/open the ProgID subkey.
		RegCloseKey(hKey_OurCLSID);
		RegCloseKey(hKeyCLSID);
		return SELFREG_E_CLASS;
	}

	// Set the default value.
	ret = RegSetValueEx(hKey_OurCLSID_ProgID, nullptr, 0, REG_SZ,
		(const BYTE*)ProgID, sizeof(ProgID));
	if (ret != ERROR_SUCCESS) {
		// Unable to set the default value.
		RegCloseKey(hKey_OurCLSID_ProgID);
		RegCloseKey(hKey_OurCLSID);
		RegCloseKey(hKeyCLSID);
		return SELFREG_E_CLASS;
	}

	// Close the registry keys.
	RegCloseKey(hKey_OurCLSID_ProgID);
	RegCloseKey(hKey_OurCLSID);
	RegCloseKey(hKeyCLSID);

	/** Register the shell extension as "approved". **/
	HKEY hKey_Approved;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0, KEY_WRITE, &hKey_Approved);
	if (ret != ERROR_SUCCESS) {
		// Unable to open the "Approved" subkey.
		return SELFREG_E_CLASS;
	}

	// Create a value for RP_ExtractIcon.
	static const wchar_t RP_ExtractIcon_Name[] = L"ROM Properties Page - Icon Extractor";
	ret = RegSetValueEx(hKey_Approved, CLSID_RP_ExtractIcon_Str, 0, REG_SZ,
		(const BYTE*)RP_ExtractIcon_Name, sizeof(RP_ExtractIcon_Name));
	if (ret != ERROR_SUCCESS) {
		// Unable to set the CLSID value for RP_ExtractIcon.
		RegCloseKey(hKey_Approved);
		return SELFREG_E_CLASS;
	}

	return (ret == ERROR_SUCCESS ? S_OK : SELFREG_E_CLASS);
}

/**
 * Unregister the DLL.
 */
STDAPI DllUnregisterServer(void)
{
	// TODO
	return S_OK;
}
