/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DialogBuilder.hpp: DLGTEMPLATEEX builder class.                         *
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

#ifndef __ROMPROPERTIES_WIN32_DIALOGBUILDER_HPP__
#define __ROMPROPERTIES_WIN32_DIALOGBUILDER_HPP__

// C includes.
#include <stdint.h>

class DialogBuilder
{
	public:
		DialogBuilder();
		~DialogBuilder();

	private:
		DialogBuilder(const DialogBuilder &);
		DialogBuilder &operator=(const DialogBuilder &);

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

	protected:
		// DLGTEMPLATEEX data.
		// TODO: Smaller maximum size and/or dynamic allocation?
		uint8_t m_DlgBuf[32768];

		// Current pointer into m_DlgBuf.
		uint8_t *m_pDlgBuf;

		// Pointer to DLGTEMPLATEEX's cDlgItems.
		LPWORD m_pcDlgItems;
};

#endif /* __ROMPROPERTIES_WIN32_DIALOGBUILDER_HPP__ */
