/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ITab.cpp: Configuration tab interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: We need a .cpp file here in order for
// automoc to generate a moc file.

#include "stdafx.h"
#include "ITab.hpp"

ITab::ITab(QWidget *parent)
	: super(parent)
{}
