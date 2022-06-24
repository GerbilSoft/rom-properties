/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_STDAFX_H__
#define __ROMPROPERTIES_GTK_STDAFX_H__

// PrecompiledHeader.cmake's FILE(GENERATE) command mangles the
// escaped double-quotes for G_LOG_DOMAIN.
#if defined(RP_UI_GTK4)
#  define G_LOG_DOMAIN "rom-properties-gtk4"
#elif defined(RP_UI_GTK3)
#  define G_LOG_DOMAIN "rom-properties-gtk3"
#elif defined(RP_UI_XFCE)
#  define G_LOG_DOMAIN "rom-properties-xfce"
#else
#  error RP_UI macro not defined
#endif

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>
#include <cinttypes>
#include <stdint.h>

// C++ includes.
#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>

#endif /* __cplusplus */

// NOTE: Thunar-1.8.0's thunarx-renamer.h depends on GtkVBox,
// which is deprecated in GTK+ 3.x.
#ifdef GTK_DISABLE_DEPRECATED
#  undef GTK_DISABLE_DEPRECATED
#endif

// GTK+ includes.
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

// GTK+ 2.x compatibility functions.
#include "gtk-compat.h"

// GLib on non-Windows platforms defines G_MODULE_EXPORT to a no-op.
// This doesn't work when we use symbol visibility settings.
#if !defined(_WIN32) && (defined(__GNUC__) && __GNUC__ >= 4)
#  ifdef G_MODULE_EXPORT
#    undef G_MODULE_EXPORT
#  endif
#  define G_MODULE_EXPORT __attribute__ ((visibility ("default")))
#endif /* !_WIN32 && __GNUC__ >= 4 */

// libi18n
#include "libi18n/i18n.h"

// librpbase common headers
#include "common.h"
#include "librpbase/aligned_malloc.h"

// librpcpu
#include "librpcpu/cpu_dispatch.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/config/Config.hpp"
#include "librpbase/img/RpPngWriter.hpp"

// librpfile C++ headers
#include "librpfile/FileSystem.hpp"
#include "librpfile/IRpFile.hpp"
#include "librpfile/RpFile.hpp"

// librptexture C++ headers
#include "librptexture/img/rp_image.hpp"
#endif /* !__cplusplus */

// GTK+ UI frontend headers
#include "PIMGTYPE.hpp"
#ifdef RP_GTK_USE_CAIRO
#  include <cairo.h>
#  include <cairo-gobject.h>
#else /* !RP_GTK_USE_CAIRO */
#  include <gdk/gdkpixbuf.h>
#endif /* RP_GTK_USE_CAIRO */

#ifdef __cplusplus
#  include "RpFile_gio.hpp"
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_GTK_STDAFX_H__ */
