/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreQt.cpp: Key store object for Qt.                                *
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

#include "KeyStoreQt.hpp"

/**
 * Create a new KeyStore object.
 * @param parent Parent object.
 */
KeyStoreQt::KeyStoreQt(QObject *parent)
	: super(parent)
{
	// Load the keys.
	reset();
}

KeyStoreQt::~KeyStoreQt()
{ }

/** Pure virtual functions for Qt signals. **/

/**
 * A key has changed.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 */
void KeyStoreQt::keyChanged_int(int sectIdx, int keyIdx)
{
	emit keyChanged(sectIdx, keyIdx);
}

/**
 * A key has changed.
 * @param idx Flat key index.
 */
void KeyStoreQt::keyChanged_int(int idx)
{
	emit keyChanged(idx);
}

/**
 * All keys have changed.
 */
void KeyStoreQt::allKeysChanged_int(void)
{
	emit allKeysChanged();
}

/**
 * KeyStore has been changed by the user.
 */
void KeyStoreQt::modified_int(void)
{
	emit modified();
}
