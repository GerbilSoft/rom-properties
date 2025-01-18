/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * DialogBuilder.hpp: DLGTEMPLATEEX builder class.                         *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

// Windows SDK
#include "RpWin32_sdk.h"

// Standard window classes.
// These macros use the ordinal value, which saves
// space in the generated dialog resource.
#define WC_ORD_BUTTON		MAKEINTATOM(0x0080)
#define WC_ORD_EDIT		MAKEINTATOM(0x0081)
#define WC_ORD_STATIC		MAKEINTATOM(0x0082)
#define WC_ORD_LISTBOX		MAKEINTATOM(0x0083)
#define WC_ORD_SCROLLBAR	MAKEINTATOM(0x0084)
#define WC_ORD_COMBOBOX		MAKEINTATOM(0x0085)

namespace LibWin32UI {

class DialogBuilder
{
public:
	DialogBuilder();

	// Disable copy/assignment constructors.
	DialogBuilder(const DialogBuilder &) = delete;
	DialogBuilder &operator=(const DialogBuilder &) = delete;

private:
	/** DLGTEMPLATEEX helper functions. **/
	inline void write_word(WORD w);
	inline void write_wstr(LPCWSTR wstr);
	inline void write_wstr_ord(LPCWSTR wstr);

	inline void align_dword(void);

public:
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
	void init(LPCDLGTEMPLATE lpTemplate, LPCWSTR lpszTitle);

	/**
	 * Add a control to the dialog.
	 * @param lpItemTemplate	[in] DLGITEMTEMPLATE.
	 * @param lpszWindowClass	[in] Window class. (May be an ordinal value.)
	 * @param lpszWindowText	[in, opt] Window text. (May be an ordinal value.)
	 */
	void add(const DLGITEMTEMPLATE *lpItemTemplate, LPCWSTR lpszWindowClass, LPCWSTR lpszWindowText);

	/**
	 * Get a pointer to the created DLGTEMPLATEEX.
	 * @return DLGTEMPLATE.
	 */
	LPCDLGTEMPLATE get(void) const;

	/**
	 * Clear the dialog template.
	 */
	void clear(void);

protected:
	// DLGTEMPLATEEX data.
	// TODO: Smaller maximum size and/or dynamic allocation?
	uint8_t m_DlgBuf[1024];

	// Current pointer into m_DlgBuf.
	uint8_t *m_pDlgBuf;

	// Pointer to DLGTEMPLATEEX's cDlgItems.
	LPWORD m_pcDlgItems;
};

} //namespace LibWin32UI
