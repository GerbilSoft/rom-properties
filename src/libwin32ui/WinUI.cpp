/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * WinUI.hpp: Windows UI common functions.                                 *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "WinUI.hpp"
#include "AutoGetDC.hpp"

#include <commdlg.h>
#include <shlwapi.h>

#include <comdef.h>
#include <shobjidl.h>

// C++ includes
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::tstring;
using std::vector;

// COM smart pointer typedefs
_COM_SMARTPTR_TYPEDEF(IFileDialog, IID_IFileDialog);
_COM_SMARTPTR_TYPEDEF(IShellItem, IID_IShellItem);

namespace LibWin32UI {

// NOTE: We can't easily link to LibWin32Common for U82T_c(),
// so we'll define it here.

/**
 * Mini U82W() function.
 * @param mbs UTF-8 string.
 * @return UTF-16 C++ wstring.
 */
#ifdef UNICODE
static wstring U82T_c(const char *mbs)
{
	tstring ts_ret;

	// NOTE: cchWcs includes the NULL terminator.
	int cchWcs = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 1) {
		return ts_ret;
	}
	cchWcs--;
 
	wchar_t *const wcs = new wchar_t[cchWcs];
	MultiByteToWideChar(CP_UTF8, 0, mbs, -1, wcs, cchWcs);
	ts_ret.assign(wcs, cchWcs);
	delete[] wcs;
	return ts_ret;
}
#else /* !UNICODE */
static inline string U82T_c(const char *mbs)
{
	// TODO: Convert UTF-8 to ANSI?
	return string(mbs);
}
#endif /* UNICODE */

/**
 * Convert UNIX line endings to DOS line endings.
 * @param tstr_unix	[in] String with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return tstring with DOS line endings.
 */
tstring unix2dos(const TCHAR *tstr_unix, int *lf_count)
{
	// TODO: Optimize this!
	tstring tstr_dos;
	tstr_dos.reserve(_tcslen(tstr_unix) + 16);
	int lf = 0;
	for (; *tstr_unix != _T('\0'); tstr_unix++) {
		if (*tstr_unix == _T('\n')) {
			tstr_dos += _T("\r\n");
			lf++;
		} else {
			tstr_dos += *tstr_unix;
		}
	}
	if (lf_count) {
		*lf_count = lf;
	}
	return tstr_dos;
}

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
int measureTextSize(HWND hWnd, HFONT hFont, const TCHAR *tstr, LPSIZE lpSize)
{
	assert(hWnd != nullptr);
	assert(hFont != nullptr);
	assert(tstr != nullptr);
	assert(lpSize != nullptr);
	if (!hWnd || !hFont || !tstr || !lpSize)
		return -EINVAL;

	SIZE size_total = {0, 0};
	AutoGetDC hDC(hWnd, hFont);

	// Find the next newline.
	do {
		const TCHAR *p_nl = _tcschr(tstr, L'\n');
		int len;
		if (p_nl) {
			// Found a newline.
			len = static_cast<int>(p_nl - tstr);
		} else {
			// No newline. Process the rest of the string.
			len = static_cast<int>(_tcslen(tstr));
		}
		assert(len >= 0);
		if (len < 0) {
			len = 0;
		}

		// Check if a '\r' is present before the '\n'.
		if (len > 0 && p_nl != nullptr && *(p_nl-1) == L'\r') {
			// Ignore the '\r'.
			len--;
		}

		// Measure the text size.
		SIZE size_cur;
		BOOL bRet = GetTextExtentPoint32(hDC, tstr, len, &size_cur);
		if (!bRet) {
			// Something failed...
			return -1;
		}

		if (size_cur.cx > size_total.cx) {
			size_total.cx = size_cur.cx;
		}
		size_total.cy += size_cur.cy;

		// Next newline.
		if (!p_nl)
			break;
		tstr = p_nl + 1;
	} while (*tstr != 0);

	*lpSize = size_total;
	return 0;
}

/**
 * Measure text size using GDI.
 * This version removes HTML-style tags before
 * calling the regular measureTextSize() function.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
int measureTextSizeLink(HWND hWnd, HFONT hFont, const TCHAR *tstr, LPSIZE lpSize)
{
	assert(tstr != nullptr);
	if (!tstr)
		return -EINVAL;

	// Remove HTML-style tags.
	// NOTE: This is a very simplistic version.
	size_t len = _tcslen(tstr);
	unique_ptr<TCHAR[]> ntstr(new TCHAR[len+1]);
	TCHAR *p = ntstr.get();

	int lbrackets = 0;
	for (; *tstr != 0; tstr++) {
		if (*tstr == _T('<')) {
			// Starting bracket.
			lbrackets++;
			continue;
		} else if (*tstr == _T('>')) {
			// Ending bracket.
			assert(lbrackets > 0);
			lbrackets--;
			continue;
		}

		if (lbrackets == 0) {
			// Not currently in a tag.
			*p++ = *tstr;
		}
	}
	assert(lbrackets == 0);

	*p = 0;
	return measureTextSize(hWnd, hFont, ntstr.get(), lpSize);
}

/**
 * Get the alternate row color for ListViews.
 *
 * This function should be called on ListView creation
 * and if the system theme is changed.
 *
 * @return Alternate row color for ListViews.
 */
COLORREF getAltRowColor(void)
{
	union {
		COLORREF color;
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
	} rgb;
	rgb.color = GetSysColor(COLOR_WINDOW);

	// TODO: Better "convert to grayscale" and brighten/darken algorithms?
	// TODO: Handle color component overflow.
	if (((rgb.r + rgb.g + rgb.b) / 3) >= 128) {
		// Subtract 16 from each color component.
		rgb.color -= 0x00101010U;
	} else {
		// Add 16 to each color component.
		rgb.color += 0x00101010U;
	}

	return rgb.color;
}

/**
 * Are we using COMCTL32.DLL v6.10 or later?
 * @return True if it's v6.10 or later; false if not.
 */
bool isComCtl32_v610(void)
{
	// Check the COMCTL32.DLL version.
	HMODULE hComCtl32 = GetModuleHandle(_T("COMCTL32"));
	assert(hComCtl32 != nullptr);
	if (!hComCtl32)
		return false;

	typedef HRESULT (CALLBACK *PFNDLLGETVERSION)(DLLVERSIONINFO *pdvi);
	PFNDLLGETVERSION pfnDllGetVersion = (PFNDLLGETVERSION)GetProcAddress(hComCtl32, "DllGetVersion");
	if (!pfnDllGetVersion)
		return false;

	bool ret = false;
	DLLVERSIONINFO dvi;
	dvi.cbSize = sizeof(dvi);
	HRESULT hr = pfnDllGetVersion(&dvi);
	if (SUCCEEDED(hr)) {
		ret = dvi.dwMajorVersion > 6 ||
			(dvi.dwMajorVersion == 6 && dvi.dwMinorVersion >= 10);
	}
	return ret;
}

/**
 * Measure the width of a string for ListView.
 * This function handles newlines.
 * @param hDC          [in] HDC for text measurement.
 * @param tstr         [in] String to measure.
 * @param pNlCount     [out,opt] Newline count.
 * @return Width. (May return LVSCW_AUTOSIZE_USEHEADER if it's a single line.)
 */
int measureStringForListView(HDC hDC, const tstring &tstr, int *pNlCount)
{
	// TODO: Actual padding value?
	static const int COL_WIDTH_PADDING = 8*2;

	// Measured width.
	int width = 0;

	// Count newlines.
	size_t prev_nl_pos = 0;
	size_t cur_nl_pos;
	int nl = 0;
	while ((cur_nl_pos = tstr.find(_T('\n'), prev_nl_pos)) != tstring::npos) {
		// Measure the width, plus padding on both sides.
		//
		// LVSCW_AUTOSIZE_USEHEADER doesn't work for entries with newlines.
		// This allows us to set a good initial size, but it won't help if
		// someone double-clicks the column splitter, triggering an automatic
		// resize.
		//
		// TODO: Use ownerdraw instead? (WM_MEASUREITEM / WM_DRAWITEM)
		// NOTE: Not using LibWin32UI::measureTextSize()
		// because that does its own newline checks.
		// TODO: Verify the values here.
		SIZE textSize;
		GetTextExtentPoint32(hDC, &tstr[prev_nl_pos], (int)(cur_nl_pos - prev_nl_pos), &textSize);
		width = std::max<int>(width, textSize.cx + COL_WIDTH_PADDING);

		nl++;
		prev_nl_pos = cur_nl_pos + 1;
	}

	// Measure the last line.
	// TODO: Verify the values here.
	SIZE textSize;
	GetTextExtentPoint32(hDC, &tstr[prev_nl_pos], (int)(tstr.size() - prev_nl_pos), &textSize);
	width = std::max<int>(width, textSize.cx + COL_WIDTH_PADDING);

	if (pNlCount) {
		*pNlCount = nl;
	}

	return width;
}

/**
 * Is the system using an RTL language?
 * @return WS_EX_LAYOUTRTL if the system is using RTL; 0 if not.
 */
DWORD isSystemRTL(void)
{
	// Check for RTL.
	// NOTE: Windows Explorer on Windows 7 seems to return 0 from GetProcessDefaultLayout(),
	// even if an RTL language is in use. We'll check the taskbar layout instead.
	// TODO: What if Explorer isn't running?
	// References:
	// - https://stackoverflow.com/questions/10391669/how-to-detect-if-a-windows-installation-is-rtl
	// - https://stackoverflow.com/a/10393376
	DWORD dwRet = 0;
	HWND hTaskBar = FindWindow(_T("Shell_TrayWnd"), nullptr);
	if (hTaskBar) {
		dwRet = static_cast<DWORD>(GetWindowLongPtr(hTaskBar, GWL_EXSTYLE)) & WS_EX_LAYOUTRTL;
	}
	return dwRet;
}

/** File dialogs **/

#ifdef UNICODE
/**
 * Get a filename using IFileDialog.
 * @param ts_ret	[out] Output filename (Empty if none.)
 * @param bSave		[in] True for save; false for open
 * @param hWnd		[in] Owner
 * @param dlgTitle	[in] Dialog title
 * @param filterSpec	[in] Filter specification (pipe-delimited)
 * @param origFilename	[in,opt] Starting filename
 * @return 0 on success; HRESULT on error.
 */
static inline HRESULT getFileName_int_IFileDialog(tstring &ts_ret, bool bSave, HWND hWnd,
	const TCHAR *dlgTitle, const char8_t *filterSpec, const TCHAR *origFilename)
{
	IFileDialogPtr pFileDlg;
	HRESULT hr = CoCreateInstance(
		bSave ? CLSID_FileSaveDialog : CLSID_FileOpenDialog,
		nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pFileDlg));
	if (FAILED(hr)) {
		// Assume the class isn't available.
		return REGDB_E_CLASSNOTREG;
	}

	// Convert the filter spec to COMDLG_FILTERSPEC.
	vector<COMDLG_FILTERSPEC> v_cdfs;

	// Convert '|' characters to NULL bytes.
	// _tcstok_s() can do this for us.
	tstring tmpfilter = U82T_c(reinterpret_cast<const char*>(filterSpec));
	TCHAR *saveptr = nullptr;
	TCHAR *token = _tcstok_s(&tmpfilter[0], _T("|"), &saveptr);
	while (token != nullptr) {
		COMDLG_FILTERSPEC cdfs;

		// Separator 1: Between display name and pattern.
		// (_tcstok_s() call was done in the previous iteration.)
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			return E_INVALIDARG;
		}
		cdfs.pszName = token;

		// Separator 2: Between pattern and MIME types.
		token = _tcstok_s(nullptr, _T("|"), &saveptr);
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			return E_INVALIDARG;
		}
		cdfs.pszSpec = token;
		v_cdfs.emplace_back(std::move(cdfs));

		// Separator 3: Between MIME types and the next display name.
		// NOTE: May be missing if this is the end of the string
		// and a MIME type isn't set.
		// NOTE: Not used by Qt.
		token = _tcstok_s(nullptr, _T("|"), &saveptr);

		// Next token.
		token = _tcstok_s(nullptr, _T("|"), &saveptr);
	}

	if (v_cdfs.empty()) {
		// No filter spec...
		return E_INVALIDARG;
	}

	hr = pFileDlg->SetFileTypes(static_cast<int>(v_cdfs.size()), v_cdfs.data());
	v_cdfs.clear();
	if (FAILED(hr))
		return hr;

	// Check if the original filename is a directory or a file.
	if (origFilename && origFilename[0] != _T('\0')) {
		// NOTE: SHCreateItemFromParsingName() was added in Vista, so we
		// need to use GetProcAddress().
		typedef HRESULT(STDAPICALLTYPE* PFNSHCREATEITEMFROMPARSINGNAME)(
			PCWSTR pszPath, IBindCtx* pbc, REFIID riid, void** ppv);
		HMODULE hShell32 = GetModuleHandle(_T("shell32"));
		assert(hShell32 != nullptr);
		if (!hShell32)
			return E_FAIL;

		PFNSHCREATEITEMFROMPARSINGNAME pfnSHCreateItemFromParsingName =
			reinterpret_cast<PFNSHCREATEITEMFROMPARSINGNAME>(
				GetProcAddress(hShell32, "SHCreateItemFromParsingName"));
		assert(pfnSHCreateItemFromParsingName != nullptr);
		if (!pfnSHCreateItemFromParsingName)
			return E_FAIL;

		DWORD dwAttrs = GetFileAttributes(origFilename);
		IShellItemPtr pFolder;
		if (dwAttrs != INVALID_FILE_ATTRIBUTES && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY)) {
			// It's a directory.
			hr = pfnSHCreateItemFromParsingName(origFilename, nullptr, IID_PPV_ARGS(&pFolder));
			if (FAILED(hr))
				return hr;
		} else {
			// It's a filename, or invalid. Get the directory portion,
			// for IFileDialog::SetFolder(), then set the
			// filename portion with IFileDialog::SetFileName().
			const TCHAR *bs = _tcsrchr(origFilename, _T('\\'));
			if (!bs) {
				// No backslash. Use the whole filename.
				hr = pFileDlg->SetFileName(origFilename);
				if (FAILED(hr))
					return hr;
			} else {
				// Set the filename.
				hr = pFileDlg->SetFileName(bs + 1);
				if (FAILED(hr))
					return hr;

				// Get the folder portion.
				tstring ts_folder(origFilename, bs - origFilename);
				hr = pfnSHCreateItemFromParsingName(ts_folder.c_str(),
					nullptr, IID_PPV_ARGS(&pFolder));
				if (FAILED(hr))
					return hr;
			}

			// Set the directory.
			hr = pFileDlg->SetFolder(pFolder);
			if (FAILED(hr))
				return hr;
		}
	}

	hr = pFileDlg->SetTitle(dlgTitle);
	if (FAILED(hr))
		return hr;
	hr = pFileDlg->SetOptions(bSave
		? FOS_DONTADDTORECENT | FOS_OVERWRITEPROMPT
		: FOS_DONTADDTORECENT | FOS_FILEMUSTEXIST);
	if (FAILED(hr))
		return hr;
	hr = pFileDlg->Show(hWnd);
	if (FAILED(hr)) {
		// User cancelled the dialog box.
		return hr;
	}

	IShellItemPtr pShellItem;
	hr = pFileDlg->GetResult(&pShellItem);
	if (FAILED(hr)) {
		// Unable to get the result...
		return hr;
	}
	PWSTR pszFilePath;
	hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
	if (FAILED(hr)) {
		// Unable to get the filename.
		return hr;
	}

	// Filename retrieved.
	ts_ret = pszFilePath;
	CoTaskMemFree(pszFilePath);
	return S_OK;
}
#endif /* UNICODE */

/**
 * Convert an RP file dialog filter to Win32.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*.*|-"
 * Similar the same as Windows, but with '|' instead of '\0'.
 * Also, no terminator sequence is needed.
 * The "(*.bin; *.srl)" part is added to the display name if needed.
 * A third segment provides for semicolon-separated MIME types. (May be "-" for 'any'.)
 *
 * @param filter RP file dialog filter. (UTF-8, from gettext())
 * @return Win32 file dialog filter.
 */
static tstring rpFileDialogFilterToWin32(const char8_t *filter)
{
	tstring ts_ret;
	assert(filter != nullptr && filter[0] != '\0');
	if (!filter || filter[0] == '\0')
		return ts_ret;

	// TODO: Change `filter` to `const TCHAR*` to eliminate
	// some temporary strings?

	// Temporary string so we can use strtok_r().
	char *const tmpfilter = strdup(reinterpret_cast<const char*>(filter));
	assert(tmpfilter != nullptr);
	char *saveptr = nullptr;

	// First strtok_r() call.
	ts_ret.reserve(strlen(tmpfilter) + 32);
	char *token = strtok_s(tmpfilter, "|", &saveptr);
	do {
		// Separator 1: Between display name and pattern.
		// (strtok_r() call was done in the previous iteration.)
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			free(tmpfilter);
			return tstring();
		}
		const size_t lastpos = ts_ret.size();
		ts_ret += U82T_c(token);

		// Separator 2: Between pattern and MIME types.
		token = strtok_s(nullptr, "|", &saveptr);
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			free(tmpfilter);
			return tstring();
		}

		// Don't append the pattern in parentheses if it's
		// the same as the display name. (e.g. for specific
		// files in KeyManagerTab)
		const tstring ts_tmpstr = U82T_c(token);
		if (ts_ret.compare(lastpos, string::npos, ts_tmpstr) != 0) {
			ts_ret += _T(" (");
			ts_ret += ts_tmpstr;
			ts_ret += _T(')');
		}
		ts_ret += _T('\0');

		// File filter.
		ts_ret += ts_tmpstr;
		ts_ret += _T('\0');

		// Separator 3: Between MIME types and the next display name.
		// NOTE: May be missing if this is the end of the string
		// and a MIME type isn't set.
		// NOTE: Not used by Qt.
		token = strtok_s(nullptr, "|", &saveptr);

		// Next token.
		token = strtok_s(nullptr, "|", &saveptr);
	} while (token != nullptr);

	free(tmpfilter);
	ts_ret += _T('\0');
	return ts_ret;
}

/**
 * Get a filename using a File Name dialog.
 *
 * Depending on OS, this may use:
 * - Vista+: IOpenFileDialog / IFileSaveDialog
 * - XP: GetOpenFileName() / GetSaveFileName()
 *
 * @param bSave		[in] True for save; false for open
 * @param hWnd		[in] Owner
 * @param dlgTitle	[in] Dialog title
 * @param filterSpec	[in] Filter specification (RP format, UTF-8)
 * @param origFilename	[in,opt] Starting filename
 * @return Filename, or empty string on error.
 */
static tstring getFileName_int(bool bSave, HWND hWnd,
	const TCHAR *dlgTitle, const char8_t *filterSpec, const TCHAR *origFilename)
{
	assert(dlgTitle != nullptr);
	assert(filterSpec != nullptr);
	tstring ts_ret;

#ifdef UNICODE
	// Try IFileDialog first. (Unicode only)
	HRESULT hr = getFileName_int_IFileDialog(ts_ret, bSave,
		hWnd, dlgTitle, filterSpec, origFilename);
	if (hr != REGDB_E_CLASSNOTREG) {
		// IFileDialog succeeded.
		return ts_ret;
	}
#endif /* UNICODE */

	// GetOpenFileName() / GetSaveFileName()
	tstring ts_filterSpec = rpFileDialogFilterToWin32(filterSpec);

	TCHAR tfilename[MAX_PATH];
	tfilename[0] = 0;

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = ts_filterSpec.c_str();
	ofn.lpstrCustomFilter = nullptr;
	ofn.lpstrFile = tfilename;
	ofn.nMaxFile = _countof(tfilename);
	ofn.lpstrTitle = dlgTitle;

	// Check if the original filename is a directory or a file.
	if (origFilename && origFilename[0] != _T('\0')) {
		DWORD dwAttrs = GetFileAttributes(origFilename);
		if (dwAttrs != INVALID_FILE_ATTRIBUTES && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY)) {
			// It's a directory.
			ofn.lpstrInitialDir = origFilename;
		} else {
			// Not a directory, or invalid.
			// Assume it's a filename.
			ofn.lpstrInitialDir = nullptr;
			_tcscpy_s(tfilename, _countof(tfilename), origFilename);
		}
	}

	// TODO: Make OFN_DONTADDTORECENT customizable?
	// FIXME: OFN_NOCHANGEDIR becomes OFN_NO-CHANGE-DIR?
	BOOL bRet;
	if (!bSave) {
		ofn.Flags = OFN_DONTADDTORECENT |
		            OFN_HIDEREADONLY |
		            //OFN_NO­CHANGE­DIR |
		            OFN_FILEMUSTEXIST |
		            OFN_PATHMUSTEXIST;
		bRet = GetOpenFileName(&ofn);
	} else {
		ofn.Flags = OFN_DONTADDTORECENT |
		            OFN_HIDEREADONLY |
		            //OFN_NOCHANGEDIR |
		            OFN_OVERWRITEPROMPT;
		bRet = GetSaveFileName(&ofn);
	}

	if (bRet != 0 && tfilename[0] != _T('\0')) {
		ts_ret = tfilename;
	}
	return ts_ret;
}

/**
 * Get a filename using the Open File Name dialog.
 *
 * Depending on OS, this may use:
 * - Vista+: IFileOpenDialog
 * - XP: GetOpenFileName()
 *
 * @param hWnd		[in] Owner
 * @param dlgTitle	[in] Dialog title
 * @param filterSpec	[in] Filter specification (RP format, UTF-8)
 * @param origFilename	[in,opt] Starting filename
 * @return Filename, or empty string on error.
 */
tstring getOpenFileName(HWND hWnd, const TCHAR *dlgTitle,
	const char8_t *filterSpec, const TCHAR *origFilename)
{
	return getFileName_int(false, hWnd, dlgTitle, filterSpec, origFilename);
}

/**
 * Get a filename using the Save File Name dialog.
 *
 * Depending on OS, this may use:
 * - Vista+: IFileSaveDialog
 * - XP: GetSaveFileName()
 *
 * @param hWnd		[in] Owner
 * @param dlgTitle	[in] Dialog title
 * @param filterSpec	[in] Filter specification (RP format, UTF-8)
 * @param origFilename	[in,opt] Starting filename
 * @return Filename, or empty string on error.
 */
tstring getSaveFileName(HWND hWnd, const TCHAR *dlgTitle,
	const char8_t *filterSpec, const TCHAR *origFilename)
{
	return getFileName_int(true, hWnd, dlgTitle, filterSpec, origFilename);
}

/** Window procedure subclasses **/

/**
 * Subclass procedure for multi-line EDIT and RICHEDIT controls.
 * This procedure does the following:
 * - ENTER and ESCAPE are forwarded to the parent window.
 * - DLGC_HASSETSEL is masked.
 *
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	HWND of parent dialog to forward WM_COMMAND messages to.
 */
LRESULT CALLBACK MultiLineEditProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg) {
		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, MultiLineEditProc, uIdSubclass);
			break;

		case WM_KEYDOWN: {
			// Work around Enter/Escape issues.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20070820-00/?p=25513
			if (!dwRefData) {
				// No parent dialog...
				break;
			}
			HWND hDlg = reinterpret_cast<HWND>(dwRefData);

			switch (wParam) {
				case VK_RETURN:
					SendMessage(hDlg, WM_COMMAND, IDOK, 0);
					return TRUE;
				case VK_ESCAPE:
					SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
					return TRUE;
				default:
					break;
			}
			break;
		}

		case WM_GETDLGCODE: {
			// Filter out DLGC_HASSETSEL.
			// References:
			// - https://stackoverflow.com/questions/20876045/cricheditctrl-selects-all-text-when-it-gets-focus
			// - https://stackoverflow.com/a/20884852
			const LRESULT code = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			return (code & ~(LRESULT)DLGC_HASSETSEL);
		}

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Subclass procedure for single-line EDIT and RICHEDIT controls.
 * This procedure does the following:
 * - DLGC_HASSETSEL is masked.
 *
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	HWND of parent dialog to forward WM_COMMAND messages to.
 */
LRESULT CALLBACK SingleLineEditProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	((void)dwRefData);

	switch (uMsg) {
		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, SingleLineEditProc, uIdSubclass);
			break;

		case WM_GETDLGCODE: {
			// Filter out DLGC_HASSETSEL.
			// References:
			// - https://stackoverflow.com/questions/20876045/cricheditctrl-selects-all-text-when-it-gets-focus
			// - https://stackoverflow.com/a/20884852
			const LRESULT code = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			return (code & ~(LRESULT)DLGC_HASSETSEL);
		}

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Subclass procedure for ListView controls to disable HDN_DIVIDERDBLCLICK handling.
 * @param hWnd		Dialog handle
 * @param uMsg		Message
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID (usually the control ID)
 * @param dwRefData	RP_ShellPropSheetExt_Private*
 */
LRESULT CALLBACK ListViewNoDividerDblClickSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	((void)dwRefData);

	switch (uMsg) {
		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://devblogs.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, ListViewNoDividerDblClickSubclassProc, uIdSubclass);
			break;

		case WM_NOTIFY: {
			const NMHDR *const pHdr = reinterpret_cast<const NMHDR*>(lParam);
			if (pHdr->code == HDN_DIVIDERDBLCLICK) {
				// Send the notification to the parent control,
				// and ignore it here.
				return SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
			}
			break;
		}

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

}