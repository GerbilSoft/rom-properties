/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * RpNautilusProvider.hpp: Nautilus (and forks) Provider Definition.       *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_RPNAUTILUSPROVIDER_HPP__
#define __ROMPROPERTIES_GTK3_RPNAUTILUSPROVIDER_HPP__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _RpNautilusProviderClass	RpNautilusProviderClass;
typedef struct _RpNautilusProvider	RpNautilusProvider;

#define TYPE_RP_NAUTILUS_PROVIDER		(rp_nautilus_provider_get_type())
#define RP_NAUTILUS_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_NAUTILUS_PROVIDER, RpNautilusProvider))
#define RP_NAUTILUS_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_NAUTILUS_PROVIDER, RpNautilusProviderClass))
#define IS_RP_NAUTILUS_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_NAUTILUS_PROVIDER))
#define IS_RP_NAUTILUS_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_NAUTILUS_PROVIDER))
#define RP_NAUTILUS_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_NAUTILUS_PROVIDER, RpNautilusProviderClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_nautilus_provider_get_type	(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_nautilus_provider_register_type_ext(GTypeModule *module) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__ROMPROPERTIES_GTK3_RPNAUTILUSPROVIDER_HPP__ */
