/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.hpp: IShellPropSheetExt implementation.            *
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

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx

#include "stdafx.h"
#include "RP_ShellPropSheetExt.hpp"
#include "RegKey.hpp"

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

// Property for "external pointer".
// This links the property sheet to the COM object.
#define EXT_POINTER_PROP L"RP_ShellPropSheetExt"

static inline LPWORD lpwAlign(LPWORD lpIn, ULONG_PTR dw2Power = 4)
{
	ULONG_PTR ul;

	ul = (ULONG_PTR)lpIn;
	ul += dw2Power-1;
	ul &= ~(dw2Power-1);
	return (LPWORD)ul;
}

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
{
	m_szSelectedFile[0] = 0;

	// Create the dialog.
	// NOTE: For property sheets, this MUST be DLGTEMPLATEEX.
	// References:
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms644996(v=vs.85).aspx#template_in_memory
	// - https://blogs.msdn.microsoft.com/oldnewthing/20040623-00/?p=38753
	// TODO: Better dialog template.
	uint8_t *pDlgBuf = m_dlgbuf;

	LPWORD lpw;
	LPDWORD lpdw;
	LPWSTR lpwsz;

	// Set up DLGTEMPLATEEX.
	lpw = (LPWORD)pDlgBuf;
	*lpw++ = 1;		// WORD wVersion
	*lpw++ = 0xFFFF;	// WORD wSignature
	lpdw = (LPDWORD)lpw;
	*lpdw++ = 0;		// DWORD dwHelpID

	// DLGTEMPLATEEX
	*lpdw++ = 0;		// DWORD dwExStyle
	*lpdw++ = DS_SETFONT | DS_FIXEDSYS | WS_CHILD | WS_DISABLED | WS_CAPTION;	// DWORD dwStyle
	lpw = (LPWORD)lpdw;
	*lpw++ = 1;			// WORD cItems
	*lpw++ = 10; *lpw++ = 10;	// WORD x, y
	*lpw++ = 200; *lpw++ = 100;	// WORD cx, cy

	*lpw++ = 0;	// No menu.
	*lpw++ = 0;	// Default dialog class.

	// Dialog title.
	lpwsz = (LPWSTR)lpw;
	wcscpy(lpwsz, L"rpdlg");
	lpw += 6;

	// Font information.
	*lpw++ = 8;		// WORD wSize
	*lpw++ = FW_NORMAL;	// WORD wWeight
	*lpw++ = 0;		// BYTE italic; BYTE charset;

	// Font name.
	lpwsz = (LPWSTR)lpw;
	wcscpy(lpwsz, L"MS Shell Dlg");
	lpw += 13;

	// Define an OK button.
	// DLGITEMTEMPLATEEX: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645389(v=vs.85).aspx
	lpw = lpwAlign(lpw);	// DWORD alignment.
	lpdw = (LPDWORD)lpw;
	*lpdw++ = 0;						// DWORD helpID;
	*lpdw++ = 0;						// DWORD exStyle;
	*lpdw++ = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;	// DWORD style;
	lpw = (LPWORD)lpdw;
	*lpw++ = 10; *lpw++ = 70;				// WORD x, y;
	*lpw++ = 180; *lpw++ = 20;				// WORD cx, cy;
	lpdw = (LPDWORD)lpw;
	*lpdw++ = IDOK;						// DWORD id;
	lpw = (LPWORD)lpdw;
	*lpw++ = 0xFFFF;
	*lpw++ = 0x0080;	// Button class

	// Button text.
	lpwsz = (LPWSTR)lpw;
	wcscpy(lpwsz, L"OK");
	lpw += 3;

	*lpw++ = 0;						// WORD extraCount;
	*lpw++ = 0;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

STDMETHODIMP RP_ShellPropSheetExt::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IShellExtInit) {
		*ppvObj = static_cast<IShellExtInit*>(this);
	} else if (riid == IID_IShellPropSheetExt) {
		*ppvObj = static_cast<IShellPropSheetExt*>(this);
	} else {
		// Interface is not supported.
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::Register(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Property Sheet";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ShellPropSheetExt), description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Register as a property sheet handler for this ProgID.
	// TODO: Register for 'all' types, like the various hash extensions?
	// Create/open the ProgID key.
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, RP_ProgID, KEY_WRITE, true);
	if (!hkcr_ProgID.isOpen())
		return hkcr_ProgID.lOpenRes();
	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkcr_ProgID, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen())
		return hkcr_ShellEx.lOpenRes();
	// Create/open the PropertySheetHandlers key.
	RegKey hkcr_PropertySheetHandlers(hkcr_ShellEx, L"PropertySheetHandlers", KEY_WRITE, true);
	if (!hkcr_PropertySheetHandlers.isOpen())
		return hkcr_PropertySheetHandlers.lOpenRes();
	// Create/open the "rom-properties" property sheet handler key.
	RegKey hkcr_PropSheet_RomProperties(hkcr_PropertySheetHandlers, RP_ProgID, KEY_WRITE, true);
	if (!hkcr_PropSheet_RomProperties.isOpen())
		return hkcr_PropSheet_RomProperties.lOpenRes();
	// Set the default value to this CLSID.
	lResult = hkcr_PropSheet_RomProperties.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS)
		return lResult;
	hkcr_PropSheet_RomProperties.close();
	hkcr_PropertySheetHandlers.close();
	hkcr_ShellEx.close();

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::Unregister(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// TODO
	return ERROR_SUCCESS;
}

PCWSTR RP_ShellPropSheetExt::GetSelectedFile(void) const
{
	return this->m_szSelectedFile;
}

/** IShellExtInit **/

IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

/** IShellPropSheetExt **/
// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb775094(v=vs.85).aspx
IFACEMETHODIMP RP_ShellPropSheetExt::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	HRESULT hr = E_FAIL;
	FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm;

	// The pDataObj pointer contains the objects being acted upon. In this 
	// example, we get an HDROP handle for enumerating the selected files and 
	// folders.
	if (SUCCEEDED(pDataObj->GetData(&fe, &stm)))
	{
		// Get an HDROP handle.
		HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
		if (hDrop != NULL)
		{
			// Determine how many files are involved in this operation. This 
			// code sample displays the custom context menu item when only 
			// one file is selected. 
			UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			if (nFiles == 1)
			{
				// Get the path of the file.
				if (0 != DragQueryFile(hDrop, 0, m_szSelectedFile, 
				    ARRAYSIZE(m_szSelectedFile)))
				{
					// TODO: Verify that this is a supported ROM file.
					hr = S_OK;
				}
			}

			GlobalUnlock(stm.hGlobal);
		}

		ReleaseStgMedium(&stm);
	}

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return hr;
}

/** IShellPropSheetExt **/

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7

	// Create a property sheet page.
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USETITLE | PSP_USECALLBACK | PSP_DLGINDIRECT;
	psp.hInstance = nullptr;
	psp.pResource = (LPCDLGTEMPLATE)m_dlgbuf;
	psp.pszIcon = nullptr;
	psp.pszTitle = L"ROM Properties";
	psp.pfnDlgProc = DlgProc;
	psp.pcRefParent = nullptr;
	psp.pfnCallback = CallbackProc;
	psp.lParam = reinterpret_cast<LPARAM>(this);

	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
	if (hPage == NULL) {
		return E_OUTOFMEMORY;
	}

	// The property sheet page is then added to the property sheet by calling 
	// the callback function (LPFNADDPROPSHEETPAGE pfnAddPage) passed to 
	// IShellPropSheetExt::AddPages.
	if (pfnAddPage(hPage, lParam)) {
		// By default, after AddPages returns, the shell releases its 
		// IShellPropSheetExt interface and the property page cannot access the
		// extension object. However, it is sometimes desirable to be able to use 
		// the extension object, or some other object, from the property page. So 
		// we increase the reference count and maintain this object until the 
		// page is released in PropPageCallbackProc where we call Release upon 
		// the extension.
		this->AddRef();
	} else {
		DestroyPropertySheetPage(hPage);
		return E_FAIL;
	}

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return S_OK;
}

IFACEMETHODIMP RP_ShellPropSheetExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
	// Not used.
	((void)uPageID);
	((void)pfnReplaceWith);
	((void)lParam);
	return E_NOTIMPL;
}

/** Property sheet callback functions. **/

//
//   FUNCTION: FilePropPageDlgProc
//
//   PURPOSE: Processes messages for the property page.
//
INT_PTR CALLBACK RP_ShellPropSheetExt::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (pPage) {
				// Access the property sheet extension from property page.
				RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(pPage->lParam);
				if (pExt) {
					// Display the name of the selected file.
					// NOTE: Uses the "OK" button.
					HWND hFileLabel = GetDlgItem(hWnd, IDOK);
					SetWindowText(hFileLabel, pExt->GetSelectedFile());

					// Store the object pointer with this particular page dialog.
					SetProp(hWnd, EXT_POINTER_PROP, static_cast<HANDLE>(pExt));
				}
			}
			return TRUE;
		}

		case WM_COMMAND: {
			// TODO
#if 0
			switch (LOWORD(wParam)) {
				case IDC_CHANGEPROP_BUTTON:
				// User clicks the "Simulate Property Changing" button...

				// Simulate property changing. Inform the property sheet to 
				// enable the Apply button.
				SendMessage(GetParent(hWnd), PSM_CHANGED, 
					reinterpret_cast<WPARAM>(hWnd), 0);
				return TRUE;
			}
#endif
			break;
		}

		case WM_NOTIFY: {
			// TODO
#if 0
			switch ((reinterpret_cast<LPNMHDR>(lParam))->code) {
				case PSN_APPLY:
					// The PSN_APPLY notification code is sent to every page in the 
					// property sheet to indicate that the user has clicked the OK, 
					// Close, or Apply button and wants all changes to take effect. 

					// Get the property sheet extension object pointer that was 
					// stored in the page dialog (See the handling of WM_INITDIALOG 
					// in FilePropPageDlgProc).
					RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
						GetProp(hWnd, EXT_POINTER_PROP));

					// Access the property sheet extension object.
					// ...

					// Return PSNRET_NOERROR to allow the property dialog to close if 
					// the user clicked OK.
					SetWindowLong(hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

					return TRUE;
			}
#endif
			break;
		}

		case WM_DESTROY: {
			// Remove the EXT_POINTER_PROP property from the page. 
			// The EXT_POINTER_PROP property stored the pointer to the 
			// FilePropSheetExt object.
			RemoveProp(hWnd, EXT_POINTER_PROP);
			return TRUE;
		}

		default:
			break;
	}

	return FALSE; // Let system deal with other messages
}


//
//   FUNCTION: FilePropPageCallbackProc
//
//   PURPOSE: Specifies an application-defined callback function that a property
//            sheet calls when a page is created and when it is about to be 
//            destroyed. An application can use this function to perform 
//            initialization and cleanup operations for the page.
//
UINT CALLBACK RP_ShellPropSheetExt::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return TRUE to enable the page to be created.
			return TRUE;
		}

		case PSPCB_RELEASE: {
			// When the callback function receives the PSPCB_RELEASE notification, 
			// the ppsp parameter of the PropSheetPageProc contains a pointer to 
			// the PROPSHEETPAGE structure. The lParam member of the PROPSHEETPAGE 
			// structure contains the extension pointer which can be used to 
			// release the object.

			// Release the property sheet extension object. This is called even 
			// if the property page was never actually displayed.
			RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(ppsp->lParam);
			if (pExt) {
				pExt->Release();
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}
