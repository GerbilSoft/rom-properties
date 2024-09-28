/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * DialogBuilder.cpp: DLGTEMPLATEEX builder class.                         *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// DLGTEMPLATEEX references:
// - DLGTEMPLATE: https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-dlgtemplate
// - DLGTEMPLATEEX: https://docs.microsoft.com/en-us/windows/win32/dlgbox/dlgtemplateex
// - DLGITEMTEMPLATE: https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-dlgitemtemplate
// - DLGITEMTEMPLATEEX: https://docs.microsoft.com/en-us/windows/win32/dlgbox/dlgitemtemplateex
// - 32-bit extended dialogs: https://devblogs.microsoft.com/oldnewthing/20040623-00/?p=38753

#include "DialogBuilder.hpp"

// C includes (C++ namespace)
#include <cassert>
#include <cstdlib>

// Windows SDK
#include <objbase.h>

// PACKED attribute
#include "common.h"

namespace LibWin32UI {

DialogBuilder::DialogBuilder()
	: m_pDlgBuf(m_DlgBuf)
	, m_pcDlgItems(nullptr)
{}

/**
 * DLGTEMPLATEEX helper structs.
 * These structs contain the main data, but not the
 * variable-length strings.
 *
 * NOTE: These structs MUST be WORD-packed!
 */

#pragma pack(2)
typedef struct PACKED _DLGTEMPLATEEX {
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATEEX;
ASSERT_STRUCT(DLGTEMPLATEEX, 26);
#pragma pack()

#pragma pack(2)
typedef struct PACKED _DLGTEMPLATEEX_FONT {
	WORD pointsize;
	WORD weight;
	BYTE italic;
	BYTE charset;
} DLGTEMPLATEEX_FONT;
ASSERT_STRUCT(DLGTEMPLATEEX_FONT, 6);
#pragma pack()

#pragma pack(2)
typedef struct PACKED _DLGITEMTEMPLATEEX {
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	short x;
	short y;
	short cx;
	short cy;
	DWORD id;
} DLGITEMTEMPLATEEX;
ASSERT_STRUCT(DLGITEMTEMPLATEEX, 24);
#pragma pack()

/**
 * Assertion macro for dialog buffer overflows.
 * TODO: Show an error message in release builds?
 * @param sz Size being added.
 */
#define ASSERT_BUFFER(sz) do { \
	assert(m_pDlgBuf + (sz) <= &m_DlgBuf[sizeof(m_DlgBuf)]); \
	if (m_pDlgBuf + (sz) > &m_DlgBuf[sizeof(m_DlgBuf)]) { \
		abort(); \
	} \
} while (0)

/** DLGTEMPLATEEX helper functions. **/

inline void DialogBuilder::write_word(WORD w)
{
	ASSERT_BUFFER(sizeof(WORD));
	LPWORD lpw = (LPWORD)m_pDlgBuf;
	*lpw = w;
	m_pDlgBuf += sizeof(WORD);
}

inline void DialogBuilder::write_wstr(LPCWSTR wstr)
{
	if (wstr) {
		size_t cbWstr = (wcslen(wstr) + 1) * sizeof(wchar_t);
		ASSERT_BUFFER(cbWstr);
		memcpy(m_pDlgBuf, wstr, cbWstr);
		m_pDlgBuf += cbWstr;
	} else {
		// NULL string.
		ASSERT_BUFFER(sizeof(WORD));
		LPWORD lpw = (LPWORD)m_pDlgBuf;
		*lpw = 0;
		m_pDlgBuf += sizeof(WORD);
	}
}

inline void DialogBuilder::write_wstr_ord(LPCWSTR wstr)
{
	// FIXME: Prevent buffer overflows.
	if ((reinterpret_cast<ULONG_PTR>(wstr)) <= 0xFFFF) {
		// String is an atom.
		ASSERT_BUFFER(sizeof(WORD)*2);
		write_word(0xFFFF);
		write_word(reinterpret_cast<ULONG_PTR>(wstr) & 0xFFFF);
	} else {
		// Not an atom. Write a normal string.
		write_wstr(wstr);
	}
}

inline void DialogBuilder::align_dword(void)
{
	ULONG_PTR ul = (ULONG_PTR)m_pDlgBuf;
	ul = (ul + 3) & ~3;
	m_pDlgBuf = reinterpret_cast<uint8_t*>(ul);
}

/**
 * Initialize the DLGTEMPLATEEX.
 *
 * DS_SETFONT will always be added to dwStyle,
 * and the appropriate dialog font will be
 * added to the dialog structure.
 *
 * NOTE: Help ID, menu, and custom dialog classes
 * are not supported.
 *
 * @param lpTemplate	[in] DLGTEMPLATE.
 * @param lpszTitle	[in, opt] Dialog title.
 */
void DialogBuilder::init(LPCDLGTEMPLATE lpTemplate, LPCWSTR lpszTitle)
{
	// Reset the dialog buffer pointer.
	m_pDlgBuf = m_DlgBuf;

	// Initialize the DLGTEMPLATEEX.
	ASSERT_BUFFER(sizeof(DLGTEMPLATEEX));
	DLGTEMPLATEEX *lpdt = (DLGTEMPLATEEX*)m_pDlgBuf;
	m_pDlgBuf += sizeof(*lpdt);
	lpdt->dlgVer = 1;
	lpdt->signature = 0xFFFF;
	lpdt->helpID = 0;

	// Copy from DLGTEMPLATE.
	lpdt->exStyle = lpTemplate->dwExtendedStyle;
	lpdt->style = lpTemplate->style | DS_SETFONT;
	lpdt->cDlgItems = 0;	// updated later
	lpdt->x = lpTemplate->x;
	lpdt->y = lpTemplate->y;
	lpdt->cx = lpTemplate->cx;
	lpdt->cy = lpTemplate->cy;

	// Save the address of DLGTEMPLATEEX's cDlgItems for later.
	m_pcDlgItems = &lpdt->cDlgItems;

	// No menu; default dialog class.
	write_word(0);		// sz_Or_Ord menu;
	write_word(0);		// sz_Or_Ord windowClass;

	// Dialog title.
	write_wstr(lpszTitle);

	// Font information.
	// TODO: Make DS_SETFONT optional?
	ASSERT_BUFFER(sizeof(DLGTEMPLATEEX_FONT));
	DLGTEMPLATEEX_FONT *lpdtf = (DLGTEMPLATEEX_FONT*)m_pDlgBuf;
	m_pDlgBuf += sizeof(*lpdtf);
	lpdtf->pointsize = 8;
	lpdtf->weight = FW_NORMAL;
	lpdtf->italic = FALSE;
	lpdtf->charset = 0;
	// TODO: "MS Shell Dlg" or "MS Shell Dlg 2"?
	write_wstr(L"MS Shell Dlg");	// WCHAR typeface[];
}

/**
 * Add a control to the dialog.
 * @param lpItemTemplate	[in] DLGITEMTEMPLATE.
 * @param lpszWindowClass	[in] Window class. (May be an ordinal value.)
 * @param lpszWindowText	[in, opt] Window text. (May be an ordinal value.)
 */
void DialogBuilder::add(const DLGITEMTEMPLATE *lpItemTemplate, LPCWSTR lpszWindowClass, LPCWSTR lpszWindowText)
{
	assert(m_pcDlgItems != nullptr);

	// Create a DLGITEMTEMPLATEEX based on the DLGITEMTEMPLATE.
	align_dword();
	ASSERT_BUFFER(sizeof(DLGITEMTEMPLATEEX));
	DLGITEMTEMPLATEEX *lpdit = (DLGITEMTEMPLATEEX*)m_pDlgBuf;
	m_pDlgBuf += sizeof(*lpdit);

	// Copy from DLGITEMTEMPLATE.
	lpdit->helpID = 0;
	lpdit->exStyle = lpItemTemplate->dwExtendedStyle;
	lpdit->style = lpItemTemplate->style;
	lpdit->x = lpItemTemplate->x;
	lpdit->y = lpItemTemplate->y;
	lpdit->cx = lpItemTemplate->cx;
	lpdit->cy = lpItemTemplate->cy;
	lpdit->id = static_cast<DWORD>(lpItemTemplate->id);

	// Window class and text.
	write_wstr_ord(lpszWindowClass);
	write_wstr_ord(lpszWindowText);

	// Extra count.
	write_word(0);		// WORD extraCount;

	// Increment the dialog's control count.
	(*m_pcDlgItems)++;
}

/**
 * Get a pointer to the created DLGTEMPLATEEX.
 * @return DLGTEMPLATE.
 */
LPCDLGTEMPLATE DialogBuilder::get(void) const
{
	if (!m_pDlgBuf || m_pDlgBuf == m_DlgBuf) {
		// DLGTEMPLATEEX hasn't been created yet.
		return nullptr;
	}

	return (LPCDLGTEMPLATE)m_DlgBuf;
}

/**
 * Clear the dialog template.
 */
void DialogBuilder::clear(void)
{
	// Reset the pointer to the beginning of the buffer.
	m_pDlgBuf = m_DlgBuf;
}

} //namespace LibWin32UI
