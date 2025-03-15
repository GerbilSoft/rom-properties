/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ImageTypesTab.hpp: Image Types tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_IMAGE_TYPES_TAB (rp_image_types_tab_get_type())

#if GTK_CHECK_VERSION(3, 0, 0)
#  define _RpImageTypesTab_super	GtkBox
#  define _RpImageTypesTab_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
#  define _RpImageTypesTab_super	GtkVBox
#  define _RpImageTypesTab_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

G_DECLARE_FINAL_TYPE(RpImageTypesTab, rp_image_types_tab, RP, IMAGE_TYPES_TAB, _RpImageTypesTab_super)

GtkWidget	*rp_image_types_tab_new			(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
