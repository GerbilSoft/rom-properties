/**************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpGtkCpp.cpp: glib/gtk+ wrappers for some libromdata functionality.     *
 * (C++-specific functionality)                                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * Convert Win32/Qt-style accelerator notation ('&') to GTK-style ('_').
 * @param str String with '&' accelerator
 * @return String with '_' accelerator
 */
std::string convert_accel_to_gtk(const char *str);

G_END_DECLS
