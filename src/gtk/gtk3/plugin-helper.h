/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * plugin-helper.h: Plugin helper macros.                                  *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_PLUGIN_HELPER_H__
#define __ROMPROPERTIES_GTK3_PLUGIN_HELPER_H__

#include <unistd.h>
#include <dlfcn.h>

#include <glib.h>
#include "check-uid.h"

G_BEGIN_DECLS

#ifdef G_ENABLE_DEBUG
# define SHOW_INIT_MESSAGE() g_message("Initializing " G_LOG_DOMAIN " extension")
#else
# define SHOW_INIT_MESSAGE() do { } while (0)
#endif

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

#endif /* __ROMPROPERTIES_GTK3_PLUGIN_HELPER_H__ */
