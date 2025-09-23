/*******************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                             *
 * NautilusExtraInterfaces.c: Extra interfaces in Nautilus-based file managers *
 *                                                                             *
 * Copyright (c) 2017-2024 by David Korth.                                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                                   *
 *******************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"
#include "NautilusExtraInterfaces.h"

#include "plugin-helper.h"

#ifdef HAVE_CAJA_INTERFACES
// Caja-specific function pointers
PFN_CAJA_CONFIGURABLE_GET_TYPE			pfn_caja_configurable_get_type;

// ConfigDialog is used when running the configuration.
#include "../config/ConfigDialog.hpp"
static GtkWidget *configDialog = NULL;
#endif /* HAVE_CAJA_INTERFACES */

#ifdef HAVE_NEMO_INTERFACES
// Nemo-specific function pointers
PFN_NEMO_NAME_AND_DESC_PROVIDER_GET_TYPE	pfn_nemo_name_and_desc_provider_get_type;

// Actual plugin types are needed to determine the name and description.
#  include "NautilusPropertyPageProvider.hpp"
#  include "NautilusMenuProvider.h"
#  include "NautilusInfoProvider.hpp"
#endif /* HAVE_NEMO_INTERFACES */

/** rom-properties functions **/

#ifdef HAVE_CAJA_INTERFACES
/**
 * Initialize Caja-specific function pointers.
 * @param libextension_so dlopen()'d handle to libcaja-extension.so.
 */
void
rp_caja_init(void *libextension_so)
{
	DLSYM(caja_configurable_get_type, caja_configurable_get_type);
}

/**
 * ConfigDialog was closed by clicking the X button.
 * @param dialog ConfigDialog
 * @param event GdkEvent
 * @param user_data
 */
static gboolean
config_dialog_delete_event(RpConfigDialog *dialog, GdkEvent *event, gpointer user_data)
{
	RP_UNUSED(event);
	RP_UNUSED(user_data);

	if (configDialog == GTK_WIDGET(dialog)) {
		// It's a match! Clear our dialog variable.
		configDialog = NULL;
	}

	// Continue processing.
	return FALSE;
}

/**
 * Open the plugin configuration.
 * @param provider CajaConfigurable
 */
static void
rp_caja_run_config(CajaConfigurable *provider)
{
	RP_UNUSED(provider);

	// Show the configuration dialog.
	if (configDialog) {
		// Configuration dialog is already created.
		// Bring it to the foreground.
		gtk_window_present(GTK_WINDOW(configDialog));
		return;
	}

	configDialog = rp_config_dialog_new();
	gtk_widget_set_name(configDialog, "configDialog");
	gtk_widget_set_visible(configDialog, TRUE);

	// NOTE: Since this is run in the Caja process, it defaults to
	// the Caja icon. Override it with "media-flash".
	// TODO: Custom rom-properties icon?
	// FIXME: Doesn't work on Wayland...
	gtk_window_set_icon_name(GTK_WINDOW(configDialog), "media-flash");

	// NOTE: We can't access the GtkApplication here, so we'll
	// have to use the "delete-event" signal instead.
	g_signal_connect(configDialog, "delete-event", G_CALLBACK(config_dialog_delete_event), NULL);
}

/**
 * CajaConfigurableIface initialization function
 * @param iface
 */
static void
rp_caja_configurable_init(CajaConfigurableIface *iface)
{
	iface->run_config = rp_caja_run_config;
}
#endif /* HAVE_CAJA_INTERFACES */

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
	const char *s_plugin_desc = NULL;
	
	if (RP_IS_NAUTILUS_PROPERTY_PAGE_PROVIDER(provider)) {
		s_plugin_desc = C_("NautilusExtraInterfaces", "Property page extension");
	} else if (RP_IS_NAUTILUS_MENU_PROVIDER(provider)) {
		s_plugin_desc = C_("NautilusExtraInterfaces", "Menu extension");
	} else if (RP_IS_NAUTILUS_INFO_PROVIDER(provider)) {
		s_plugin_desc = C_("NautilusExtraInterfaces", "Info provider extension");
	} else {
		assert(!"Not a supported GObject class!");
	}

	GList *ret = NULL;
	if (s_plugin_desc) {
		const char *const s_ext_name = C_("NautilusExtraInterfaces", "ROM Properties Page");
		ret = g_list_prepend(NULL, g_strdup_printf("%s:::%s", s_ext_name, s_plugin_desc));
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

#ifdef HAVE_CAJA_INTERFACES
	// If running in Caja, add the CajaConfigurable interface.
	if (pfn_caja_configurable_get_type) {
		static const GInterfaceInfo g_implement_interface_info = {
			(GInterfaceInitFunc)(void (*)(void))rp_caja_configurable_init, NULL, NULL
		};
		g_type_module_add_interface(g_module, instance_type,
			CAJA_TYPE_CONFIGURABLE, &g_implement_interface_info);
	}
#endif /* HAVE_CAJA_INTERFACES */

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
