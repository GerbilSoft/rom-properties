/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-file-utils.h: File utilities from tracker-miners-3.6.2.         *
 *                                                                         *
 * Copyright (C) 2006, Jamie McCracken <jamiemcc@gnome.org>                *
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>                        *
 * SPDX-License-Identifier: LGPL-2.1-or-later                              *
 ***************************************************************************/

// These functions are used by Tracker's own extractor modules,
// but are not exported by libtracker_extractor. They're part of
// libtracker-miners-common, which is statically-linked.
#pragma once

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

gchar *tracker_file_get_content_identifier(GFile *file, GFileInfo *info, const gchar *suffix);

G_END_DECLS
