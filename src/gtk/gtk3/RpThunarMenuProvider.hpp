/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * RpThunarMenuProvider.hpp: ThunarX Menu Provider Definition.             *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_RPTHUNARMENUPROVIDER_HPP__
#define __ROMPROPERTIES_GTK3_RPTHUNARMENUPROVIDER_HPP__

#include "RpThunarPlugin.hpp"

G_BEGIN_DECLS

typedef struct _RpThunarMenuProviderClass	RpThunarMenuProviderClass;
typedef struct _RpThunarMenuProvider		RpThunarMenuProvider;

#define TYPE_RP_THUNAR_MENU_PROVIDER		(rp_thunar_menu_provider_get_type())
#define RP_THUNAR_MENU_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_THUNAR_MENU_PROVIDER, RpThunarMenuProvider))
#define RP_THUNAR_MENU_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_THUNAR_MENU_PROVIDER, RpThunarMenuProviderClass))
#define IS_RP_THUNAR_MENU_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_THUNAR_MENU_PROVIDER))
#define IS_RP_THUNAR_MENU_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_THUNAR_MENU_PROVIDER))
#define RP_THUNAR_MENU_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_THUNAR_MENU_PROVIDER, RpThunarMenuProviderClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_thunar_menu_provider_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_thunar_menu_provider_register_type_ext	(ThunarxProviderPlugin *plugin) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__ROMPROPERTIES_GTK3_RPTHUNARMENUPROVIDER_HPP__ */
