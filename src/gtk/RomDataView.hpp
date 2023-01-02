/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.hpp: RomData viewer widget.                                 *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_ROMDATAVIEW_HPP__
#define __ROMPROPERTIES_GTK_ROMDATAVIEW_HPP__

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_ROM_DATA_VIEW (rp_rom_data_view_get_type())
#if GTK_CHECK_VERSION(3,0,0)
G_DECLARE_FINAL_TYPE(RpRomDataView, rp_rom_data_view, RP, ROM_DATA_VIEW, GtkBox)
#else /* !GTK_CHECK_VERSION(3,0,0) */
G_DECLARE_FINAL_TYPE(RpRomDataView, rp_rom_data_view, RP, ROM_DATA_VIEW, GtkVBox)
#endif /* GTK_CHECK_VERSION(3,0,0) */

/** RpDescFormatType: How the "description" label is formatted. **/
typedef enum {
	RP_DFT_XFCE	= 0,	/*< nick=XFCE style (default) >*/
	RP_DFT_GNOME	= 1,	/*< nick=GNOME style >*/

	RP_DFT_LAST
} RpDescFormatType;

GtkWidget	*rp_rom_data_view_new		(void) G_GNUC_MALLOC;
GtkWidget	*rp_rom_data_view_new_with_uri	(const gchar	*uri,
						 RpDescFormatType desc_format_type) G_GNUC_MALLOC;

const gchar	*rp_rom_data_view_get_uri	(RpRomDataView	*page);
void		rp_rom_data_view_set_uri	(RpRomDataView	*page,
						 const gchar	*uri);

RpDescFormatType rp_rom_data_view_get_desc_format_type(RpRomDataView *page);
void		rp_rom_data_view_set_desc_format_type(RpRomDataView *page,
						   RpDescFormatType desc_format_type);

gboolean	rp_rom_data_view_is_showing_data(RpRomDataView	*page);

G_END_DECLS

#ifdef __cplusplus
namespace LibRpBase {
	class RomData;
};
GtkWidget	*rp_rom_data_view_new_with_romData(const gchar *uri,
						 LibRpBase::RomData *romData,
						 RpDescFormatType desc_format_type) G_GNUC_MALLOC;
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_GTK_ROMDATAVIEW_HPP__ */
