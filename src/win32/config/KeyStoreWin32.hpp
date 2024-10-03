/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStoreWin32.hpp: Key store object for Windows.                        *
 *                                                                         *
 * Copyright (c) 2012-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libromdata/crypto/KeyStoreUI.hpp"
#include "common.h"

class KeyStoreWin32 final : public LibRomData::KeyStoreUI
{
public:
	explicit KeyStoreWin32(HWND hWnd);

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
	/** Pure virtual functions for base class signals **/

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
