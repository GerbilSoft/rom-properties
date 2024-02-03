/**************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpGtkCpp.cpp: glib/gtk+ wrappers for some libromdata functionality.     *
 * (C++-specific functionality)                                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpGtkCpp.hpp"

// C++ STL classes
using std::string;

/**
 * Convert Win32/Qt-style accelerator notation ('&') to GTK-style ('_').
 * @param str String with '&' accelerator
 * @return String with '_' accelerator
 */
string convert_accel_to_gtk(const char *str)
{
	// GTK+ uses '_' for accelerators, not '&'.
	string s_ret = str;
	const size_t accel_pos = s_ret.find('&');
	if (accel_pos != string::npos) {
		s_ret[accel_pos] = '_';
	}
	return s_ret;
}
