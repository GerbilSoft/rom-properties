/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * LvData.cpp: ListView data internal implementation.                      *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LvData.hpp"

/**
 * Reset the sorting map.
 * This uses the "default" sort.
 */
void LvData::resetSortMap(void)
{
	vSortMap.resize(vvStr.size());
	const unsigned int size = static_cast<unsigned int>(vSortMap.size());
	for (unsigned int i = 0; i < size; i++) {
		vSortMap[i] = i;
	}
}
