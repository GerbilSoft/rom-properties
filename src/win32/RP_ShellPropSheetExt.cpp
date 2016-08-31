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

// libromdata
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/RpFile.hpp"
using LibRomData::RomDataFactory;
using LibRomData::RomData;
using LibRomData::RomFields;
using LibRomData::IRpFile;
using LibRomData::RpFile;
using LibRomData::rp_string;
using LibRomData::rp_strlen;

// C++ includes.
#include <vector>
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
#define IDC_RFT_STRING(idx)		(0x2000 + (idx))
#define IDC_RFT_BITFIELD(idx, bit)	(0x3000 + ((idx) * 32) + (bit))
#define IDC_RFT_LISTDATA(idx)		(0x4000 + (idx))

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
	: m_romData(nullptr)
{
	m_szSelectedFile[0] = 0;
}

RP_ShellPropSheetExt::~RP_ShellPropSheetExt()
{
	delete m_romData;
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

PCWSTR RP_ShellPropSheetExt::GetSelectedFile(void) const
{
	return this->m_szSelectedFile;
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
				if (0 != DragQueryFile(hDrop, 0, m_szSelectedFile, 
				    ARRAYSIZE(m_szSelectedFile)))
				{
					// Open the file.
					// FIXME: Use utf16_to_rp_string() after merging to master.
					IRpFile *file = new RpFile(rp_string(W2RP_c(m_szSelectedFile)),
						RpFile::FM_OPEN_READ);
					if (file && file->isOpen()) {
						// Get the appropriate RomData class for this ROM.
						m_romData = RomDataFactory::getInstance(file);
						hr = (m_romData != nullptr ? S_OK : E_FAIL);
					}

					// RomData classes dup() the file, so
					// we can close the original one.
					delete file;
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
 * Initialize the dialog for the open ROM data object.
 * @return Dialog template from DialogBuilder on success; nullptr on error.
 */
LPCDLGTEMPLATE RP_ShellPropSheetExt::initDialog(void)
{
	if (!m_romData) {
		// No ROM data loaded.
		m_dlgBuilder.clear();
		return nullptr;
	}

	// Get the fields.
	const RomFields *fields = m_romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		m_dlgBuilder.clear();
		return nullptr;
	}
	const int count = fields->count();

	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Create the dialog.
	DLGTEMPLATE dlt;
	dlt.style = DS_SETFONT | DS_FIXEDSYS | WS_CHILD | WS_DISABLED | WS_CAPTION;
	dlt.dwExtendedStyle = 0;
	dlt.cdit = 0;   // automatically updated by DialogBuilder
	dlt.x = 0; dlt.y = 0;
	dlt.cx = PROP_MED_CXDLG; dlt.cy = PROP_MED_CYDLG;
	// TODO: Localization.
	m_dlgBuilder.init(&dlt, L"ROM Properties");

	// Dialog font and device context.
	// Reference: http://cboard.cprogramming.com/windows-programming/81464-obtaining-system-font-custom-size.html
	const int fontPtSize = 8;
	HDC hDC = GetDC(nullptr);
	const LOGFONT lfDlgFont = {
		MulDiv(fontPtSize, GetDeviceCaps(hDC, LOGPIXELSY), 72),	// lfHeight
		0, 0, 0,			// lfWidht, lfEscapement, lfOrientation
		FW_NORMAL, FALSE, FALSE, FALSE,	// lfWeight, lfItalic, lfUnderline, lfStrikeOut
		DEFAULT_CHARSET,		// lfCharSet
		OUT_DEFAULT_PRECIS,		// lfOutPrecision
		CLIP_DEFAULT_PRECIS,		// lfClipPrecision
		DEFAULT_QUALITY,		// lfQuality
		DEFAULT_PITCH | FF_DONTCARE,	// lfPitchAndFamily
		L"MS Shell Dlg"			// lfFaceName
	};
	HFONT hFont = CreateFontIndirect(&lfDlgFont);
	HFONT hFontOrig = SelectFont(hDC, hFont);

	// Calculate dialog units for this font.
	// Reference: https://support.microsoft.com/en-us/kb/145994
	SIZE dlgBase;
	{
		TEXTMETRIC tm;
		GetTextMetrics(hDC, &tm);
		SIZE textSize;
		GetTextExtentPoint32(hDC,
			L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &textSize);
		dlgBase.cx = (textSize.cx / 26 + 1) / 2;
		dlgBase.cy = tm.tmHeight;
	}

	// Determine the maximum length of all field names.
	// TODO: Line breaks?
	int max_text_width = 0;
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
		std::wstring s_name = RP2W_c(desc->name);

		// Get the width of this specific entry.
		SIZE textSize;
		GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
		if (textSize.cx > max_text_width) {
			max_text_width = textSize.cx;
		}
	}

	// Convert the text width to dialog units.
	// NOTE: +(4*3) for the ":". [TODO: Is that right?]
	const short dlu_text_width = (short)((max_text_width * 4) / dlgBase.cx) + (4*3);

	SelectFont(hDC, hFontOrig);
	DeleteFont(hFont);
	ReleaseDC(nullptr, hDC);

	// Create the ROM field widgets.
	// Each static control is dlu_text_width DLUs wide,
	// and 8 DLUs tall, plus 4 vertical DLUs for spacing.
	// NOTE: Not using SIZE because dialogs use short, not LONG.
	// FIXME: May need to resize labels on display because the
	// DLU calculation isn't always accurate...
	struct DLU_SZ { short cx, cy; };
	const DLU_SZ descSize = {dlu_text_width, 8+4};

	// Current position.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	// NOTE: Not using POINT because dialogs use short, not LONG.
	struct DLU_PT { short x, y; };
	DLU_PT curPt = {7, 7};

	// Width available for the value widget(s).
	const short dlit_value_width = dlt.cx - (curPt.x * 2) - descSize.cx;

	DLGITEMTEMPLATE dlit;
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		// Append a ":" to the description.
		// TODO: Localization.
		rp_string desc_text(desc->name);
		desc_text += _RP_CHR(':');

		// Create the static text widget. (FIXME: Disable mnemonics?)
		dlit.style = WS_CHILD | WS_VISIBLE | SS_LEFT;
		dlit.dwExtendedStyle = 0;
		dlit.x = curPt.x; dlit.y = curPt.y;
		dlit.cx = descSize.cx; dlit.cy = descSize.cy;
		dlit.id = IDC_STATIC_DESC(i);
		// TODO: Remove if the value widget is invalid?
		m_dlgBuilder.add(&dlit, WC_ORD_STATIC, desc_text);

		// Create the value widget.
		int field_cy = dlit.cy;	// Default row size.
		const DLU_PT pt_start = {curPt.x + descSize.cx, curPt.y};
		switch (desc->type) {
			case RomFields::RFT_STRING:
				// Create a read-only EDIT widget.
				// The STATIC control doesn't allow the user
				// to highlight and copy data.
				dlit.style = WS_CHILD | WS_VISIBLE | ES_READONLY;
				dlit.dwExtendedStyle = WS_EX_LEFT;
				dlit.x = pt_start.x; dlit.y = pt_start.y;
				dlit.cx = dlit_value_width; dlit.cy = field_cy;
				dlit.id = IDC_RFT_STRING(i);
				m_dlgBuilder.add(&dlit, WC_ORD_EDIT, data->str);
				break;

			case RomFields::RFT_BITFIELD: {
				// NOTE: Although we can use dialog metrics to create the
				// bitfield checkboxes here, the result is a mess due to
				// DLUs not necessarily being a 1:1 mapping to pixels,
				// so some rows don't have the same spacing as others.
				// Just reserve the space for the rows here.
				const RomFields::BitfieldDesc *bitfieldDesc = desc->bitfield;
				int rows = 1;
				if (bitfieldDesc->elemsPerRow > 0) {
					// Multiple rows.
					// Determine how many rows we need.
					int cols = 0;
					for (int j = 0; j < bitfieldDesc->elements; j++) {
						if (!bitfieldDesc->names[i])
							continue;
						if (cols >= bitfieldDesc->elemsPerRow) {
							// Next row.
							rows++;
							cols = 0;
						}

						// Next column.
						cols++;
					}
				}

				field_cy *= rows;
				field_cy -= (rows / 2);
				break;
			}

			case RomFields::RFT_LISTDATA:
				// Create a ListView widget.
				// TODO: Best width/height?
				field_cy = 72;
				dlit.style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_ALIGNLEFT | LVS_REPORT;
				dlit.dwExtendedStyle = WS_EX_LEFT;
				dlit.x = pt_start.x; dlit.y = pt_start.y;
				dlit.cx = dlit_value_width; dlit.cy = field_cy;
				dlit.id = IDC_RFT_LISTDATA(i);
				m_dlgBuilder.add(&dlit, WC_LISTVIEW, L"");
				break;

			default:
				// FIXME: Implement the rest, then uncomment this assert.
				//assert(false);
				// TODO: Remove the description STATIC control.
				break;
		}

		// Next row.
		curPt.y += field_cy;
	}

	return m_dlgBuilder.get();
}

/**
 * Initialize a bitfield layout.
 * @param hDlg Dialog window.
 * @param pt_start Starting position, in pixels.
 * @param idx Field index.
 */
void RP_ShellPropSheetExt::initBitfield(HWND hDlg, const POINT &pt_start, int idx)
{
	if (!hDlg)
		return;

	const RomFields *fields = m_romData->fields();
	if (!fields)
		return;

	const RomFields::Desc *desc = fields->desc(idx);
	const RomFields::Data *data = fields->data(idx);
	if (!desc || !data)
		return;
	if (desc->type != RomFields::RFT_BITFIELD ||
	    data->type != RomFields::RFT_BITFIELD)
		return;
	if (!desc->name || desc->name[0] == '\0')
		return;

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
			std::wstring s_name = RP2W_c(name);

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
			std::wstring s_name = RP2W_c(name);

			// Get the width of this specific entry.
			SIZE textSize;
			GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
			chk_w = rect_chkbox.right + textSize.cx;
		} else {
			// Use the largest width in the column.
			chk_w = col_widths[col];
		}

		// FIXME: Tab ordering?
		HWND hCheckBox = CreateWindow(WC_BUTTON, RP2W_c(name),
			WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
			pt.x, pt.y, chk_w, rect_chkbox.bottom,
			hDlg, (HMENU)(IDC_RFT_BITFIELD(idx, j)),
			nullptr, nullptr);
		SetWindowFont(hCheckBox, hFont, FALSE);

		// Set the checkbox state.
		Button_SetCheck(hCheckBox, (data->bitfield & (1 << j)) ? BST_CHECKED : BST_UNCHECKED);

		// Next column.
		pt.x += chk_w;
		col++;
		if (col == bitfieldDesc->elemsPerRow) {
			// Next row.
			row++;
			col = 0;
			pt.x = pt_start.x;
			pt.y += rect_chkbox.bottom;
		}
	}

	SelectFont(hDC, hFontOrig);
	ReleaseDC(hDlg, hDC);
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
 * Initialize various fields in a dialog.
 * @param hDlg Dialog window.
 */
void RP_ShellPropSheetExt::initFields(HWND hDlg)
{
	const RomFields *fields = m_romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}

	const int count = fields->count();
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		HWND hDlgItem;
		switch (desc->type) {
			case RomFields::RFT_BITFIELD:
				// Get the position of the description label.
				hDlgItem = GetDlgItem(hDlg, IDC_STATIC_DESC(i));
				if (hDlgItem) {
					RECT lblRect; POINT pt_start;
					GetWindowRect(hDlgItem, &lblRect);
					// NOTE: .bottom/.right is 1px outside of
					// the control's rectangle.
					pt_start.x = lblRect.right;
					pt_start.y = lblRect.top;
					ScreenToClient(hDlg, &pt_start);
					initBitfield(hDlg, pt_start, i);
				}
				break;

			case RomFields::RFT_LISTDATA:
				// Check if we have a matching dialog item.
				hDlgItem = GetDlgItem(hDlg, IDC_RFT_LISTDATA(i));
				if (hDlgItem) {
					initListView(hDlgItem, desc, fields->data(i));
				}
				break;
		}
	}
}

/** IShellPropSheetExt **/

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7

	// Initialize the dialog template.
	LPCDLGTEMPLATE pDlgTemplate = initDialog();
	if (!pDlgTemplate) {
		// No property sheet is available.
		return E_FAIL;
	}

	// Create a property sheet page.
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_DLGINDIRECT;
	psp.hInstance = nullptr;
	psp.pResource = pDlgTemplate;
	psp.pszIcon = nullptr;
	psp.pszTitle = nullptr;
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

					// Initialize various data fields.
					pExt->initFields(hWnd);
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
