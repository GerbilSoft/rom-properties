/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * stdafx.h: Common definitions and includes.                              *
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

#ifdef __cplusplus
/** C++ **/

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>

// C++ includes
#include <algorithm>
#include <array>
#include <forward_list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// libfmt
#include "rp-libfmt.h"

#else /* !__cplusplus */
/** C **/

// C includes
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#endif /* __cplusplus */

// NOTE: Thunar-1.8.0's thunarx-renamer.h depends on GtkVBox,
// which is deprecated in GTK+ 3.x.
#ifdef GTK_DISABLE_DEPRECATED
#  undef GTK_DISABLE_DEPRECATED
#endif

// GLib compatibility functions (includes glib.h)
#include "glib-compat.h"
#include <glib-object.h>
#include <gio/gio.h>

// GTK+ compatibility functions (includes gtk.h)
#ifndef RP_IS_GLIB_ONLY
#  include "gtk-compat.h"
#endif /* !RP_IS_GLIB_ONLY */

// GLib on non-Windows platforms (prior to 2.53.1) defines G_MODULE_EXPORT to a no-op.
// This doesn't work when we use symbol visibility settings.
#if !GLIB_CHECK_VERSION(2,53,1) && !defined(_WIN32) && (defined(__GNUC__) && __GNUC__ >= 4)
#  ifdef G_MODULE_EXPORT
#    undef G_MODULE_EXPORT
#  endif
#  define G_MODULE_EXPORT __attribute__((visibility("default")))
#endif /* !GLIB_CHECK_VERSION(2,53,1) && !_WIN32 && __GNUC__ >= 4 */

// libi18n
#include "libi18n/i18n.h"

// librpbase common headers
#include "common.h"
#include "aligned_malloc.h"
#include "ctypex.h"
#include "dll-macros.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/config/Config.hpp"
#include "librpbase/img/RpPngWriter.hpp"

// librpfile C++ headers
#include "librpfile/FileSystem.hpp"
#include "librpfile/IRpFile.hpp"
#include "librpfile/RpFile.hpp"

// librptexture C++ headers
#include "librptexture/img/rp_image.hpp"

// librptext C++ headers
#include "librptext/conversion.hpp"
#endif /* !__cplusplus */

// GTK+ UI frontend headers
#ifndef RP_IS_GLIB_ONLY
#  include "PIMGTYPE.hpp"
#  ifdef RP_GTK_USE_CAIRO
#    include <cairo.h>
#    include <cairo-gobject.h>
#  else /* !RP_GTK_USE_CAIRO */
#    if GTK_CHECK_VERSION(4, 11, 3)
// TODO: Find a suitable replacement for GdkPixbuf.
#      include <gdk/deprecated/gdkpixbuf.h>
#    else /* !GTK_CHECK_VERSION(4, 11, 3) */
#      include <gdk/gdkpixbuf.h>
#    endif /* GTK_CHECK_VERSION(4, 11, 3) */
#  endif /* RP_GTK_USE_CAIRO */
#endif /* !RP_IS_GLIB_ONLY */

#ifdef __cplusplus
#  include "RpFile_gio.hpp"
#endif /* __cplusplus */
