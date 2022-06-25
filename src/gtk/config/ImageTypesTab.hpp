/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ImageTypesTab.hpp: Image Types tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_IMAGETYPESTAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_IMAGETYPESTAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _ImageTypesTabClass	ImageTypesTabClass;
typedef struct _ImageTypesTab		ImageTypesTab;

#define TYPE_IMAGE_TYPES_TAB            (image_types_tab_get_type())
#define IMAGE_TYPES_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_IMAGE_TYPES_TAB, ImageTypesTab))
#define IMAGE_TYPES_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_IMAGE_TYPES_TAB, ImageTypesTabClass))
#define IS_IMAGE_TYPES_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_IMAGE_TYPES_TAB))
#define IS_IMAGE_TYPES_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_IMAGE_TYPES_TAB))
#define IMAGE_TYPES_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_IMAGE_TYPES_TAB, ImageTypesTabClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		image_types_tab_get_type	(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		image_types_tab_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*image_types_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_IMAGETYPESTAB_HPP__ */
