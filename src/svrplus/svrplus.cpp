/***************************************************************************
 * ROM Properties Page shell extension installer. (svrplus)                *
 * svrplus.rc: Win32 installer for rom-properties.                         *
 *                                                                         *
 * Copyright (c) 2017 by Egor.                                             *
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

// C includes. (C++ namespace)
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cwchar>

// C++ includes.
#include <string>
using std::wstring;

// Windows headers.
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>

// Application resources.
#include "resource.h"

#define MSVCRT_URL L"https://www.microsoft.com/en-us/download/details.aspx?id=53587"

namespace {
	// File paths
	constexpr wchar_t str_rp32path[] = L"i386\\rom-properties.dll";
	constexpr wchar_t str_rp64path[] = L"amd64\\rom-properties.dll";

	/** Dialog strings **/

	// User visible strings
	constexpr wchar_t str_exitConfirmation[] =
		L"An installation is in progress\n\n"
		L"Are you sure you want to exit?\n\n"
		L"(regsvr32 may be still running in the background)";

	constexpr wchar_t str_installTitle[] = L"ROM Properties installer";
	constexpr wchar_t str_uninstallTitle[] = L"ROM Properties uninstaller";

	constexpr wchar_t strfmt_installSuccess[] = L"%d-bit DLL registration successful.";
	constexpr wchar_t strfmt_uninstallSuccess[] = L"%d-bit DLL unregistration successful.";

	constexpr wchar_t strfmt_installFailure[] = L"%d-bit DLL registration failed:\n";
	constexpr wchar_t strfmt_uninstallFailure[] = L"%d-bit DLL unregistration failed:\n";

	constexpr wchar_t strfmt_notFound[] = L"%s not found.";
	constexpr wchar_t strfmt_dllRequiredNote[] = L"\n\nThis DLL is required in order to support %d-bit applications.";
	constexpr wchar_t strfmt_skippingUnreg[] = L"\n\nSkipping %d-bit DLL unregistration.";

	// extraErrors
	constexpr wchar_t strfmt_createProcessErr[] = L"Couldn't create regsvr32 process (%d)";
	constexpr wchar_t str_stillActive[] = L"Process returned STILL_ACTIVE";
	constexpr wchar_t strfmt_regsvrErr[] = L"regsvr32 returned error code (%d)";

	constexpr wchar_t strfmt_createThreadErr[] = L"Couldn't start worker thread (%d)";

	constexpr wchar_t str_msvcNotFound[] =
		L"ERROR: MSVC 2015 runtime was not found.\n"
		L"Please download and install the MSVC 2015 runtime packages from: " MSVCRT_URL L"\n\n"
		L"NOTE: On 64-bit systems, you will need to install both the 32-bit and the 64-bit versions.\n\n"
		L"Do you want to open the URL in your web browser?";

	constexpr wchar_t str_couldntOpenUrl[] = L"ERROR: Couldn't open URL";

	// Globals

	HINSTANCE g_hInstance; /**< hInstance of this application */
#ifdef _WIN64
	constexpr bool g_is64bit = true;	/**< true if running on 64-bit system */
#else /* !_WIN64 */
	bool g_is64bit = false;			/**< true if running on 64-bit system */
#endif
	bool g_inProgress = false; /**< true if currently (un)installing the DLLs */

	// Custom messages
	constexpr UINT WM_APP_ENDTASK = WM_APP;

	// Icons. (NOTE: These MUST be deleted after use!)
	HICON hIconDialog = nullptr;
	HICON hIconDialogSmall = nullptr;

	/**
	 * Formats a wide string. C++-ism for swprintf.
	 *
	 * @param format the format string
	 * @returns formated string
	 */
	wstring Format(const wchar_t *format, ...) {
		// NOTE: There's no standard "dry-run" version of swprintf, and even the msvc
		// extension is deprecated by _s version which doesn't allow dry-running.
		va_list args;
		va_start(args, format);
		int count = _vsnwprintf((wchar_t*)nullptr, 0, format, args); // cast is nescessary for VS2015, because template overloads
		wchar_t *buf = new wchar_t[count + 1];
		_vsnwprintf(buf, count + 1, format, args);
		wstring rv(buf);
		delete[] buf;
		va_end(args);
		return rv;
	}

	/**
	 * Checks wether or not Visual C++ runtime is installed.
	 *
	 * @param is64 when true, checks for 64-bit version
	 * @return true if installed
	 */
	bool CheckMsvc(bool is64) {
		wchar_t msvc[MAX_PATH];
		if (0 == GetWindowsDirectory(msvc, MAX_PATH)) {
			DebugBreak();
			return true;
		}
#ifdef _WIN64
		const wchar_t *const msvcp140_dll = (is64 ? L"System32\\msvcp140.dll" : L"SysWOW64\\msvcp140.dll");
#else /* !_WIN64 */
		const wchar_t *const msvcp140_dll = (is64 ? L"Sysnative\\msvcp140.dll" : L"System32\\msvcp140.dll");
#endif
		PathAppend(msvc, msvcp140_dll);
		return PathFileExists(msvc) == TRUE;
	}

	/**
	 * (Un)installs the COM Server DLL.
	 *
	 * @param isUninstall when true, uninstalls the DLL, instead of installing it.
	 * @param is64 when true, installs 64-bit version
	 * @param extraError additional error message
	 * @return 0 - success, 1 - file not found, 2 - other (fatal) error (see extraError), -1 - silent failure
	 */
	int InstallServer(bool isUninstall, bool is64, wstring& extraError) {
		// Construct path for regsvr
		wchar_t regsvr[MAX_PATH];
		if (0 == GetWindowsDirectory(regsvr, MAX_PATH)) {
			DebugBreak();
			return -1;
		}
#ifdef _WIN64
		const wchar_t *const regsvr32_exe = (is64 ? L"System32\\regsvr32.exe" : L"SysWOW64\\regsvr32.exe");
#else /* !_WIN64 */
		const wchar_t *const regsvr32_exe = (is64 ? L"Sysnative\\regsvr32.exe" : L"System32\\regsvr32.exe");
#endif
		PathAppend(regsvr, regsvr32_exe);

		// Construct arguments
		wchar_t args[14 + MAX_PATH + 4 + 3] = L"regsvr32.exe \"";
		// Construct path to rom-properties.dll inside the arguments
		DWORD szModuleFn = GetModuleFileName(g_hInstance, &args[14], MAX_PATH);
		if (szModuleFn == MAX_PATH || szModuleFn == 0) {
			// TODO: add an error message for the MAX_PATH case?
			DebugBreak();
			return -1;
		}
		PathFindFileName(&args[14])[0] = 0;
		PathAppend(&args[14], is64 ? str_rp64path : str_rp32path);
		if (!PathFileExists(&args[14])) {
			return 1; // File not found
		}
		// Append /s (silent) key, and optionally append /u (uninstall) key.
		wcscat_s(args, _countof(args), isUninstall ? L"\" /s /u" : L"\" /s");

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		if (!CreateProcess(regsvr, args, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
			extraError = Format(strfmt_createProcessErr, GetLastError());
			return 2;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD status;
		GetExitCodeProcess(pi.hProcess, &status);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (status == STILL_ACTIVE) {
			extraError = str_stillActive;
			return 2;
		}

		if (status) {
			extraError = Format(strfmt_regsvrErr, status);
			return 2;
		}
		return 0;
	}

	/**
	 * Tries to (un)install the COM Server DLL, displays errors in message boxes.
	 *
	 * @param hWnd owner window of message boxes
	 * @param isUninstall when true, uninstalls the DLL, instead of installing it.
	 * @param is64 when true, installs 64-bit version
	 */
	void TryInstallServer(HWND hWnd, bool isUninstall, bool is64) {
		wstring extraError;
		int result = InstallServer(isUninstall, is64, extraError);
		const wchar_t *title = isUninstall ? str_uninstallTitle : str_installTitle;

		// This is different from the original install script in that it
		// doesn't consider 64-bit errors to be fatal.
		// Rationale: User might want to only install 32-bit version.

		if (result == -1) {
			return;
		}
		else if (result == 1) { // File not found
			const wstring suffix = isUninstall
				? Format(strfmt_skippingUnreg, is64 ? 64 : 32)
				: (g_is64bit
					? Format(strfmt_dllRequiredNote, is64 ? 64 : 32)
					: L"");
			const wchar_t *filename = is64 ? str_rp64path : str_rp32path;
			MessageBox(hWnd, (Format(strfmt_notFound, filename) + suffix).c_str(), title, isUninstall || g_is64bit ? MB_ICONWARNING : MB_ICONERROR);
		}
		else {
			const wchar_t *fmtString;
			wstring suffix;
			if (result == 0) { // Success
				fmtString = isUninstall ? strfmt_uninstallSuccess : strfmt_installSuccess;
			}
			else { // Failure
				fmtString = isUninstall ? strfmt_uninstallFailure : strfmt_installFailure;
				suffix = extraError;
				if (!isUninstall && g_is64bit) suffix += Format(strfmt_dllRequiredNote, is64 ? 64 : 32);
			}
			MessageBox(hWnd, (Format(fmtString, is64 ? 64 : 32) + suffix).c_str(), title, result == 0 ? MB_ICONINFORMATION : MB_ICONERROR);
		}
	}

	struct ThreadParams {
		HWND hWnd; /**< Window that created the thread */
		bool isUninstall; /**< true if uninstalling */
	};

	/**
	 * Worker thread procedure.
	 *
	 * @param lpParameter ptr to parameters of type ThreadPrams. Owned by this thread.
	 */
	DWORD WINAPI ThreadProc(LPVOID lpParameter) {
		ThreadParams *params = reinterpret_cast<ThreadParams*>(lpParameter);
		HWND hWnd = params->hWnd;

		// if installing, do additional msvc check
		if (!params->isUninstall && (!CheckMsvc(false) || g_is64bit && !CheckMsvc(true)))
		{
			if (IDYES == MessageBox(hWnd, str_msvcNotFound, str_installTitle, MB_ICONWARNING | MB_YESNO)) {
				if (32 >= (int)ShellExecute(nullptr, L"open", MSVCRT_URL, nullptr, nullptr, SW_SHOW)) {
					MessageBox(hWnd, str_couldntOpenUrl, str_installTitle, MB_ICONERROR);
				}
			}
		}
		else {
			if (g_is64bit) TryInstallServer(hWnd, false, true);
			TryInstallServer(hWnd, false, false);
		}

		SendMessage(hWnd, WM_APP_ENDTASK, 0, 0);
		delete params;
		return 0;
	}

	/**
	 * Changes the cursor depending on wether installation is in progress.
	 */
	inline void DlgUpdateCursor() {
		SetCursor(LoadCursor(nullptr, g_inProgress ? IDC_WAIT : IDC_ARROW));
	}

	/**
	 * Initialize the dialog.
	 * @param hDlg Dialog handle.
	 */
	inline void InitDialog(HWND hDlg)
	{
		if (hIconDialog) {
			SendMessage(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIconDialog));
		}
		if (hIconDialogSmall) {
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconDialogSmall));
		}

		// FIXME: Figure out the SysLink styles. (0x50010000?)
		ShowWindow(GetDlgItem(hDlg, IDC_STATIC_STATUS2), SW_HIDE);

		// Set the dialog strings.
		static const wchar_t strdlg_desc[] =
			L"This installer will register the ROM Properties Page DLL with the system, "
			L"which will provide extra functionality for supported files in Windows Explorer.\n\n"
			L"Note that the DLL locations are hard-coded in the registry. If you move the DLLs, "
			L"you will have to rerun this installer. In addition, the DLLs will usually be locked "
			L"by Explorer, so you will need to use this program to uninstall the DLLs first and "
			L"then restart Explorer in order to move the DLLs.\n\n"
			L"Uninstalling will unregister the ROM Properties DLL, which will disable the extra "
			L"functionality provided by the DLL for supported ROM files.";
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), strdlg_desc);

		// Check if MSVCRT is installed. If it isn't, show a warning
		// message and disable the buttons.
		const bool hasMsvc32 = CheckMsvc(false);

		// Go through the various permutations.
		const wchar_t *line1 = nullptr, *line2 = nullptr;
#ifndef _WIN64
		if (!g_is64bit) {
			// 32-bit system.
			if (!hasMsvc32) {
				// 32-bit MSVCRT is missing.
				line1 = L"The 32-bit MSVC 2017 runtime is not installed.";
				line2 = L"You can download the 32-bit MSVC 2017 runtime at:\n"
					L"\x2022 <a href=\"https://go.microsoft.com/fwlink/?LinkId=746571\">https://go.microsoft.com/fwlink/?LinkId=746571</a>";
			}
		} else
#endif /* !_WIN64 */
		{
			// 64-bit system.
			const bool hasMsvc64 = CheckMsvc(true);
			if (!hasMsvc32 && !hasMsvc64) {
				// Both 32-bit and 64-bit MSVCRT are missing.
				line1 = L"The 32-bit and 64-bit MSVC 2017 runtimes are not installed.";
				line2 = L"You can download the MSVC 2017 runtime at:\n"
					L"\x2022 32-bit: <a href=\"https://go.microsoft.com/fwlink/?LinkId=746571\">https://go.microsoft.com/fwlink/?LinkId=746571</a>\n";
					L"\x2022 64-bit: <a href=\"https://go.microsoft.com/fwlink/?LinkId=746572\">https://go.microsoft.com/fwlink/?LinkId=746572</a>";
			} else if (!hasMsvc32 && hasMsvc64) {
				// 32-bit MSVCRT is missing.
				line1 = L"The 32-bit MSVC 2017 runtime is not installed.";
				line2 = L"You can download the 32-bit MSVC 2017 runtime at:\n"
					L"\x2022 <a href=\"https://go.microsoft.com/fwlink/?LinkId=746571\">https://go.microsoft.com/fwlink/?LinkId=746571</a>";
			} else if (hasMsvc32 && !hasMsvc64) {
				// 64-bit MSVCRT is missing.
				line1 = L"The 64-bit MSVC 2017 runtime is not installed.";
				line2 = L"You can download the 64-bit MSVC 2017 runtime at:\n"
					L"\x2022 <a href=\"https://go.microsoft.com/fwlink/?LinkId=746572\">https://go.microsoft.com/fwlink/?LinkId=746572</a>";
			}
		}

		if (line1) {
			// MSVCRT is missing.
			// TODO: IDI_EXCLAMATION
			HWND hStatus1 = GetDlgItem(hDlg, IDC_STATIC_STATUS1);
			HWND hStatus2 = GetDlgItem(hDlg, IDC_STATIC_STATUS2);
			SetWindowText(hStatus1, line1);
			SetWindowText(hStatus2, line2);
			ShowWindow(hStatus1, SW_SHOW);
			ShowWindow(hStatus2, SW_SHOW);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_INSTALL), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UNINSTALL), FALSE);
		}
	}

	/**
	 * Main dialog message handler
	 */
	INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_INITDIALOG: {
			InitDialog(hDlg);
			break;
		}
		case WM_SETCURSOR: {
			DlgUpdateCursor();
			return TRUE;
		}
		case WM_APP_ENDTASK: {
			g_inProgress = false;
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_INSTALL), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UNINSTALL), TRUE);
			DlgUpdateCursor();
			return TRUE;
		}
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDC_BUTTON_INSTALL:
				case IDC_BUTTON_UNINSTALL: {
					if (g_inProgress) return TRUE;
					bool isUninstall = LOWORD(wParam) == IDC_BUTTON_UNINSTALL;
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_INSTALL), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UNINSTALL), FALSE);
					g_inProgress = true;
					DlgUpdateCursor();

					// The installation is done on a separate thread so that we don't lock the message loop
					ThreadParams *params = new ThreadParams;
					params->hWnd = hDlg;
					params->isUninstall = isUninstall;
					HANDLE hThread = CreateThread(nullptr, 0, ThreadProc, params, 0, nullptr);
					if (!hThread) {
						// TODO: Show the error message on the status labels.
						const wchar_t *const title = (isUninstall ? L"Uninstall Error" : L"Install Error");
						MessageBox(hDlg, Format(strfmt_createThreadErr, GetLastError()).c_str(), title, MB_ICONERROR);
						return TRUE;
					}
					CloseHandle(hThread);
					return TRUE;
				}

				default:
					break;
			}
			return FALSE;
		}
		case WM_CLOSE:
			if (!g_inProgress) {
				EndDialog(hDlg, 0);
			}
			return TRUE;
		}
		return FALSE;
	}
}

/**
 * Entry point
 */
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;

#ifndef _WIN64
	// Check if this is a 64-bit system. (Wow64)
	HMODULE kernel32 = GetModuleHandle(L"kernel32");
	if (!kernel32) {
		DebugBreak();
		return 1;
	}
	typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
	PFNISWOW64PROCESS pfnIsWow64Process = (PFNISWOW64PROCESS)GetProcAddress(kernel32, "IsWow64Process");

	if (!pfnIsWow64Process) {
		// IsWow64Process() isn't available.
		// This must be a 32-bit system.
		g_is64bit = false;
	} else {
		BOOL bWow;
		if (!pfnIsWow64Process(GetCurrentProcess(), &bWow)) {
			DebugBreak();
			return 1;
		}
		g_is64bit = !!bWow;
	}
#endif /* !_WIN64 */

	// Load the icon.
	hIconDialog = static_cast<HICON>(LoadImage(
		g_hInstance, MAKEINTRESOURCE(IDI_SVRPLUS), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON),
		GetSystemMetrics(SM_CYICON), 0));
	hIconDialogSmall = static_cast<HICON>(LoadImage(
		g_hInstance, MAKEINTRESOURCE(IDI_SVRPLUS), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON), 0));

	// Run the dialog
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_SVRPLUS), nullptr, DialogProc);

	// Delete the icons.
	if (hIconDialog) {
		DestroyIcon(hIconDialog);
	}
	if (hIconDialogSmall) {
		DestroyIcon(hIconDialogSmall);
	}

	return 0;
}
