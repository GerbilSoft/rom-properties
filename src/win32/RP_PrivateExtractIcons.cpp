/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_PrivateExtractIcons.cpp: PrivateExtractIcons() implementation.       *
 *                                                                         *
 * Copyright (c) 2025 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// PrivateExtractIcons() wrapper functions. These are used with a
// stub executable that uses Microsoft's Detours library to enable
// 16-bit executables to be thumbnailed on 64-bit Windows.

// References:
// - https://github.com/otya128/Icon16bitFix (Wine license)
// - https://github.com/microsoft/Detours (MIT license)

#include "stdafx.h"
#include "RP_PrivateExtractIcons.hpp"

#include "dll-macros.h"

// Other rom-properties libraries.
#include "librpfile/RpFile.hpp"
#include "libromdata/Other/EXE.hpp"
using namespace LibRomData;
using namespace LibRpFile;

// Detours
#include "detours.h"

// C++ STL classes
using std::unique_ptr;

typedef UINT (WINAPI *PrivateExtractIconsW_t)(
	_In_ LPCWSTR szFileName,
	_In_ int nIconIndex,
	_In_ int cxIcon,
	_In_ int cyIcon,
	_Out_opt_ HICON *phicon,
	_Out_opt_ UINT *piconid,
	_In_ UINT nIcons,
	_In_ UINT flags);

typedef UINT (WINAPI *PrivateExtractIconsA_t)(
	_In_ LPCSTR szFileName,
	_In_ int nIconIndex,
	_In_ int cxIcon,
	_In_ int cyIcon,
	_Out_opt_ HICON *phicon,
	_Out_opt_ UINT *piconid,
	_In_ UINT nIcons,
	_In_ UINT flags);

// Old PrivateExtractIcons() function pointers, and a reference counter.
static int ref_count = 0;
static PrivateExtractIconsW_t old_PrivateExtractIconsW = nullptr;
static PrivateExtractIconsA_t old_PrivateExtractIconsA = nullptr;

/**
 * Custom implementation of PrivateExtractIconsW().
 * (Internal function; called by the wrapper functions.)
 *
 * Handles .dll/.exe on 64-bit Windows in order to extract icons from
 * 16-bit Windows executables, which normally aren't handled on anything
 * other than 32-bit i386 Windows.
 *
 * @param szFileName
 * @param nIconIndex
 * @param cxIcon
 * @param cyIcon
 * @param phicon
 * @param piconid
 * @param nIcons
 * @param flags
 * @return Number of icons extracted, 0 on error, or 0xFFFFFFFFU if the file was not found.
 */
static UINT WINAPI RP_PrivateExtractIconsW_int(
	_In_ LPCWSTR szFileName,
	_In_ int nIconIndex,
	_In_ int cxIcon,
	_In_ int cyIcon,
	_Out_opt_ HICON *phicon,
	_Out_opt_ UINT *piconid,
	_In_ UINT nIcons,
	_In_ UINT flags)
{
	// TODO: Handle nIcons > 1.
	assert(szFileName != nullptr);
	assert(szFileName[0] != L'\0');
	assert(cxIcon != 0);
	assert(cyIcon != 0);
	assert(nIcons >= 0);
	assert(nIcons <= 2);
	if (!szFileName || szFileName[0] == L'\0' || cxIcon == 0 || cyIcon == 0) {
		return 0;
	}

	// Zero out phicon initially.
	if (phicon && nIcons > 0) {
		memset(phicon, 0, sizeof(HICON) * nIcons);
	}

	if (nIcons < 0 || nIcons > 2) {
		return 0;
	}

	// NOTE: This function only supports .exe/.dll.
	// We could also handle .ico, but Windows should handle that regardless.
	// TODO: Win1.x/2.x icons?
	// MSDN says .ani and .bmp are also supported.
	// Reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-privateextracticonsw

	// Attempt to open the file first.
	IRpFilePtr file = std::make_shared<RpFile>(szFileName, RpFile::FM_OPEN_READ);
	if (!file->isOpen()) {
		// Unable to open the file.
		return 0;
	}

	// NOTE: cxIcon/cyIcon are packed using MAKELONG().
	// If two sizes are specified, and nIcons is 2, return 2 icons.
	// Otherwise, for a single icon, only use LOWORD().
	// TODO: Only doing one icon right now.
	nIcons = 1;

	// Try to load the file as .exe/.dll.
	unique_ptr<EXE> exe(new EXE(file));
	if (!exe->isValid()) {
		// Not a valid EXE file.
		return 0;
	}

	// Get the raw icon data.
	// TODO: Win1.x/2.x executables?
	uint32_t iconResID = 0;
	rp::uvector<uint8_t> iconData = exe->loadIconResourceData(nIconIndex, LOWORD(cxIcon), HIWORD(cyIcon), &iconResID);
	if (iconData.empty()) {
		// No icon data...
		if (piconid) {
			*piconid = iconResID;
		}
		return 0;
	}

	// Create one icon for now.
	// TODO: Second icon too?
	phicon[0] = CreateIconFromResourceEx(
		iconData.data(),			// presbits
		static_cast<DWORD>(iconData.size()),	// dwResSize
		TRUE,					// fIcon
		0x00030000,				// dwVer
		LOWORD(cxIcon),				// cxDesired
		LOWORD(cyIcon),				// cyDesired
		flags);					// Flags

	return (phicon[0]) ? 1 : 0;
}

/**
 * Custom implementation of PrivateExtractIconsW().
 *
 * Handles .dll/.exe on 64-bit Windows in order to extract icons from
 * 16-bit Windows executables, which normally aren't handled on anything
 * other than 32-bit i386 Windows.
 *
 * @param szFileName
 * @param nIconIndex
 * @param cxIcon
 * @param cyIcon
 * @param phicon
 * @param piconid
 * @param nIcons
 * @param flags
 * @return Number of icons extracted, 0 on error, or 0xFFFFFFFFU if the file was not found.
 */
static UINT WINAPI RP_PrivateExtractIconsW(
	_In_ LPCWSTR szFileName,
	_In_ int nIconIndex,
	_In_ int cxIcon,
	_In_ int cyIcon,
	_Out_opt_ HICON *phicon,
	_Out_opt_ UINT *piconid,
	_In_ UINT nIcons,
	_In_ UINT flags)
{
	// Try the original function first.
	UINT uRet = old_PrivateExtractIconsW(szFileName, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags);
	if (uRet == 0 || uRet == 0xFFFFFFFFU) {
		// The original function failed. Call RP_PrivateExtractIconsW_int().
		uRet = RP_PrivateExtractIconsW_int(szFileName, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags);
	}
	return uRet;
}

/**
 * Custom implementation of PrivateExtractIconsA().
 *
 * Handles .dll/.exe on 64-bit Windows in order to extract icons from
 * 16-bit Windows executables, which normally aren't handled on anything
 * other than 32-bit i386 Windows.
 *
 * @param szFileName
 * @param nIconIndex
 * @param cxIcon
 * @param cyIcon
 * @param phicon
 * @param piconid
 * @param nIcons
 * @param flags
 * @return Number of icons extracted, 0 on error, or 0xFFFFFFFFU if the file was not found.
 */
static UINT WINAPI RP_PrivateExtractIconsA(
	_In_ LPCSTR szFileName,
	_In_ int nIconIndex,
	_In_ int cxIcon,
	_In_ int cyIcon,
	_Out_opt_ HICON *phicon,
	_Out_opt_ UINT *piconid,
	_In_ UINT nIcons,
	_In_ UINT flags)
{
	// Try the original function first.
	UINT uRet = old_PrivateExtractIconsA(szFileName, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags);
	if (uRet == 0 || uRet == 0xFFFFFFFFU) {
		// The original function failed.
		// Convert the ANSI filename to Unicode, then call RP_PrivateExtractIconsW_int().
		uRet = RP_PrivateExtractIconsW_int(A2W_c(szFileName), nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags);
	}
	return uRet;
}

/**
 * DllMain() DLL_PROCESS_ATTACH function.
 * @return Win32 error code from the Detours library.
 */
LONG RP_PrivateExtractIcons_DllProcessAttach(void)
{
	ref_count++;
	if (ref_count != 1) {
		return ERROR_SUCCESS;
	}

	// Detours applies to the current process only.
	// Since we're already registered as a shell extension for icons,
	// that should be "good enough".
	HMODULE hUser32 = nullptr;
        BOOL bRet = GetModuleHandleEx(0, _T("user32.dll"), &hUser32);
	assert(bRet != FALSE);
	if (!bRet || !hUser32) {
		// user32.dll isn't loaded for some reason?
		return ERROR_MOD_NOT_FOUND;
	}

        old_PrivateExtractIconsW = reinterpret_cast<PrivateExtractIconsW_t>(GetProcAddress(hUser32, "PrivateExtractIconsW"));
	old_PrivateExtractIconsA = reinterpret_cast<PrivateExtractIconsA_t>(GetProcAddress(hUser32, "PrivateExtractIconsA"));
	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((PVOID*)&old_PrivateExtractIconsW, RP_PrivateExtractIconsW);
	DetourAttach((PVOID*)&old_PrivateExtractIconsA, RP_PrivateExtractIconsA);
	return DetourTransactionCommit();
}

/**
 * DllMain() DLL_PROCESS_DETACH function.
 * @return Win32 error code from the Detours library.
 */
LONG RP_PrivateExtractIcons_DllProcessDetach(void)
{
	assert(ref_count > 0);
	ref_count--;
	if (ref_count > 0) {
		return ERROR_SUCCESS;
	}

	// Un-detour the functions.
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach((PVOID*)&old_PrivateExtractIconsW, RP_PrivateExtractIconsW);
	DetourDetach((PVOID*)&old_PrivateExtractIconsA, RP_PrivateExtractIconsA);
	return DetourTransactionCommit();
}
