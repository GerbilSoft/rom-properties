/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DialogBuilder.cpp: DLGTEMPLATEEX builder class.                         *
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

// DLGTEMPLATEEX references:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms644996(v=vs.85).aspx#template_in_memory
// - https://blogs.msdn.microsoft.com/oldnewthing/20040623-00/?p=38753

#include "stdafx.h"
#include "DialogBuilder.hpp"

// C includes. (C++ namespace)
#include <cassert>

DialogBuilder::DialogBuilder()
	: m_pDlgBuf(m_DlgBuf)
	, m_pcDlgItems(nullptr)
{ }

DialogBuilder::~DialogBuilder()
{ }

/** DLGTEMPLATEEX helper functions. **/

inline void DialogBuilder::write_word(WORD w)
{
	// FIXME: Prevent buffer overflows.
	LPWORD lpw = (LPWORD)m_pDlgBuf;
	*lpw = w;
	m_pDlgBuf += sizeof(WORD);
}

inline void DialogBuilder::write_dword(DWORD dw)
{
	// FIXME: Prevent buffer overflows.
	LPDWORD lpdw = (LPDWORD)m_pDlgBuf;
	*lpdw = dw;
	m_pDlgBuf += sizeof(DWORD);
}

inline void DialogBuilder::write_wstr(LPCWSTR wstr)
{
	// FIXME: Prevent buffer overflows.
	if (wstr) {
		size_t cbWstr = (wcslen(wstr) + 1) * sizeof(wchar_t);
		memcpy(m_pDlgBuf, wstr, cbWstr);
		m_pDlgBuf += cbWstr;
	} else {
		// NULL string.
		LPWORD lpw = (LPWORD)m_pDlgBuf;
		*lpw = 0;
		m_pDlgBuf += sizeof(WORD);
	}
}

inline void DialogBuilder::write_wstr_ord(LPCWSTR wstr)
{
	// FIXME: Prevent buffer overflows.
	if (((ULONG_PTR)wstr) <= 0xFFFF) {
		// String is an atom.
		write_word(0xFFFF);
		write_word(((ULONG_PTR)wstr) & 0xFFFF);
	} else {
		// Not an atom. Write a normal string.
		write_wstr(wstr);
	}
}

inline void DialogBuilder::align_word(void)
{
	ULONG_PTR ul = (ULONG_PTR)m_pDlgBuf;
	ul = (ul + 1) & ~1;
	m_pDlgBuf = (uint8_t*)ul;
}

inline void DialogBuilder::align_dword(void)
{
	ULONG_PTR ul = (ULONG_PTR)m_pDlgBuf;
	ul = (ul + 3) & ~3;
	m_pDlgBuf = (uint8_t*)ul;
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

	// DLGTEMPLATEEX
	write_word(1);		// WORD wVersion
	write_word(0xFFFF);	// WORD wSignature
	write_dword(0);		// DWORD dwHelpID

	// Copy from DLGTEMPLATE.
	write_dword(lpTemplate->dwExtendedStyle);	// DWORD dwExtendedStyle
	write_dword(lpTemplate->style | DS_SETFONT);	// DWORD style
	m_pcDlgItems = (LPWORD)m_pDlgBuf;		// Save the cDlgItems pointer.
	write_word(0);					// WORD cDlgItems
	write_word(lpTemplate->x);			// short x
	write_word(lpTemplate->y);			// short y
	write_word(lpTemplate->cx);			// short cx
	write_word(lpTemplate->cy);			// short cy

	// No menu; default dialog class.
	write_word(0);		// sz_Or_Ord menu
	write_word(0);		// sz_Or_Ord windowClass

	// Dialog title.
	write_wstr(lpszTitle);

	// Font information.
	// TODO: Make DS_SETFONT optional?
	write_word(8);		// WORD wSize
	write_word(FW_NORMAL);	// WORD wWeight
	write_word(0);		// BYTE italic; BYTE charset;
	write_wstr(L"MS Shell Dlg");	// WCHAR typeface[]
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

	// Copy from DLGITEMTEMPLATE.
	write_dword(0);					// DWORD helpID
	write_dword(lpItemTemplate->dwExtendedStyle);	// DWORD exStyle
	write_dword(lpItemTemplate->style);		// DWORD style
	write_word(lpItemTemplate->x);			// short x
	write_word(lpItemTemplate->y);			// short y
	write_word(lpItemTemplate->cx);			// short cx
	write_word(lpItemTemplate->cy);			// short cy
	write_dword((DWORD)lpItemTemplate->id);		// WORD id

	// Window class and text.
	write_wstr_ord(lpszWindowClass);
	write_wstr_ord(lpszWindowText);

	write_word(0);					// WORD extraCount

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
