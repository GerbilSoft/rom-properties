/*******************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                             *
 * NautilusExtraInterfaces.c: Extra interfaces in Nautilus-based file managers *
 *                                                                             *
 * Copyright (c) 2017-2023 by David Korth.                                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                                   *
 *******************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"
#include "NautilusExtraInterfaces.h"

#include "plugin-helper.h"

#ifdef HAVE_NEMO_INTERFACES
// Nemo-specific function pointers
PFN_NEMO_NAME_AND_DESC_PROVIDER_GET_TYPE	pfn_nemo_name_and_desc_provider_get_type;

// Actual plugin types are needed to determine the name and description.
#  include "NautilusPropertyPageProvider.hpp"
#  include "NautilusMenuProvider.h"
#endif /* HAVE_NEMO_INTERFACES */

/** rom-properties functions **/

#ifdef HAVE_NEMO_INTERFACES
/**
 * Initialize Nemo-specific function pointers.
 * @param libextension_so dlopen()'d handle to libcaja-extension.so.
 */
void
rp_nemo_init(void *libextension_so)
{
	DLSYM(nemo_name_and_desc_provider_get_type, nemo_name_and_desc_provider_get_type);
}

/**
 * NemoNameAndDescProvider::get_name_and_desc() function
 * @param provider NemoNameAndDescProvider
 * @return Name and description
 */
static GList*
rp_nemo_name_and_desc_provider_get_name_and_desc(NemoNameAndDescProvider *provider)
{
	GList *ret = NULL;

	if (RP_IS_NAUTILUS_PROPERTY_PAGE_PROVIDER(provider)) {
		ret = g_list_append(NULL, g_strdup("ROM Properties Page:::Property page extension"));
	} else if (RP_IS_NAUTILUS_MENU_PROVIDER(provider)) {
		ret = g_list_append(NULL, g_strdup("ROM Properties Page:::Menu extension"));
	} else {
		assert(!"Not a supported GObject class!");
	}

	return ret;
}

/**
 * NemoNameAndDescProviderInterface initialization function
 * @param iface
 */
static void
rp_nemo_name_and_desc_provider_init(NemoNameAndDescProviderInterface *iface)
{
	iface->get_name_and_desc = rp_nemo_name_and_desc_provider_get_name_and_desc;
}

#endif /* HAVE_NEMO_INTERFACES */

/**
 * Add extra fork-specific interfaces.
 * Call this function from rp_*_provider_register_type_ext().
 * @param g_module GTypeModule
 * @param instance_type Instance type
 */
void
rp_nautilus_extra_interfaces_add(GTypeModule *g_module, GType instance_type)
{
	// It's used in some builds, but not others.
	RP_UNUSED(g_module);

#ifdef HAVE_NEMO_INTERFACES
	// If running in Nemo, add the NemoNameAndDescProvider interface.
	if (pfn_nemo_name_and_desc_provider_get_type) {
		static const GInterfaceInfo g_implement_interface_info = {
			(GInterfaceInitFunc)(void (*)(void))rp_nemo_name_and_desc_provider_init, NULL, NULL
		};
		g_type_module_add_interface(g_module, instance_type,
			NEMO_TYPE_NAME_AND_DESC_PROVIDER, &g_implement_interface_info);
	}
#endif /* HAVE_NEMO_INTERFACES */
}
