/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt_ops.cpp: IShellPropSheetExt implementation.        *
 * (ROM operations)                                                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "RP_ShellPropSheetExt.hpp"
#include "RP_ShellPropSheetExt_p.hpp"
#include "res/resource.h"

// Custom controls
#include "LanguageComboBox.hpp"
#include "MessageWidget.hpp"
#include "OptionsMenuButton.hpp"

// Other rom-properties libraries
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
#include <fstream>
#include <sstream>
using std::ofstream;
using std::ostringstream;
using std::string;
using std::vector;
using std::wstring;	// for tstring

/**
 * Adjust tabs for the message widget.
 * Message widget must have been created first.
 * @param bVisible True for visible; false for not.
 */
void RP_ShellPropSheetExt_Private::adjustTabsForMessageWidgetVisibility(bool bVisible)
{
	// NOTE: IsWindowVisible(hMessageWidget) doesn't seem to be
	// correct when this function is called, so we have to take
	// the visibility as a parameter instead.
	RECT rectMsgw;
	GetClientRect(hMessageWidget, &rectMsgw);
	int tab_h = iTabHeightOrig;
	if (bVisible) {
		tab_h -= rectMsgw.bottom;
	}

	for (const tab &tab : tabs) {
		RECT tabRect;
		GetClientRect(tab.hDlg, &tabRect);
		if (tabRect.bottom != tab_h) {
			SetWindowPos(tab.hDlg, nullptr, 0, 0, tabRect.right, tab_h,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
	}
}

/**
 * Show the message widget.
 * Message widget must have been created first.
 * @param messageType Message type.
 * @param lpszMsg Message.
 */
void RP_ShellPropSheetExt_Private::showMessageWidget(unsigned int messageType, const TCHAR *lpszMsg)
{
	assert(hMessageWidget != nullptr);
	if (!hMessageWidget)
		return;

	// Set the message widget stuff.
	MessageWidget_SetMessageType(hMessageWidget, messageType);
	SetWindowText(hMessageWidget, lpszMsg);

	adjustTabsForMessageWidgetVisibility(true);
	ShowWindow(hMessageWidget, SW_SHOW);
}

/**
 * Dialog subclass procedure to intercept WM_COMMAND for the "Options" button.
 * @param hWnd		Dialog handle
 * @param uMsg		Message
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID (usually the control ID)
 * @param dwRefData	RP_ShellPropSheetExt_Private*
 */
LRESULT CALLBACK RP_ShellPropSheetExt_Private::MainDialogSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	// FIXME: Move this to OptionsMenuButton.
	switch (uMsg) {
		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, MainDialogSubclassProc, uIdSubclass);
			break;

		case WM_COMMAND: {
			if (HIWORD(wParam) != BN_CLICKED)
				break;
			if (LOWORD(wParam) != IDC_RP_OPTIONS)
				break;

			// Pop up the menu.
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(dwRefData);
			assert(d->hBtnOptions != nullptr);
			if (!d->hBtnOptions)
				break;
			const int menuId = OptionsMenuButton_PopupMenu(d->hBtnOptions);
			if (menuId != 0) {
				d->btnOptions_action_triggered(menuId);
			}
			return TRUE;
		}

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Create the "Options" button in the parent window.
 * Called by WM_INITDIALOG.
 */
void RP_ShellPropSheetExt_Private::createOptionsButton(void)
{
	assert(hDlgSheet != nullptr);
	assert(romData != nullptr);
	if (!hDlgSheet || !romData) {
		// No dialog, or no ROM data loaded.
		return;
	}

	HWND hWndParent = GetParent(hDlgSheet);
	assert(hWndParent != nullptr);
	if (!hWndParent) {
		// No parent window...
		return;
	}

	// is the "Options" button already present?
	if (GetDlgItem(hWndParent, IDC_RP_OPTIONS) != nullptr) {
		assert(!"IDC_RP_OPTIONS is already created.");
		return;
	}

	// TODO: Verify RTL positioning.
	HWND hBtnOK = GetDlgItem(hWndParent, IDOK);
	HWND hTabControl = PropSheet_GetTabControl(hWndParent);
	if (!hBtnOK || !hTabControl) {
		return;
	}

	RECT rect_btnOK, rect_tabControl;
	GetWindowRect(hBtnOK, &rect_btnOK);
	GetWindowRect(hTabControl, &rect_tabControl);
	MapWindowPoints(HWND_DESKTOP, hWndParent, (LPPOINT)&rect_btnOK, 2);
	MapWindowPoints(HWND_DESKTOP, hWndParent, (LPPOINT)&rect_tabControl, 2);

	// Create the "Options" button.
	OptionsMenuButtonRegister();
	POINT ptBtn = {rect_tabControl.left, rect_btnOK.top};
	const SIZE szBtn = {
		rect_btnOK.right - rect_btnOK.left,
		rect_btnOK.bottom - rect_btnOK.top
	};
	hBtnOptions = CreateWindowEx(dwExStyleRTL,
		WC_OPTIONSMENUBUTTON, nullptr,	// OptionsMenuButton will set the text.
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_PUSHBUTTON | BS_CENTER,
		ptBtn.x, ptBtn.y, szBtn.cx, szBtn.cy,
		hWndParent, (HMENU)IDC_RP_OPTIONS, nullptr, nullptr);
	SetWindowFont(hBtnOptions, GetWindowFont(hDlgSheet), FALSE);

	// Initialize the "Options" submenu.
	OptionsMenuButton_ReinitMenu(hBtnOptions, romData.get());

	// Fix up the tab order. ("Options" should be after "Apply".)
	HWND hBtnApply = GetDlgItem(hWndParent, IDC_APPLY_BUTTON);
	if (hBtnApply) {
		SetWindowPos(hBtnOptions, hBtnApply,
			0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	// Subclass the parent dialog so we can intercept WM_COMMAND.
	SetWindowSubclass(hWndParent, MainDialogSubclassProc,
		static_cast<UINT_PTR>(IDC_RP_OPTIONS),
		reinterpret_cast<DWORD_PTR>(this));
}

/**
 * Update a field's value.
 * This is called after running a ROM operation.
 * @param fieldIdx Field index.
 * @return 0 on success; non-zero on error.
 */
int RP_ShellPropSheetExt_Private::updateField(int fieldIdx)
{
	const RomFields *const pFields = romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return 1;
	}

	assert(fieldIdx >= 0);
	assert(fieldIdx < pFields->count());
	if (fieldIdx < 0 || fieldIdx >= pFields->count())
		return 2;

	const RomFields::Field *const field = pFields->at(fieldIdx);
	assert(field != nullptr);
	if (!field)
		return 3;

	// Get the tab dialog control the control is in.
	assert(field->tabIdx >= 0);
	assert(field->tabIdx < tabs.size());
	if (field->tabIdx < 0 || field->tabIdx >= tabs.size())
		return 4;
	HWND hDlg = tabs[field->tabIdx].hDlg;

	// Update the value widget(s).
	int ret;
	switch (field->type) {
		case RomFields::RFT_INVALID:
			assert(!"Cannot update an RFT_INVALID field.");
			ret = 5;
			break;
		default:
			assert(!"Unsupported field type.");
			ret = 6;
			break;

		case RomFields::RFT_STRING: {
			// HWND is a STATIC control.
			HWND hLabel = GetDlgItem(hDlg, IDC_RFT_STRING(fieldIdx));
			assert(hLabel != nullptr);
			if (!hLabel) {
				ret = 7;
				break;
			}

			if (field->data.str) {
				const tstring ts_text = LibWin32UI::unix2dos(U82T_s(field->data.str));
				SetWindowText(hLabel, ts_text.c_str());
			} else {
				SetWindowText(hLabel, _T(""));
			}
			ret = 0;
			break;
		}

		case RomFields::RFT_BITFIELD: {
			// Multiple checkboxes with unique dialog IDs.

			// Bits with a blank name aren't included, so we'll need to iterate
			// over the bitfield description.
			const auto &bitfieldDesc = field->desc.bitfield;
			int count = (int)bitfieldDesc.names->size();
			assert(count <= 32);
			if (count > 32)
				count = 32;

			// Unlike GTK+ and KDE, we don't need to check bitfieldDesc.names to determine
			// if a checkbox is present, since GetDlgItem() will return nullptr in that case.
			uint32_t bitfield = field->data.bitfield;
			int id = IDC_RFT_BITFIELD(fieldIdx, 0);
			for (; count >= 0; count--, id++, bitfield >>= 1) {
				HWND hCheckBox = GetDlgItem(hDlg, id);
				if (!hCheckBox)
					continue;

				// Set the checkbox.
				Button_SetCheck(hCheckBox, (bitfield & 1) ? BST_CHECKED : BST_UNCHECKED);
			}
			ret = 0;
			break;
		}
	}

	return ret;
}

/**
 * An "Options" menu button action was triggered.
 * @param menuId Menu ID. (Options ID + IDM_OPTIONS_MENU_BASE)
 */
void RP_ShellPropSheetExt_Private::btnOptions_action_triggered(int menuId)
{
	if (menuId < IDM_OPTIONS_MENU_BASE) {
		// Export to text or JSON.
		const char *const rom_filename = romData->filename();
		if (!rom_filename)
			return;

		const unsigned int id2 = static_cast<unsigned int>(abs(menuId - IDM_OPTIONS_MENU_BASE)) - 1;
		assert(id2 < 4);
		if (id2 >= 4) {
			// Out of range.
			return;
		}

		struct StdActsInfo_t {
			const char *title;	// NOP_C_
			const char *filter;	// NOP_C_
			TCHAR default_ext[7];
			bool toClipboard;
		};
		static const StdActsInfo_t stdActsInfo[] = {
			// OPTION_EXPORT_TEXT
			{NOP_C_("RomDataView", "Export to Text File"),
			 // tr: "Text Files" filter (RP format)
			 NOP_C_("RomDataView", "Text Files|*.txt|text/plain|All Files|*|-"),
			 _T(".txt"), false},

			// OPTION_EXPORT_JSON
			{NOP_C_("RomDataView", "Export to JSON File"),
			 // tr: "JSON Files" filter (RP format)
			 NOP_C_("RomDataView", "JSON Files|*.json|application/json|All Files|*|-"),
			 _T(".json"), false},

			// OPTION_COPY_TEXT
			{nullptr, nullptr, _T(""), true},

			// OPTION_COPY_JSON
			{nullptr, nullptr, _T(""), true},
		};

		// Standard Actions information for this action
		const StdActsInfo_t *const info = &stdActsInfo[id2];

		ofstream ofs;
		tstring ts_out;

		if (!info->toClipboard) {
			if (ts_prevExportDir.empty()) {
				ts_prevExportDir = U82T_c(rom_filename);

				// Remove the filename portion.
				const size_t bspos = ts_prevExportDir.rfind(_T('\\'));
				if (bspos != string::npos) {
					if (bspos > 2) {
						ts_prevExportDir.resize(bspos);
					} else if (bspos == 2) {
						ts_prevExportDir.resize(3);
					}
				}
			}

			tstring defaultFileName = ts_prevExportDir;
			if (!defaultFileName.empty() && defaultFileName.at(defaultFileName.size()-1) != _T('\\')) {
				defaultFileName += _T('\\');
			}

			// Get the base name of the ROM.
			tstring rom_basename;
			const char *const bspos = strrchr(rom_filename, '\\');
			if (bspos) {
				rom_basename = U82T_c(bspos+1);
			} else {
				rom_basename = U82T_c(rom_filename);
			}
			// Remove the extension, if present.
			const size_t extpos = rom_basename.rfind(_T('.'));
			if (extpos != string::npos) {
				rom_basename.resize(extpos);
			}
			defaultFileName += rom_basename;
			if (info->default_ext[0] != _T('\0')) {
				defaultFileName += info->default_ext;
			}

			const tstring tfilename = LibWin32UI::getSaveFileName(hDlgSheet,
				U82T_c(dpgettext_expr(RP_I18N_DOMAIN, "RomDataView", info->title)),
				U82T_c(dpgettext_expr(RP_I18N_DOMAIN, "RomDataView", info->filter)),
				defaultFileName.c_str());
			if (tfilename.empty())
				return;

			// Save the previous export directory.
			ts_prevExportDir = tfilename;
			const size_t bspos2 = ts_prevExportDir.rfind(_T('\\'));
			if (bspos2 != tstring::npos && bspos2 > 3) {
				ts_prevExportDir.resize(bspos2);
			}

#ifdef HAVE_OFSTREAM_CTOR_WCHAR_T
			ofs.open(tfilename.c_str(), ofstream::out);
#else /* !HAVE_OFSTREAM_CTOR_WCHAR_T */
			// FIXME: Convert to ANSI, not UTF-8.
			ofs.open(T2U8(tfilename).c_str(), ofstream::out);
#endif /* HAVE_OFSTREAM_CTOR_WCHAR_T */
			if (ofs.fail())
				return;
		}

		// TODO: Optimize this such that we can pass ofstream or ostringstream
		// to a factored-out function.

		switch (menuId) {
			case IDM_OPTIONS_MENU_EXPORT_TEXT: {
				const uint32_t lc = cboLanguage
					? LanguageComboBox_GetSelectedLC(cboLanguage)
					: 0;
				ofs << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << '\n';
				ROMOutput ro(romData.get(), lc);
				ofs << ro;
				ofs.flush();
				break;
			}
			case IDM_OPTIONS_MENU_EXPORT_JSON: {
				JSONROMOutput jsro(romData.get());
				ofs << jsro << '\n';
				ofs.flush();
				break;
			}
			case IDM_OPTIONS_MENU_COPY_TEXT: {
				// NOTE: Some fields may have embedded newlines,
				// so we'll need to convert everything afterwards.
				const uint32_t lc = cboLanguage
					? LanguageComboBox_GetSelectedLC(cboLanguage)
					: 0;
				ostringstream oss;
				oss << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << '\n';
				ROMOutput ro(romData.get(), lc);
				oss << ro;
				oss.flush();
				ts_out = LibWin32UI::unix2dos(U82T_s(oss.str()));
				break;
			}
			case IDM_OPTIONS_MENU_COPY_JSON: {
				ostringstream oss;
				JSONROMOutput jsro(romData.get());
				jsro.setCrlf(true);
				oss << jsro << '\n';
				oss.flush();
				ts_out = U82T_s(oss.str());
				break;
			}
			default:
				assert(!"Invalid action ID.");
				return;
		}

		if (info->toClipboard && OpenClipboard(hDlgSheet)) {
			EmptyClipboard();
			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (ts_out.size() + 1) * sizeof(TCHAR));
			if (hglbCopy) {
				LPTSTR lpszCopy = static_cast<LPTSTR>(GlobalLock(hglbCopy));
				memcpy(lpszCopy, ts_out.data(), ts_out.size() * sizeof(TCHAR));
				lpszCopy[ts_out.size()] = _T('\0');
				GlobalUnlock(hglbCopy);
				SetClipboardData(CF_UNICODETEXT, hglbCopy);
			}
			CloseClipboard();
		}
		return;
	}

	// Run a ROM operation.
	// TODO: Don't keep rebuilding this vector...
	vector<RomData::RomOp> ops = romData->romOps();
	const int id = menuId - IDM_OPTIONS_MENU_BASE;
	assert(id < (int)ops.size());
	if (id >= (int)ops.size()) {
		// ID is out of range.
		return;
	}

	string s_save_filename;
	RomData::RomOpParams params;
	const RomData::RomOp *op = &ops[id];
	if (op->flags & RomData::RomOp::ROF_SAVE_FILE) {
		// Add the "All Files" filter.
		string filter = op->sfi.filter;
		if (!filter.empty()) {
			// Make sure the last field isn't empty.
			if (filter.at(filter.size()-1) == '|') {
				filter += '-';
			}
			filter += '|';
		}
		// tr: "All Files" filter (RP format)
		filter += C_("RomData", "All Files|*|-");

		// Initial file and directory, based on the current file.
		string initialFile = FileSystem::replace_ext(romData->filename(), op->sfi.ext);

		// Prompt for a save file.
		tstring t_save_filename = LibWin32UI::getSaveFileName(hDlgSheet,
			U82T_c(op->sfi.title), U82T_s(filter), U82T_s(initialFile));
		if (t_save_filename.empty())
			return;
		s_save_filename = T2U8(t_save_filename);
		params.save_filename = s_save_filename.c_str();
	}

	const int ret = romData->doRomOp(id, &params);
	unsigned int messageType;
	if (ret == 0) {
		// ROM operation completed.
		messageType = MB_ICONINFORMATION;

		// Update fields.
		for (const int fieldIdx : params.fieldIdx) {
			this->updateField(fieldIdx);
		}

		// Update the RomOp menu entry in case it changed.
		// NOTE: Assuming the RomOps vector order hasn't changed.
		ops = romData->romOps();
		assert(id < (int)ops.size());
		if (id < (int)ops.size()) {
			OptionsMenuButton_UpdateOp(hBtnOptions, id, &ops[id]);
		}
	} else {
		// An error occurred...
		// TODO: Show an error message.
		messageType = MB_ICONWARNING;
	}

	if (!params.msg.empty()) {
		MessageBeep(messageType);

		if (!hMessageWidget) {
			// FIXME: Make sure this works if multiple tabs are present.
			MessageWidgetRegister();

			// Align to the bottom of the dialog and center-align the text.
			// 7x7 DLU margin is recommended by the Windows UX guidelines.
			// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
			RECT tmpRect = {7, 7, 8, 8};
			MapDialogRect(hDlgSheet, &tmpRect);
			RECT winRect;
			GetClientRect(hDlgSheet, &winRect);
			// NOTE: We need to move left by 1px.
			OffsetRect(&winRect, -1, 0);

			// Determine the position.
			// TODO: Update on DPI change.
			const int cySmIcon = GetSystemMetrics(SM_CYSMICON);
			SIZE szMsgw = {winRect.right - winRect.left, cySmIcon + 8};
			POINT ptMsgw = {winRect.left, winRect.bottom - szMsgw.cy};
			if (tabs.size() > 1) {
				ptMsgw.x += tmpRect.left;
				ptMsgw.y -= tmpRect.top;
				szMsgw.cx -= (tmpRect.left * 2);
			} else {
				ptMsgw.x += (tmpRect.left / 2);
				ptMsgw.y -= (tmpRect.top / 2);
				szMsgw.cx -= tmpRect.left;
			}

			hMessageWidget = CreateWindowEx(
				WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
				WC_MESSAGEWIDGET, nullptr,
				WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
				ptMsgw.x, ptMsgw.y, szMsgw.cx, szMsgw.cy,
				hDlgSheet, (HMENU)IDC_MESSAGE_WIDGET,
				HINST_THISCOMPONENT, nullptr);
			SetWindowFont(hMessageWidget, GetWindowFont(hDlgSheet), false);
		}

		showMessageWidget(messageType, U82T_s(params.msg));
	}
}
