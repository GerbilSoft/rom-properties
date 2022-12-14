/********************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                              *
 * NautilusPropertyPageProvider.hpp: Nautilus Property Page Provider Definition *
 *                                                                              *
 * Copyright (c) 2017-2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                                    *
 ********************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_NAUTILUSPROPERTYPAGEPROVIDER_HPP__
#define __ROMPROPERTIES_GTK3_NAUTILUSPROPERTYPAGEPROVIDER_HPP__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _RpNautilusPropertyPageProviderClass	RpNautilusPropertyPageProviderClass;
typedef struct _RpNautilusPropertyPageProvider		RpNautilusPropertyPageProvider;

#define TYPE_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER			(rp_nautilus_property_page_provider_get_type())
#define RP_NAUTILUS_PROPERTY_PAGE_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER, RpNautilusPropertyPageProvider))
#define RP_NAUTILUS_PROPERTY_PAGE_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER, RpNautilusPropertyPageProviderClass))
#define IS_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER))
#define IS_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER))
#define RP_NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_NAUTILUS_PROPERTY_PAGE_PROVIDER, RpNautilusPropertyPageProviderClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_nautilus_property_page_provider_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_nautilus_property_page_provider_register_type_ext	(GTypeModule *module) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__ROMPROPERTIES_GTK3_NAUTILUSPROPERTYPAGEPROVIDER_HPP__ */
