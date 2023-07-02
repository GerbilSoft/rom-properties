/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * plugin-helper.h: Plugin helper macros.                                  *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <dlfcn.h>
#include <unistd.h>

#include <glib.h>
#include "check-uid.h"

G_BEGIN_DECLS

#ifdef G_ENABLE_DEBUG
#  define SHOW_INIT_MESSAGE() g_message("Initializing " G_LOG_DOMAIN " extension")
#else
#  define SHOW_INIT_MESSAGE() do { } while (0)
#endif

#if GTK_CHECK_VERSION(3,0,0)
#  define VERIFY_GTK_VERSION() do { \
	/** \
	 * Make sure the correct GTK version is loaded. \
	 * NOTE: Can only do this for GTK3/GTK4, since GTK2 doesn't \
	 * have an easily-accessible runtime version function. \
	 */ \
	const guint gtk_major = gtk_get_major_version(); \
	if (gtk_major != GTK_MAJOR_VERSION) { \
		g_critical("expected GTK%u, found GTK%u; not registering", \
			(unsigned int)GTK_MAJOR_VERSION, gtk_major); \
		return; \
	} \
} while (0)
#else /* !GTK_CHECK_VERSION(3,0,0) */
#  define VERIFY_GTK_VERSION() do { } while (0)
#endif /* GTK_CHECK_VERSION(3,0,0) */

#define DLSYM(symvar, symdlopen) do { \
	pfn_##symvar = (__typeof__(pfn_##symvar))dlsym(libextension_so, #symdlopen); \
	if (!pfn_##symvar) { \
		g_critical("*** " G_LOG_DOMAIN ": dlsym(%s) failed: %s\n", #symdlopen, dlerror()); \
		dlclose(libextension_so); \
		libextension_so = NULL; \
		return; \
	} \
} while (0)

G_END_DECLS
