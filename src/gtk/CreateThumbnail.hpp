/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CreateThumbnail.hpp: Thumbnail creator for wrapper programs.            *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// GLib
#include <glib.h>
#include <gmodule.h>

// for RP_C_API
#include "dll-macros.h"

G_BEGIN_DECLS

/**
 * Thumbnail creator function for wrapper programs. (v2)
 * @param source_file Source file or URI (UTF-8)
 * @param output_file Output file (UTF-8)
 * @param maximum_size Maximum size
 * @param flags Flags (see RpCreateThumbnailFlags)
 * @return 0 on success; non-zero on error.
 */
G_MODULE_EXPORT int RP_C_API rp_create_thumbnail2(
	const char *source_file, const char *output_file, int maximum_size, unsigned int flags);

G_END_DECLS
