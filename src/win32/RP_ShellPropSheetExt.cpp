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
#include "resource.h"

// libromdata
#include "libromdata/common.h"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/RpWin32.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using std::unique_ptr;
using std::unordered_map;
using std::wstring;
using std::vector;

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

// IDC_STATIC might not be defined.
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Control base IDs.
#define IDC_STATIC_DESC(idx)		(0x1000 + (idx))
#define IDC_RFT_STRING(idx)		(0x1400 + (idx))
#define IDC_RFT_LISTDATA(idx)		(0x1800 + (idx))
// Date/Time acts like a string widget internally.
#define IDC_RFT_DATETIME(idx)		IDC_RFT_STRING(idx)

// Bitfield is last due to multiple controls per field.
#define IDC_RFT_BITFIELD(idx, bit)	(0x7000 + ((idx) * 32) + (bit))

// Property for "external pointer".
// This links the property sheet to the COM object.
#define EXT_POINTER_PROP L"RP_ShellPropSheetExt"

class RP_ShellPropSheetExt_Private
{
	public:
		RP_ShellPropSheetExt_Private();
		~RP_ShellPropSheetExt_Private();

	private:
		RP_ShellPropSheetExt_Private(const RP_ShellPropSheetExt_Private &other);
		RP_ShellPropSheetExt_Private &operator=(const RP_ShellPropSheetExt_Private &other);

	public:
		// Selected file.
		wchar_t szSelectedFile[MAX_PATH];
		PCWSTR GetSelectedFile(void) const;

		// ROM data.
		LibRomData::RomData *romData;

		// Property dialog hWnd.
		HWND hDlgProps;

		// Fonts.
		HFONT hFontBold;	// Bold font.
		HFONT hFontMono;	// Monospaced font.

		// Subclassed multiline edit controls.
		unordered_map<HWND, WNDPROC> mapOldEditProc;

		// Header row widgets.
		HWND lblSysInfo;
};

/** RP_ShellPropSheetExt_Private **/

RP_ShellPropSheetExt_Private::RP_ShellPropSheetExt_Private()
	: romData(nullptr)
	, hDlgProps(nullptr)
	, hFontBold(nullptr)
	, hFontMono(nullptr)
	, lblSysInfo(nullptr)
{
	szSelectedFile[0] = 0;
}

RP_ShellPropSheetExt_Private::~RP_ShellPropSheetExt_Private()
{
	delete romData;

	// Delete the fonts.
	if (hFontBold) {
		DeleteFont(hFontBold);
	}
	if (hFontMono) {
		DeleteFont(hFontMono);
	}
}

PCWSTR RP_ShellPropSheetExt_Private::GetSelectedFile(void) const
{
	return this->szSelectedFile;
}

/** RP_ShellPropSheetExt **/

static inline LPWORD lpwAlign(LPWORD lpIn, ULONG_PTR dw2Power = 4)
{
	ULONG_PTR ul;

	ul = (ULONG_PTR)lpIn;
	ul += dw2Power-1;
	ul &= ~(dw2Power-1);
	return (LPWORD)ul;
}

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
	: d(new RP_ShellPropSheetExt_Private())
{ }

RP_ShellPropSheetExt::~RP_ShellPropSheetExt()
{
	delete d;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ShellPropSheetExt::QueryInterface(REFIID riid, LPVOID *ppvObj)
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

/** IShellExtInit **/

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
	if (SUCCEEDED(pDataObj->GetData(&fe, &stm))) {
		// Get an HDROP handle.
		HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
		if (hDrop != NULL) {
			// Determine how many files are involved in this operation. This 
			// code sample displays the custom context menu item when only 
			// one file is selected. 
			UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			if (nFiles == 1) {
				// Get the path of the file.
				if (0 != DragQueryFile(hDrop, 0, d->szSelectedFile, 
				    ARRAYSIZE(d->szSelectedFile)))
				{
					// Open the file.
					unique_ptr<IRpFile> file(new RpFile(
						rp_string(W2RP_c(d->szSelectedFile)),
						RpFile::FM_OPEN_READ));
					if (file && file->isOpen()) {
						// Get the appropriate RomData class for this ROM.
						// file is dup()'d by RomData.
						d->romData = RomDataFactory::getInstance(file.get());
						hr = (d->romData != nullptr ? S_OK : E_FAIL);
					}
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

/**
 * Convert UNIX line endings to DOS line endings.
 * TODO: Move to RpWin32?
 * @param wstr_unix	[in] wstring with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return wstring with DOS line endings.
 */
static inline wstring unix2dos(const wstring &wstr_unix, int *lf_count = nullptr)
{
	// TODO: Optimize this!
	wstring wstr_dos;
	wstr_dos.reserve(wstr_unix.size());
	int lf = 0;
	for (size_t i = 0; i < wstr_unix.size(); i++) {
		if (wstr_unix[i] == L'\n') {
			wstr_dos += L"\r\n";
			lf++;
		} else {
			wstr_dos += wstr_unix[i];
		}
	}
	if (lf_count) {
		*lf_count = lf;
	}
	return wstr_dos;
}

/**
 * Create the header row.
 * @param hDlg		[in] Dialog window.
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a full-width single line label.
 * @return Row height, in pixels.
 */
int RP_ShellPropSheetExt::createHeaderRow(HWND hDlg, const POINT &pt_start, const SIZE &size)
{
	if (!hDlg)
		return 0;

	// System name.
	// TODO: Logo, game icon, and game title?
	const rp_char *systemName = d->romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);

	// File type.
	const rp_char *fileType = nullptr;
	switch (d->romData->fileType()) {
		case RomData::FTYPE_ROM_IMAGE:
			fileType = _RP("ROM Image");
			break;
		case RomData::FTYPE_DISC_IMAGE:
			fileType = _RP("Disc Image");
			break;
		case RomData::FTYPE_SAVE_FILE:
			fileType = _RP("Save File");
			break;
		case RomData::FTYPE_UNKNOWN:
		default:
			fileType = nullptr;
			break;
	}

	wstring sysInfo;
	int sysInfo_lines = 1;
	if (systemName) {
		sysInfo = RP2W_c(systemName);
	}
	if (fileType) {
		if (!sysInfo.empty()) {
			sysInfo += L"\r\n";
			sysInfo_lines++;
		}
		sysInfo += RP2W_c(fileType);
	}

	// Get the default font.
	HFONT hFont = GetWindowFont(hDlg);
 
	// Label size.
	SIZE sz_lblSysInfo = {0, 0};

	if (!sysInfo.empty()) {
		// Use a bold font.
		// TODO: Delete the old font if it's already there?
		if (!d->hFontBold) {
			// Create the bold font.
			LOGFONT lfFontBold;
			if (GetObject(hFont, sizeof(lfFontBold), &lfFontBold) != 0) {
				// Adjust the font and create a new one.
				lfFontBold.lfWeight = FW_BOLD;
				d->hFontBold = CreateFontIndirect(&lfFontBold);
			}
		}

		// Determine the appropriate label size.
		HDC hDC = GetDC(hDlg);
		HFONT hFontOrig = SelectFont(hDC, hFont);
		BOOL bRet = GetTextExtentPoint32(hDC, sysInfo.data(), (int)sysInfo.size(), &sz_lblSysInfo);
		if (!bRet) {
			// Error determining the label size.
			// Don't draw the label.
			sz_lblSysInfo.cx = 0;
			sz_lblSysInfo.cy = 0;
		} else {
			// GetTextExtentPoint32() doesn't take newlines into account.
			// Adjust the height for newlines.
			sz_lblSysInfo.cy *= sysInfo_lines;
		}
	}

	// Total widget width.
	// TODO: Add icon and banner.
	const int total_widget_width = sz_lblSysInfo.cx;
	POINT curPt = {
		((size.cx - total_widget_width) / 2) + pt_start.x,
		pt_start.y
	};

	// lblSysInfo
	if (sz_lblSysInfo.cx > 0 && sz_lblSysInfo.cy > 0) {
		d->lblSysInfo = CreateWindow(WC_STATIC, sysInfo.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			curPt.x, curPt.y,
			sz_lblSysInfo.cx, sz_lblSysInfo.cy,
			hDlg, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(d->lblSysInfo, (d->hFontBold ? d->hFontBold : hFont), FALSE);
		curPt.x += sz_lblSysInfo.cx + pt_start.x;
	}

	// Return the label height and some extra padding.
	return sz_lblSysInfo.cy + (pt_start.y * 5 / 8);
}

/**
 * Initialize a string field. (Also used for Date/Time.)
 * @param hDlg		[in] Dialog window.
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @param wcs		[in,opt] String data. (If nullptr, field data is used.)
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt::initString(HWND hDlg, const POINT &pt_start, int idx, const SIZE &size, LPCWSTR wcs)
{
	if (!hDlg)
		return 0;

	const RomFields *fields = d->romData->fields();
	if (!fields)
		return 0;

	const RomFields::Desc *desc = fields->desc(idx);
	if (!desc)
		return 0;
	if (!desc->name || desc->name[0] == '\0')
		return 0;

	// NOTE: libromdata uses Unix-style newlines.
	// For proper display on Windows, we have to
	// add carriage returns.

	// If string data wasn't specified, get the RFT_STRING data
	// from the RomFields::Data object.
	int lf_count = 0;
	wstring wstr;
	if (!wcs) {
		const RomFields::Data *data = fields->data(idx);
		if (!data)
			return 0;
		if (desc->type != RomFields::RFT_STRING ||
		    data->type != RomFields::RFT_STRING)
			return 0;

		// TODO: NULL string == empty string?
		if (data->str) {
			wstr = unix2dos(RP2W_s(data->str), &lf_count);
		}
	} else {
		// Use the specified string.
		wstr = unix2dos(wstring(wcs), &lf_count);
	}

	// Field height.
	int field_cy = size.cy;

	// Create a read-only EDIT widget.
	// The STATIC control doesn't allow the user
	// to highlight and copy data.
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | ES_READONLY;
	if (lf_count > 0) {
		// Multiple lines.
		// NOTE: Only add 5/8 of field_cy per line.
		// FIXME: 5/8 needs adjustment...
		field_cy += (field_cy * lf_count) * 5 / 8;
		dwStyle |= ES_MULTILINE;
	}

	HWND hDlgItem = CreateWindow(WC_EDIT, wstr.c_str(), dwStyle,
		pt_start.x, pt_start.y,
		size.cx, field_cy,
		hDlg, (HMENU)(INT_PTR)(IDC_RFT_STRING(idx)),
		nullptr, nullptr);

	// Get the default font.
	HFONT hFont = GetWindowFont(hDlg);

	// Subclass multiline controls to work around Enter/Escape issues.
	// Reference:  http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
	if (dwStyle & ES_MULTILINE) {
		// Store the object pointer so we can reference it later.
		SetProp(hDlgItem, EXT_POINTER_PROP, static_cast<HANDLE>(this));

		// Subclass the control.
		WNDPROC oldEditProc = (WNDPROC)SetWindowLongPtr(
			hDlgItem, GWLP_WNDPROC, (LONG_PTR)MultilineEditProc);
		d->mapOldEditProc.insert(std::make_pair(hDlgItem, oldEditProc));
	}

	// Check for any formatting options.
	if (desc->type == RomFields::RFT_STRING && desc->str_desc) {
		// Monospace font?
		if (desc->str_desc->formatting & RomFields::StringDesc::STRF_MONOSPACE) {
			if (d->hFontMono != nullptr) {
				hFont = d->hFontMono;
			}
		}
	}

	SetWindowFont(hDlgItem, hFont, FALSE);
	return field_cy;
}

/**
 * Initialize a bitfield layout.
 * @param hDlg Dialog window.
 * @param pt_start Starting position, in pixels.
 * @param idx Field index.
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt::initBitfield(HWND hDlg, const POINT &pt_start, int idx)
{
	if (!hDlg)
		return 0;

	const RomFields *fields = d->romData->fields();
	if (!fields)
		return 0;

	const RomFields::Desc *desc = fields->desc(idx);
	const RomFields::Data *data = fields->data(idx);
	if (!desc || !data)
		return 0;
	if (desc->type != RomFields::RFT_BITFIELD ||
	    data->type != RomFields::RFT_BITFIELD)
		return 0;
	if (!desc->name || desc->name[0] == '\0')
		return 0;

	// Checkbox size.
	// Reference: http://stackoverflow.com/questions/1164868/how-to-get-size-of-check-and-gap-in-check-box
	RECT rect_chkbox = {0, 0, 12+4, 11};
	MapDialogRect(hDlg, &rect_chkbox);

	// Dialog font and device context.
	HFONT hFont = GetWindowFont(hDlg);
	HDC hDC = GetDC(hDlg);
	HFONT hFontOrig = SelectFont(hDC, hFont);

	// Create a grid of checkboxes.
	const RomFields::BitfieldDesc *bitfieldDesc = desc->bitfield;

	// Column widths for multi-row layouts.
	// (Includes the checkbox size.)
	std::vector<int> col_widths;
	int row = 0, col = 0;
	if (bitfieldDesc->elemsPerRow == 1) {
		// Optimization: Use the entire width of the dialog.
		// TODO: Testing; right margin.
		RECT rectDlg;
		GetClientRect(hDlg, &rectDlg);
		col_widths.push_back(rectDlg.right - pt_start.x);
	} else if (bitfieldDesc->elemsPerRow > 1) {
		// Determine the widest entry in each column.
		col_widths.resize(bitfieldDesc->elemsPerRow);
		for (int j = 0; j < bitfieldDesc->elements; j++) {
			const rp_char *name = bitfieldDesc->names[j];
			if (!name)
				continue;

			// Make sure this is a UTF-16 string.
			wstring s_name = RP2W_c(name);

			// Get the width of this specific entry.
			SIZE textSize;
			GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
			int chk_w = rect_chkbox.right + textSize.cx;
			if (chk_w > col_widths[col]) {
				col_widths[col] = chk_w;
			}

			// Next column.
			col++;
			if (col == bitfieldDesc->elemsPerRow) {
				// Next row.
				row++;
				col = 0;
			}
		}
	}

	// Initial position on the dialog, in pixels.
	POINT pt = pt_start;
	// Subtract 0.5 DLU from the starting row.
	RECT rect_subtract = {0, 0, 1, 1};
	MapDialogRect(hDlg, &rect_subtract);
	if (rect_subtract.bottom > 1) {
		rect_subtract.bottom /= 2;
	}
	pt.y -= rect_subtract.bottom;

	row = 0; col = 0;
	for (int j = 0; j < bitfieldDesc->elements; j++) {
		const rp_char *name = bitfieldDesc->names[j];
		if (!name)
			continue;

		// Get the text size.
		int chk_w;
		if (bitfieldDesc->elemsPerRow == 0) {
			// Make sure this is a UTF-16 string.
			wstring s_name = RP2W_c(name);

			// Get the width of this specific entry.
			SIZE textSize;
			GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
			chk_w = rect_chkbox.right + textSize.cx;
		} else {
			if (col == bitfieldDesc->elemsPerRow) {
				// Next row.
				row++;
				col = 0;
				pt.x = pt_start.x;
				pt.y += rect_chkbox.bottom;
			}

			// Use the largest width in the column.
			chk_w = col_widths[col];
		}

		// FIXME: Tab ordering?
		HWND hCheckBox = CreateWindow(WC_BUTTON, RP2W_c(name),
			WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
			pt.x, pt.y, chk_w, rect_chkbox.bottom,
			hDlg, (HMENU)(INT_PTR)(IDC_RFT_BITFIELD(idx, j)),
			nullptr, nullptr);
		SetWindowFont(hCheckBox, hFont, FALSE);

		// Set the checkbox state.
		Button_SetCheck(hCheckBox, (data->bitfield & (1 << j)) ? BST_CHECKED : BST_UNCHECKED);

		// Next column.
		pt.x += chk_w;
		col++;
	}

	SelectFont(hDC, hFontOrig);
	ReleaseDC(hDlg, hDC);

	// Return the total number of rows times the checkbox height,
	// plus another 0.25 of a checkbox.
	int ret = ((row + 1) * rect_chkbox.bottom);
	ret += (rect_chkbox.bottom / 4);
	return ret;
}

/**
 * Initialize a ListView control.
 * @param hWnd HWND of the ListView control.
 * @param desc RomFields description.
 * @param data RomFields data.
 */
void RP_ShellPropSheetExt::initListView(HWND hWnd, const RomFields::Desc *desc, const RomFields::Data *data)
{
	if (!hWnd || !desc || !data)
		return;
	if (desc->type != RomFields::RFT_LISTDATA ||
	    data->type != RomFields::RFT_LISTDATA)
		return;
	if (!desc->name || desc->name[0] == '\0')
		return;

	// Set extended ListView styles.
	ListView_SetExtendedListViewStyle(hWnd, LVS_EX_FULLROWSELECT);

	// Insert columns.
	// TODO: Make sure there aren't any columns to start with?
	const RomFields::ListDataDesc *listDataDesc = desc->list_data;
	const int count = listDataDesc->count;
	for (int i = 0; i < count; i++) {
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_FMT | LVCF_TEXT;
		lvColumn.fmt = LVCFMT_LEFT;
		if (listDataDesc->names[i]) {
			// TODO: Support for RP_UTF8?
			// NOTE: pszText is LPWSTR, not LPCWSTR...
			lvColumn.pszText = (LPWSTR)listDataDesc->names[i];
		} else {
			// Don't show this column.
			// FIXME: Zero-width column is a bad hack...
			lvColumn.pszText = L"";
			lvColumn.mask |= LVCF_WIDTH;
			lvColumn.cx = 0;
		}

		ListView_InsertColumn(hWnd, i, &lvColumn);
	}

	// Add the row data.
	const RomFields::ListData *listData = data->list_data;
	for (int i = 0; i < (int)listData->data.size(); i++) {
		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = i;

		const vector<rp_string> &data_row = listData->data.at(i);
		int field = 0;
		for (vector<rp_string>::const_iterator iter = data_row.begin();
		     iter != data_row.end(); ++iter, ++field)
		{
			lvItem.iSubItem = field;
			// TODO: Support for RP_UTF8?
			// NOTE: pszText is LPWSTR, not LPCWSTR...
			lvItem.pszText = (LPWSTR)(*iter).c_str();
			if (field == 0) {
				// Field 0: Insert the item.
				ListView_InsertItem(hWnd, &lvItem);
			} else {
				// Fields 1 and higher: Set the subitem.
				ListView_SetItem(hWnd, &lvItem);
			}
		}
	}

	// Resize all of the columns.
	// TODO: Do this on system theme change?
	for (int i = 0; i < count; i++) {
		ListView_SetColumnWidth(hWnd, i, LVSCW_AUTOSIZE_USEHEADER);
	}
}

/**
 * Initialize a Date/Time field.
 * This function internally calls initString().
 * @param hDlg		[in] Dialog window.
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt::initDateTime(HWND hDlg, const POINT &pt_start, int idx, const SIZE &size)
{
	if (!hDlg)
		return 0;

	const RomFields *fields = d->romData->fields();
	if (!fields)
		return 0;

	const RomFields::Desc *desc = fields->desc(idx);
	const RomFields::Data *data = fields->data(idx);
	if (!desc || !data)
		return 0;
	if (desc->type != RomFields::RFT_DATETIME ||
	    data->type != RomFields::RFT_DATETIME)
		return 0;
	if (!desc->name || desc->name[0] == '\0')
		return 0;

	// Format the date/time using the system locale.
	const RomFields::DateTimeDesc *const dateTimeDesc = desc->date_time;
	wchar_t dateTimeStr[256];
	int start_pos = 0;
	int cchBuf = ARRAY_SIZE(dateTimeStr);

	// Convert from UNIX time to Win32 SYSTEMTIME.
	// Reference: https://support.microsoft.com/en-us/kb/167296
	SYSTEMTIME st;
	FILETIME ft;
	int64_t ftll = (data->date_time * 10000000LL) + 116444736000000000LL;
	ft.dwLowDateTime = (DWORD)ftll;
	ft.dwHighDateTime = (DWORD)(ftll >> 32);
	FileTimeToSystemTime(&ft, &st);

	// At least one of Date and/or Time must be set.
	assert((dateTimeDesc->flags &
		(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME)) != 0);

	if (!(dateTimeDesc->flags & RomFields::RFT_DATETIME_IS_UTC)) {
		// Convert to the current timezone.
		SYSTEMTIME st_utc = st;
		BOOL ret = SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st);
		if (!ret) {
			// Conversion failed.
			return 0;
		}
	}

	if (dateTimeDesc->flags & RomFields::RFT_DATETIME_HAS_DATE) {
		// Format the date.
		int ret = GetDateFormat(
			MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
			DATE_SHORTDATE,
			&st, nullptr, &dateTimeStr[start_pos], cchBuf);
		if (ret <= 0) {
			// Error!
			return 0;
		}

		// Adjust the buffer position.
		// NOTE: ret includes the NULL terminator.
		start_pos += ret-1;
		cchBuf -= ret-1;
	}

	if (dateTimeDesc->flags & RomFields::RFT_DATETIME_HAS_TIME) {
		// Format the time.
		if (start_pos > 0 && cchBuf >= 1) {
			// Add a space.
			dateTimeStr[start_pos] = L' ';
			dateTimeStr[start_pos+1] = 0;
			start_pos++;
			cchBuf--;
		}

		int ret = GetTimeFormat(
			MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
			0, &st, nullptr, &dateTimeStr[start_pos], cchBuf);
		if (ret <= 0) {
			// Error!
			return 0;
		}

		// Adjust the buffer position.
		// NOTE: ret includes the NULL terminator.
		start_pos += ret-1;
		cchBuf -= ret-1;
	}

	if (start_pos == 0) {
		// Empty string.
		// Something failed...
		return 0;
	}

	// Initialize the string.
	return initString(hDlg, pt_start, idx, size, dateTimeStr);
}

/**
 * Initialize the dialog.
 * Called by WM_INITDIALOG.
 * @param hDlg Dialog window.
 */
void RP_ShellPropSheetExt::initDialog(HWND hDlg)
{
	if (!d->romData) {
		// No ROM data loaded.
		return;
	}

	// Get the fields.
	const RomFields *fields = d->romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = fields->count();

	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Dialog font and device context.
	HFONT hFont = GetWindowFont(hDlg);
	HDC hDC = GetDC(hDlg);
	HFONT hFontOrig = SelectFont(hDC, hFont);

	// Determine the maximum length of all field names.
	// TODO: Line breaks?
	int max_text_width = 0;
	SIZE textSize;
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		// Make sure this is a UTF-16 string.
		wstring s_name = RP2W_c(desc->name);

		// Get the width of this specific entry.
		GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
		if (textSize.cx > max_text_width) {
			max_text_width = textSize.cx;
		}
	}

	// Add additional spacing for the ':'.
	GetTextExtentPoint32(hDC, L":  ", 3, &textSize);
	max_text_width += textSize.cx;

	// Release the DC.
	SelectFont(hDC, hFontOrig);
	ReleaseDC(hDlg, hDC);

	// Create the ROM field widgets.
	// Each static control is max_text_width pixels wide
	// and 8 DLUs tall, plus 4 vertical DLUs for spacing.
	RECT tmpRect = {0, 0, 0, 8+4};
	MapDialogRect(hDlg, &tmpRect);
	const SIZE descSize = {max_text_width, tmpRect.bottom};

	// Current position.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	tmpRect.left = 7; tmpRect.top = 7;
	tmpRect.right = 8; tmpRect.bottom = 8;
	MapDialogRect(hDlg, &tmpRect);
	POINT curPt = {tmpRect.left, tmpRect.top};

	// Width available for the value widget(s).
	GetClientRect(hDlg, &tmpRect);
	const int dlg_value_width = tmpRect.right - (curPt.x * 2) - descSize.cx;

	// Create the header row.
	const SIZE full_width_size = {dlg_value_width + descSize.cx, descSize.cy};
	curPt.y += createHeaderRow(hDlg, curPt, full_width_size);

	// Get a matching monospaced font.
	// TODO: Delete the old font if it's already there?
	if (!d->hFontMono) {
		LOGFONT lfFontMono;
		if (GetObject(hFont, sizeof(lfFontMono), &lfFontMono) != 0) {
			// Adjust the font and create a new one.
			// TODO: What's the best font for this?
			wcscpy(lfFontMono.lfFaceName, L"Lucida Console");
			d->hFontMono = CreateFontIndirect(&lfFontMono);
		}
	}

	for (int idx = 0; idx < count; idx++) {
		const RomFields::Desc *desc = fields->desc(idx);
		const RomFields::Data *data = fields->data(idx);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		// Append a ":" to the description.
		// TODO: Localization.
		wstring desc_text = RP2W_c(desc->name);
		desc_text += L':';

		// Create the static text widget. (FIXME: Disable mnemonics?)
		HWND hStatic = CreateWindow(WC_STATIC, desc_text.c_str(),
			WS_CHILD | WS_VISIBLE | SS_LEFT,
			curPt.x, curPt.y, descSize.cx, descSize.cy,
			hDlg, (HMENU)(INT_PTR)(IDC_STATIC_DESC(idx)),
			nullptr, nullptr);
		SetWindowFont(hStatic, hFont, FALSE);

		// Create the value widget.
		int field_cy = descSize.cy;	// Default row size.
		const POINT pt_start = {curPt.x + descSize.cx, curPt.y};
		HWND hDlgItem;
		switch (desc->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				DestroyWindow(hStatic);
				field_cy = 0;
				break;

			case RomFields::RFT_STRING: {
				// String data.
				SIZE size = {dlg_value_width, field_cy};
				field_cy = initString(hDlg, pt_start, idx, size, nullptr);
				if (field_cy == 0) {
					// initString() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;
			}

			case RomFields::RFT_BITFIELD:
				// Create checkboxes starting at the current point.
				field_cy = initBitfield(hDlg, pt_start, idx);
				if (field_cy == 0) {
					// initBitfield() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;

			case RomFields::RFT_LISTDATA:
				// Increase row height to 72 DLU.
				// descSize is 8+4 DLU (12); 72/12 == 6
				// FIXME: The last row seems to be cut off with the
				// Windows XP Luna theme, but not Windows Classic...
				field_cy *= 6;

				// Create a ListView widget.
				hDlgItem = CreateWindowEx(WS_EX_LEFT | WS_EX_CLIENTEDGE,
					WC_LISTVIEW, RP2W_c(data->str),
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_ALIGNLEFT | LVS_REPORT,
					pt_start.x, pt_start.y,
					dlg_value_width, field_cy,
					hDlg, (HMENU)(INT_PTR)(IDC_RFT_LISTDATA(idx)),
					nullptr, nullptr);
				SetWindowFont(hDlgItem, hFont, FALSE);

				// Initialize the ListView data.
				initListView(hDlgItem, desc, fields->data(idx));
				break;

			case RomFields::RFT_DATETIME: {
				// Date/Time in Unix format.
				SIZE size = {dlg_value_width, field_cy};
				field_cy = initDateTime(hDlg, pt_start, idx, size);
				if (field_cy == 0) {
					// initDateTime() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;
			}

			default:
				// Unsupported data type.
				assert(!"Unsupported RomFields::RomFieldsType.");
				DestroyWindow(hStatic);
				field_cy = 0;
				break;
		}

		// Next row.
		curPt.y += field_cy;
	}
}

/** IShellPropSheetExt **/

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7

	// Create a property sheet page.
	extern HINSTANCE g_hInstance;
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTY_SHEET);
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
					// Store the object pointer with this particular page dialog.
					SetProp(hWnd, EXT_POINTER_PROP, static_cast<HANDLE>(pExt));

					// Save the property dialog handle for later.
					pExt->d->hDlgProps = GetParent(hWnd);

					// Initialize the dialog.
					pExt->initDialog(hWnd);
				}
			}
			return TRUE;
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

/**
 * Subclass procedure for ES_MULTILINE EDIT controls.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK RP_ShellPropSheetExt::MultilineEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
		GetProp(hWnd, EXT_POINTER_PROP));
	if (!pExt) {
		// No RP_ShellPropSheetExt. Can't do anything...
		return 0;
	}

	switch (uMsg) {
		case WM_KEYDOWN:
			// Work around Enter/Escape issues.
			// Reference: http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
			switch (wParam) {
				case VK_RETURN:
					SendMessage(pExt->d->hDlgProps, WM_COMMAND, IDOK, 0);
					return TRUE;

				case VK_ESCAPE:
					SendMessage(pExt->d->hDlgProps, WM_COMMAND, IDCANCEL, 0);
					return TRUE;

				default:
					break;
			}

		default:
			break;
	}

	// Call the original window procedure.
	unordered_map<HWND, WNDPROC>::const_iterator iter = pExt->d->mapOldEditProc.find(hWnd);
	if (iter != pExt->d->mapOldEditProc.end()) {
		WNDPROC wndProc = iter->second;
		return CallWindowProc(wndProc, hWnd, uMsg, wParam, lParam);
	}

	// Can't find the original window procedure...
	return DefDlgProc(hWnd, uMsg, wParam, lParam);
}
