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
	static const wchar_t ProgID[] = L"rom-properties";

	/** Register the ".nds" file type. **/

	// Create/open the ".nds" key.
	RegKey hkcr_nds(HKEY_CLASSES_ROOT, L".nds", KEY_WRITE, true);
	if (!hkcr_nds.isOpen())
		return SELFREG_E_CLASS;
	// Set the default value to "rom-properties".
	LONG ret = hkcr_nds.write(nullptr, ProgID);
	if (ret != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	hkcr_nds.close();

	/** Register the ProgID. **/

	// Create/open the ProgID key.
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, ProgID, KEY_WRITE, true);
	if (!hkcr_ProgID.isOpen())
		return SELFREG_E_CLASS;

	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkcr_ProgID, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen())
		return SELFREG_E_CLASS;
	// Create/open the "IconHandler" key.
	RegKey hkcr_IconHandler(hkcr_ShellEx, L"IconHandler", KEY_WRITE, true);
	if (!hkcr_IconHandler.isOpen())
		return SELFREG_E_CLASS;
	// Set the default value to RP_ExtractIcon's CLSID.
	ret = hkcr_IconHandler.write(nullptr, CLSID_RP_ExtractIcon_Str);
	if (ret != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	hkcr_IconHandler.close();
	hkcr_ShellEx.close();

	// Create/open the "DefaultIcon" key.
	RegKey hkcr_DefaultIcon(hkcr_ProgID, L"DefaultIcon", KEY_WRITE, true);
	if (!hkcr_DefaultIcon.isOpen())
		return SELFREG_E_CLASS;
	// Set the default value to "%1".
	ret = hkcr_DefaultIcon.write(nullptr, L"%1");
	if (ret != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	hkcr_DefaultIcon.close();
	hkcr_ProgID.close();

	/** Register the DLL itself. **/

	// Open HKCR\CLSID.
	RegKey hkcr_CLSID(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE, false);
	if (!hkcr_CLSID.isOpen())
		return SELFREG_E_CLASS;
	// Create a key using the CLSID.
	RegKey hkcr_RP_ExtractIcon(hkcr_CLSID, CLSID_RP_ExtractIcon_Str, KEY_WRITE, true);
	if (!hkcr_RP_ExtractIcon.isOpen())
		return SELFREG_E_CLASS;
	// Set the name of the key.
	ret = hkcr_RP_ExtractIcon.write(nullptr, ProgID);
	if (ret != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	// Create an InprocServer32 subkey.
	RegKey hkcr_InprocServer32(hkcr_RP_ExtractIcon, L"InprocServer32", KEY_WRITE, true);
	if (!hkcr_InprocServer32.isOpen())
		return SELFREG_E_CLASS;

	// Set the default value to the DLL filename.
	wchar_t dll_filename[MAX_PATH];
	int dll_filename_len = GetModuleFileName(g_hInstance, dll_filename, sizeof(dll_filename)/sizeof(dll_filename[0]));
	// TODO: Handle buffer size errors.
	if (dll_filename_len > 0) {
		ret = hkcr_InprocServer32.write(nullptr, dll_filename);
		if (ret != ERROR_SUCCESS)
			return SELFREG_E_CLASS;
	}

	// Set the threading model to Apartment.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/cc144110%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
	ret = hkcr_InprocServer32.write(L"ThreadingModel", L"Apartment");
	if (ret != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	hkcr_InprocServer32.close();

	// Create a ProgID subkey.
	RegKey hkcr_CLSID_ProgID(hkcr_RP_ExtractIcon, L"ProgID", KEY_WRITE, true);
	if (!hkcr_CLSID_ProgID.isOpen())
		return SELFREG_E_CLASS;
	// Set the default value.
	ret = hkcr_CLSID_ProgID.write(nullptr, ProgID);
	if (ret != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	hkcr_CLSID_ProgID.close();
	hkcr_RP_ExtractIcon.close();
	hkcr_CLSID.close();

	/** Register the shell extension as "approved". **/
	RegKey hklm_Approved(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
		KEY_WRITE, false);
	if (!hklm_Approved.isOpen())
		return SELFREG_E_CLASS;

	// Create a value for RP_ExtractIcon.
	static const wchar_t RP_ExtractIcon_Name[] = L"ROM Properties Page - Icon Extractor";
	ret = hklm_Approved.write(CLSID_RP_ExtractIcon_Str, RP_ExtractIcon_Name);
	if (ret != ERROR_SUCCESS)
		return SELFREG_E_CLASS;
	hklm_Approved.close();

	return S_OK;
}

/**
 * Unregister the DLL.
 */
STDAPI DllUnregisterServer(void)
{
	// TODO
	return S_OK;
}
