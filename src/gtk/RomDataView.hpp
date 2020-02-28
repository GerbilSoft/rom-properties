/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.hpp: RomData viewer widget.                                 *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_ROMDATA_VIEW_HPP__
#define __ROMPROPERTIES_GTK_ROMDATA_VIEW_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _RomDataViewClass	RomDataViewClass;
typedef struct _RomDataView		RomDataView;

#define TYPE_ROM_DATA_VIEW            (rom_data_view_get_type())
#define ROM_DATA_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ROM_DATA_VIEW, RomDataView))
#define ROM_DATA_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ROM_DATA_VIEW, RomDataViewClass))
#define IS_ROM_DATA_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ROM_DATA_VIEW))
#define IS_ROM_DATA_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ROM_DATA_VIEW))
#define ROM_DATA_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ROM_DATA_VIEW, RomDataViewClass))

/** RpDescFormatType: How the "description" label is formatted. **/
typedef enum {
	RP_DFT_XFCE	= 0,	/*< nick=XFCE style (default) >*/
	RP_DFT_GNOME	= 1,	/*< nick=GNOME style >*/

	RP_DFT_LAST
} RpDescFormatType;

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		rom_data_view_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		rom_data_view_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*rom_data_view_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

const gchar	*rom_data_view_get_uri		(RomDataView	*page) G_GNUC_INTERNAL;
void		rom_data_view_set_uri		(RomDataView	*page,
						 const gchar	*uri) G_GNUC_INTERNAL;

RpDescFormatType rom_data_view_get_desc_format_type(RomDataView *page) G_GNUC_INTERNAL;
void		rom_data_view_set_desc_format_type(RomDataView *page, RpDescFormatType desc_format_type);

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_ROMDATA_VIEW_HPP__ */
