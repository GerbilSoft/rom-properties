/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStoreWin32.hpp: Key store object for Windows.                        *
 *                                                                         *
 * Copyright (c) 2012-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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
		/** Pure virtual functions for base class signals. **/

		/**
		 * A key has changed.
		 * @param sectIdx Section index.
		 * @param keyIdx Key index.
		 */
		void keyChanged_int(int sectIdx, int keyIdx) final;

		/**
		 * A key has changed.
		 * @param idx Flat key index.
		 */
		void keyChanged_int(int idx) final;

		/**
		 * All keys have changed.
		 */
		void allKeysChanged_int(void) final;

		/**
		 * KeyStore has been changed by the user.
		 */
		void modified_int(void) final;
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_KEYSTOREWIN32_HPP__ */
