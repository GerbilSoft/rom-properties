/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreQt.cpp: Key store object for Qt.                                *
 *                                                                         *
 * Copyright (c) 2012-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
