/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyStoreWin32.cpp: Key store object for Windows.                        *
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

#include "stdafx.h"
#include "KeyStoreWin32.hpp"

/**
 * Create a new KeyStore object.
 */
KeyStoreWin32::KeyStoreWin32()
{
	// Load the keys.
	reset();
}

KeyStoreWin32::~KeyStoreWin32()
{ }

/** Pure virtual functions for Win32 notifications. **/

/**
 * A key has changed.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 */
void KeyStoreWin32::keyChanged_int(int sectIdx, int keyIdx)
{
	// TODO
	//emit keyChanged(sectIdx, keyIdx);
}

/**
 * A key has changed.
 * @param idx Flat key index.
 */
void KeyStoreWin32::keyChanged_int(int idx)
{
	// TODO
	//emit keyChanged(idx);
}

/**
 * All keys have changed.
 */
void KeyStoreWin32::allKeysChanged_int(void)
{
	// TODO
	//emit allKeysChanged();
}

/**
 * KeyStore has been changed by the user.
 */
void KeyStoreWin32::modified_int(void)
{
	// TODO
	//emit modified();
}
