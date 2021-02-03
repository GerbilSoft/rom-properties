/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * RpThunarProvider.hpp: ThunarX Provider Definition.                      *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_RPTHUNARPROVIDER_HPP__
#define __ROMPROPERTIES_GTK3_RPTHUNARPROVIDER_HPP__

#include "RpThunarPlugin.hpp"

G_BEGIN_DECLS

typedef struct _RpThunarProviderClass	RpThunarProviderClass;
typedef struct _RpThunarProvider	RpThunarProvider;

#define TYPE_RP_THUNAR_PROVIDER			(rp_thunar_provider_get_type())
#define RP_THUNAR_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_THUNAR_PROVIDER, RpThunarProvider))
#define RP_THUNAR_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_THUNAR_PROVIDER, RpThunarProviderClass))
#define IS_RP_THUNAR_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_THUNAR_PROVIDER))
#define IS_RP_THUNAR_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_THUNAR_PROVIDER))
#define RP_THUNAR_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_THUNAR_PROVIDER, RpThunarProviderClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_thunar_provider_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_thunar_provider_register_type_ext	(ThunarxProviderPlugin *plugin) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__ROMPROPERTIES_GTK3_RPTHUNARPROVIDER_HPP__ */
