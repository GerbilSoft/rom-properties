#include <stdio.h>
#include <Windows.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include <wchar.h>
#include <assert.h>
#include <string>
#include <stdarg.h>
#include "resource.h"
#if defined(_WIN64)
#error This app should only be compiled as x86 application
#endif

#define MSVCRT_URL L"https://www.microsoft.com/en-us/download/details.aspx?id=53587"

namespace {
	// File paths
	constexpr wchar_t str_rp32path[] = L"i386\\rom-properties.dll";
	constexpr wchar_t str_rp64path[] = L"amd64\\rom-properties.dll";

	// Dialog strings
	constexpr wchar_t strdlg_title[] = L"ROM Properties installer";
	constexpr wchar_t strdlg_installBtn[] = L"Install";
	constexpr wchar_t strdlg_uninstallBtn[] = L"Uninstall";

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

	constexpr wchar_t str_installIntro[] =
		L"This installer will register the ROM Properties DLL with the system, "
		L"which will provide extra functionality for supported files in Windows Explorer.\n\n"
		L"Note that the DLL locations are hard-coded in the registry. "
		L"If you move the DLLs, you will have to rerun this script. "
		L"In addition, the DLLs will usually be locked by Explorer, "
		L"so you will need to run the uninstall script and restart "
		L"Windows Explorer in order to move the DLLs.\n\n"
		L"Do you want to proceed?";

	constexpr wchar_t str_uninstallIntro[] =
		L"This uninstaller will unregister the ROM Properties DLL.\n\n"
		L"NOTE: Unregistering will disable all functionality provided "
		L"by the DLL for supported ROM files.\n\n"
		L"Do you want to proceed?";

	constexpr wchar_t str_msvcNotFound[] =
		L"ERROR: MSVC 2015 runtime was not found.\n"
		L"Please download and install the MSVC 2015 runtime packages from: " MSVCRT_URL L"\n\n"
		L"NOTE: On 64-bit systems, you will need to install both the 32-bit and the 64-bit versions.\n\n"
		L"Do you want to open the URL in your web browser?";

	constexpr wchar_t str_couldntOpenUrl[] = L"ERROR: Couldn't open URL";

	// Globals

	HINSTANCE g_hInst; /**< hInstance of this application */
	bool g_isWow; /**< true if running on 64-bit system */
	bool g_inProgress = false; /**< true if currently (un)installing the DLLs */

	// Custom messages
	constexpr UINT WM_APP_ENDTASK = WM_APP;

	/**
	 * Formats a wide string. C++-ism for swprintf.
	 *
	 * @param format the format string
	 * @returns formated string
	 */
	std::wstring Format(const wchar_t *format, ...) {
		// NOTE: There's no standard "dry-run" version of swprintf, and even the msvc
		// extension is deprecated by _s version which doesn't allow dry-running.
		va_list args;
		va_start(args, format);
		int count = _vsnwprintf((wchar_t*)nullptr, 0, format, args); // cast is nescessary for VS2015, because template overloads
		wchar_t *buf = new wchar_t[count + 1];
		_vsnwprintf(buf, count + 1, format, args);
		std::wstring rv(buf);
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
		PathAppend(msvc, is64 ? L"Sysnative\\msvcp140.dll" : L"System32\\msvcp140.dll");
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
	int InstallServer(bool isUninstall, bool is64, std::wstring& extraError) {
		// Construct path for regsvr
		wchar_t regsvr[MAX_PATH];
		if (0 == GetWindowsDirectory(regsvr, MAX_PATH)) {
			DebugBreak();
			return -1;
		}
		PathAppend(regsvr, is64 ? L"Sysnative\\regsvr32.exe" : L"System32\\regsvr32.exe");

		// Construct arguments
		wchar_t args[14 + MAX_PATH + 4 + 3] = L"regsvr32.exe \"";
		// Construct path to rom-properties.dll inside the arguments
		DWORD szModuleFn = GetModuleFileName(g_hInst, &args[14], MAX_PATH);
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
		std::wstring extraError;
		int result = InstallServer(isUninstall, is64, extraError);
		const wchar_t *title = isUninstall ? str_uninstallTitle : str_installTitle;

		// This is different from the original install script in that it
		// doesn't consider 64-bit errors to be fatal.
		// Rationale: User might want to only install 32-bit version.

		if (result == -1) {
			return;
		}
		else if (result == 1) { // File not found
			const std::wstring suffix = isUninstall
				? Format(strfmt_skippingUnreg, is64 ? 64 : 32)
				: (g_isWow
					? Format(strfmt_dllRequiredNote, is64 ? 64 : 32)
					: L"");
			const wchar_t *filename = is64 ? str_rp64path : str_rp32path;
			MessageBox(hWnd, (Format(strfmt_notFound, filename) + suffix).c_str(), title, isUninstall || g_isWow ? MB_ICONWARNING : MB_ICONERROR);
		}
		else {
			const wchar_t *fmtString;
			std::wstring suffix;
			if (result == 0) { // Success
				fmtString = isUninstall ? strfmt_uninstallSuccess : strfmt_installSuccess;
			}
			else { // Failure
				fmtString = isUninstall ? strfmt_uninstallFailure : strfmt_installFailure;
				suffix = extraError;
				if (!isUninstall && g_isWow) suffix += Format(strfmt_dllRequiredNote, is64 ? 64 : 32);
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
		if (!params->isUninstall && (!CheckMsvc(false) || g_isWow && !CheckMsvc(true)))
		{
			if (IDYES == MessageBox(hWnd, str_msvcNotFound, str_installTitle, MB_ICONWARNING | MB_YESNO)) {
				if (32 >= (int)ShellExecute(nullptr, L"open", MSVCRT_URL, nullptr, nullptr, SW_SHOW)) {
					MessageBox(hWnd, str_couldntOpenUrl, str_installTitle, MB_ICONERROR);
				}
			}
		}
		else {
			if (g_isWow) TryInstallServer(hWnd, false, true);
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
	 * Main dialog message handler
	 */
	INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_INITDIALOG: {
			// Set icon
			HICON hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			if (hIcon) {
				SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			}

			// Set title/strings
			SetWindowText(hWnd, strdlg_title);
			SetWindowText(GetDlgItem(hWnd, IDC_BUTTON_INSTALL), strdlg_installBtn);
			SetWindowText(GetDlgItem(hWnd, IDC_BUTTON_UNINSTALL), strdlg_uninstallBtn);

			return TRUE;
		}
		case WM_SETCURSOR: {
			DlgUpdateCursor();
			return TRUE;
		}
		case WM_APP_ENDTASK: {
			g_inProgress = false;
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_INSTALL), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_UNINSTALL), TRUE);
			DlgUpdateCursor();
			return TRUE;
		}
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
			case IDC_BUTTON_INSTALL:
			case IDC_BUTTON_UNINSTALL:
				if (g_inProgress) return TRUE;
				bool isUninstall = LOWORD(wParam) == IDC_BUTTON_UNINSTALL;
				const wchar_t* intro = isUninstall ? str_uninstallIntro : str_installIntro;
				const wchar_t* title = isUninstall ? str_uninstallTitle : str_installTitle;
				if (IDYES == MessageBox(hWnd, intro, title, MB_ICONINFORMATION | MB_YESNO)) {
					EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_INSTALL), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_UNINSTALL), FALSE);
					g_inProgress = true;
					DlgUpdateCursor();

					// The installation is done on a separate thread so that we don't lock the message loop
					ThreadParams *params = new ThreadParams;
					params->hWnd = hWnd;
					params->isUninstall = isUninstall;
					HANDLE hThread = CreateThread(nullptr, 0, ThreadProc, params, 0, nullptr);
					if (!hThread) {
						MessageBox(hWnd, Format(strfmt_createThreadErr, GetLastError()).c_str(), title, MB_ICONERROR);
						return TRUE;
					}
					CloseHandle(hThread);
				}
				return TRUE;
			}
			return FALSE;
		}
		case WM_CLOSE:
			if (!g_inProgress) {
				EndDialog(hWnd, 0);
			}
			return TRUE;
			break;
		}
		return FALSE;
	}
}

/**
 * Entry point
 */
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow) {
	g_hInst = hInstance;

	// Detect Wow64
	HMODULE kernel32 = GetModuleHandle(L"kernel32");
	if (!kernel32) {
		DebugBreak();
		return 1;
	}
	typedef BOOL(WINAPI *fnIsWow64Process)(HANDLE, PBOOL);
	fnIsWow64Process myIsWow64Process = (fnIsWow64Process)GetProcAddress(kernel32, "IsWow64Process");

	if (myIsWow64Process == nullptr) {
		g_isWow = false;
	}
	else {
		BOOL bWow;
		if (!myIsWow64Process(GetCurrentProcess(), &bWow)) {
			DebugBreak();
			return 1;
		}
		g_isWow = bWow ? true : false;
	}

	// Run the dialog
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), nullptr, DialogProc);

	return 0;
}
