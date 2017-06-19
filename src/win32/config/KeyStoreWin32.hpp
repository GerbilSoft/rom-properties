/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStoreWin32.hpp: Key store object for Windows.                        *
 *                                                                         *
 * Copyright (c) 2012-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_WIN32_CONFIG_KEYSTOREWIN32_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_KEYSTOREWIN32_HPP__

#include "libromdata/crypto/KeyStoreUI.hpp"
#include "librpbase/common.h"

class KeyStoreWin32 : public LibRomData::KeyStoreUI
{
	public:
		explicit KeyStoreWin32(HWND hWnd);
		virtual ~KeyStoreWin32();

	private:
		RP_DISABLE_COPY(KeyStoreWin32)
		HWND m_hWnd;

	public:
		/**
		 * Get the window.
		 * @return Window.
		 */
		inline HWND hWnd(void) const { return m_hWnd; }

		/**
		 * Set the window.
		 * @param hWnd Window.
		 */
		inline void setHWnd(HWND hWnd) { m_hWnd = hWnd; }

	protected:
		/** Pure virtual functions for Qt signals. **/

		/**
		 * A key has changed.
		 * @param sectIdx Section index.
		 * @param keyIdx Key index.
		 */
		virtual void keyChanged_int(int sectIdx, int keyIdx) override final;

		/**
		 * A key has changed.
		 * @param idx Flat key index.
		 */
		virtual void keyChanged_int(int idx) override final;

		/**
		 * All keys have changed.
		 */
		virtual void allKeysChanged_int(void) override final;

		/**
		 * KeyStore has been changed by the user.
		 */
		virtual void modified_int(void) override final;
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_KEYSTOREWIN32_HPP__ */
