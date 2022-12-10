/******************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                            *
 * RpNautilusMenuProvider.cpp: Nautilus (and forks) Menu Provider Definition. *
 *                                                                            *
 * Copyright (c) 2017-2022 by David Korth.                                    *
 * SPDX-License-Identifier: GPL-2.0-or-later                                  *
 ******************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_RPNAUTILUSMENUPROVIDER_HPP__
#define __ROMPROPERTIES_GTK3_RPNAUTILUSMENUPROVIDER_HPP__

#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(4,0,0)
#  include "../gtk4/RpNautilusPlugin.hpp"
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  include "RpNautilusPlugin.hpp"
#endif /* GTK_CHECK_VERSION(4,0,0) */

G_BEGIN_DECLS

typedef struct _RpNautilusMenuProviderClass	RpNautilusMenuProviderClass;
typedef struct _RpNautilusMenuProvider		RpNautilusMenuProvider;

#define TYPE_RP_NAUTILUS_MENU_PROVIDER			(rp_nautilus_menu_provider_get_type())
#define RP_NAUTILUS_MENU_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_NAUTILUS_MENU_PROVIDER, RpNautilusMenuProvider))
#define RP_NAUTILUS_MENU_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_NAUTILUS_MENU_PROVIDER, RpNautilusMenuProviderClass))
#define IS_RP_NAUTILUS_MENU_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_NAUTILUS_MENU_PROVIDER))
#define IS_RP_NAUTILUS_MENU_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_NAUTILUS_MENU_PROVIDER))
#define RP_NAUTILUS_MENU_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_NAUTILUS_MENU_PROVIDER, RpNautilusMenuProviderClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_nautilus_menu_provider_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_nautilus_menu_provider_register_type_ext	(GTypeModule *module) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__ROMPROPERTIES_GTK3_RPNAUTILUSMENUPROVIDER_HPP__ */
