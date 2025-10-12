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
#include "librpbyteswap/byteswap_rp.h"

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

// Win1.x/2.x icon structs cpoied from ico_structs.h.
// We can't include ico_structs.h here due to conflicts with the Windows SDK.

/**
 * Windows 1.0: Icon (and cursor)
 *
 * All fields are in little-endian.
 */
typedef struct _ICO_Win1_Header {
	uint16_t format;	// [0x000] See ICO_Win1_Format_e
	uint16_t hotX;		// [0x002] Cursor hotspot X (cursors only)
	uint16_t hotY;		// [0x004] Cursor hotspot Y (cursors only)
	uint16_t width;		// [0x006] Width, in pixels
	uint16_t height;	// [0x008] Height, in pixels
	uint16_t stride;	// [0x00A] Row stride, in bytes
	uint16_t color;		// [0x00C] Cursor color
} ICO_Win1_Header;

/**
 * Windows 1.0: Icon format
 */
typedef enum {
	ICO_WIN1_FORMAT_MAYBE_WIN3	= 0x0000U,	// may be a Win3 icon/cursor

	ICO_WIN1_FORMAT_ICON_DIB	= 0x0001U,
	ICO_WIN1_FORMAT_ICON_DDB	= 0x0101U,
	ICO_WIN1_FORMAT_ICON_BOTH	= 0x0201U,

	ICO_WIN1_FORMAT_CURSOR_DIB	= 0x0003U,
	ICO_WIN1_FORMAT_CURSOR_DDB	= 0x0103U,
	ICO_WIN1_FORMAT_CURSOR_BOTH	= 0x0203U,
} ICO_Win1_Format_e;

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
	uint32_t iconResID = 0;
	rp::uvector<uint8_t> iconData = exe->loadIconResourceData(nIconIndex, LOWORD(cxIcon), HIWORD(cyIcon), &iconResID);
	if (iconData.size() < sizeof(ICO_Win1_Header)) {
		// No icon data...
		if (piconid) {
			*piconid = iconResID;
		}
		return 0;
	}

	// Check for a Win1.x/2.x icon.
	const ICO_Win1_Header *win1 = reinterpret_cast<const ICO_Win1_Header*>(iconData.data());
	const uint16_t win1_format = cpu_to_le16(win1->format);
	if (((win1_format & 0xFF) == 1) || ((win1_format & 0xFF) == 3) &&
		((win1_format & 0xFF00) == 0) || ((win1_format & 0xFF00) == 1) || ((win1_format & 0xFF00) == 2))
	{
		// This is a Win1.x/2.x icon.
		// Need to vertically flip the icon data.
		// NOTE: 2x height * stride because of bitmap + mask.
		// TODO: Flip the second one if we have DIB+DDB?
		unsigned int ico_height = le16_to_cpu(win1->height) * 2;
		unsigned int ico_stride = le16_to_cpu(win1->stride);
		assert(ico_height > 0);
		assert(ico_stride > 0);
		if (ico_height == 0 || ico_stride == 0) {
			return 0;
		}
		const uint8_t *pEndSrcIco0 = &iconData[sizeof(ICO_Win1_Header) + (ico_height * ico_stride)];
		const uint8_t *pSrc = pEndSrcIco0 - ico_stride;

		rp::uvector<uint8_t> flipIcon;
		flipIcon.resize(iconData.size());
		memcpy(flipIcon.data(), win1, sizeof(*win1));
		uint8_t *pDest = &flipIcon[sizeof(ICO_Win1_Header)];

		for (unsigned int y = 0; y < ico_height; y++, pSrc -= ico_stride, pDest += ico_stride) {
			memcpy(pDest, pSrc, ico_stride);
		}

		// TODO: DIB+DDB needs testing.
#if 0
		// Is this DIB+DDB?
		if ((win1_format & 0xFF00) == 0x0200) {
			// Flip the second image.
			win1 = reinterpret_cast<const ICO_Win1_Header*>(pEndSrcIco0 - 2);
			memcpy(pDest, pEndSrcIco0, sizeof(*win1) - 2);
			pDest += (sizeof(*win1) - 2);

			ico_height = le16_to_cpu(win1->height) * 2;
			ico_stride = le16_to_cpu(win1->stride);
			assert(ico_height > 0);
			assert(ico_stride > 0);
			if (ico_height == 0 || ico_stride == 0) {
				return 0;
			}
			pSrc = pEndSrcIco0 + sizeof(*win1) - 2;

			for (unsigned int y = 0; y < ico_height; y++, pSrc -= ico_stride, pDest += ico_stride) {
				memcpy(pDest, pSrc, ico_stride);
			}
		}
#endif

		iconData = std::move(flipIcon);
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
