/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * rp_log_domain.h: G_LOG_DOMAIN for rom-properties.                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// PrecompiledHeader.cmake's FILE(GENERATE) command mangles the
// escaped double-quotes for G_LOG_DOMAIN.
#if defined(RP_UI_GTK4)
#  define G_LOG_DOMAIN "rom-properties-gtk4"
#elif defined(RP_UI_GTK3)
#  define G_LOG_DOMAIN "rom-properties-gtk3"
#elif defined(RP_UI_XFCE)
#  define G_LOG_DOMAIN "rom-properties-xfce"
#else
#  define RP_IS_GLIB_ONLY 1
#  define G_LOG_DOMAIN "rom-properties-glib"
#endif
