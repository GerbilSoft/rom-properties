/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * RpThunarPage.hpp: ThunarX Properties Page.                              *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_RPTHUNARPAGE_HPP__
#define __ROMPROPERTIES_GTK3_RPTHUNARPAGE_HPP__

#include "RpThunarPlugin.h"

G_BEGIN_DECLS

typedef struct _RpThunarPageClass	RpThunarPageClass;
typedef struct _RpThunarPage		RpThunarPage;

#define TYPE_RP_THUNAR_PAGE            (rp_thunar_page_get_type())
#define RP_THUNAR_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_THUNAR_PAGE, RpThunarPage))
#define RP_THUNAR_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_THUNAR_PAGE, RpThunarPageClass))
#define IS_RP_THUNAR_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_THUNAR_PAGE))
#define IS_RP_THUNAR_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_THUNAR_PAGE))
#define RP_THUNAR_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_THUNAR_PAGE, RpThunarPageClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_thunar_page_get_type			(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_thunar_page_register_type_ext	(ThunarxProviderPlugin *plugin) G_GNUC_INTERNAL;

RpThunarPage		*rp_thunar_page_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

ThunarxFileInfo		*rp_thunar_page_get_file	(RpThunarPage	*page) G_GNUC_INTERNAL;
void			rp_thunar_page_set_file		(RpThunarPage	*page,
							 ThunarxFileInfo *file) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK3_RPTHUNARPAGE_HPP__ */
