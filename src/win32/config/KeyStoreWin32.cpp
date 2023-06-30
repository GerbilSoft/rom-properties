/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStoreWin32.cpp: Key store object for Windows.                        *
 *                                                                         *
 * Copyright (c) 2012-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyStoreWin32.hpp"
#include "res/resource.h"

/**
 * Create a new KeyStore object.
 * @param hWnd Parent window for messages.
 */
KeyStoreWin32::KeyStoreWin32(HWND hWnd)
	: m_hWnd(hWnd)
{
	// Load the keys.
	reset();
}

/** Pure virtual functions for Win32 notifications. **/

/**
 * A key has changed.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 */
void KeyStoreWin32::keyChanged_int(int sectIdx, int keyIdx)
{
	if (m_hWnd) {
		KeyStore_KeyChanged_SectKey(m_hWnd, sectIdx, keyIdx);
	}
}

/**
 * A key has changed.
 * @param idx Flat key index.
 */
void KeyStoreWin32::keyChanged_int(int idx)
{
	if (m_hWnd) {
		KeyStore_KeyChanged_Idx(m_hWnd, idx);
	}
}

/**
 * All keys have changed.
 */
void KeyStoreWin32::allKeysChanged_int(void)
{
	if (m_hWnd) {
		KeyStore_AllKeysChanged_Idx(m_hWnd);
	}
}

/**
 * KeyStore has been changed by the user.
 */
void KeyStoreWin32::modified_int(void)
{
	if (m_hWnd) {
		KeyStore_Modified(m_hWnd);
	}
}
