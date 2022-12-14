/*****************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * NautilusPropertiesModelProvider.hpp: Nautilus properties model provider   *
 *                                                                           *
 * NOTE: Nautilus 43 only accepts key/value pairs for properties, instead of *
 * arbitrary GtkWidgets. As such, the properties returned will be more       *
 * limited than in previous versions.                                        *
 *                                                                           *
 * Copyright (c) 2017-2022 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#ifndef __ROMPROPERTIES_GTK4_NAUTILUSPROPERTIESMODELPROVIDER_HPP__
#define __ROMPROPERTIES_GTK4_NAUTILUSPROPERTIESMODELPROVIDER_HPP__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _RpNautilusPropertiesModelProviderClass	RpNautilusPropertiesModelProviderClass;
typedef struct _RpNautilusPropertiesModelProvider	RpNautilusPropertiesModelProvider;

#define TYPE_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER		(rp_nautilus_properties_model_provider_get_type())
#define RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER, RpNautilusPropertiesModelProvider))
#define RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER, RpNautilusPropertiesModelProviderClass))
#define IS_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER))
#define IS_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER))
#define RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_NAUTILUS_PROPERTIES_MODEL_PROVIDER, RpNautilusPropertiesModelProviderClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_nautilus_properties_model_provider_get_type	(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_nautilus_properties_model_provider_register_type_ext(GTypeModule *module) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__ROMPROPERTIES_GTK4_NAUTILUSPROPERTIESMODELPROVIDER_HPP__ */
