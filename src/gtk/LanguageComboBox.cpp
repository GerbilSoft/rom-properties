/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBox.cpp: Language GtkComboBox subclass                     *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PIMGTYPE.hpp"

#include "LanguageComboBox.hpp"
#include "LanguageComboBox_p.hpp"

#include "FlagSpriteSheet.hpp"

#ifdef USE_GTK_DROP_DOWN
#  include "LanguageComboBoxItem.h"
#endif /* USE_GTK_DROP_DOWN */

// C++ STL classes
using std::string;

static void	rp_language_combo_box_set_property(GObject	*object,
						   guint	 prop_id,
						   const GValue	*value,
						   GParamSpec	*pspec);
static void	rp_language_combo_box_get_property(GObject	*object,
						   guint	 prop_id,
						   GValue	*value,
						   GParamSpec	*pspec);

static void	rp_language_combo_box_dispose     (GObject	*object);

// Needed by both _gtk3.cpp and _gtk4.cpp.
GParamSpec *rp_language_combo_box_props[PROP_LAST];

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpLanguageComboBox, rp_language_combo_box,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
rp_language_combo_box_class_init(RpLanguageComboBoxClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_language_combo_box_set_property;
	gobject_class->get_property = rp_language_combo_box_get_property;
	gobject_class->dispose = rp_language_combo_box_dispose;

	/** Properties **/

	rp_language_combo_box_props[PROP_SELECTED_LC] = g_param_spec_uint(
		"selected-lc", "Selected LC", "Selected language code.",
		0U, ~0U, 0U,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	rp_language_combo_box_props[PROP_FORCE_PAL] = g_param_spec_boolean(
		"force-pal", "Force PAL", "Force PAL regions.",
		false,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, rp_language_combo_box_props);
}

static void
rp_language_combo_box_init(RpLanguageComboBox *widget)
{
	// NOTE: This function is defined by G_DEFINE_TYPE_EXTENDED().
	// It cannot be changed to non-static, so we'll call a sub-function.
	rp_language_combo_box_init_gtkX(widget);
}

GtkWidget*
rp_language_combo_box_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_LANGUAGE_COMBO_BOX, nullptr));
}

/** Properties **/

static void
rp_language_combo_box_set_property(GObject	*object,
				   guint	 prop_id,
				   const GValue	*value,
				   GParamSpec	*pspec)
{
	RpLanguageComboBox *const widget = RP_LANGUAGE_COMBO_BOX(object);

	switch (prop_id) {
		case PROP_SELECTED_LC:
			rp_language_combo_box_set_selected_lc(widget, g_value_get_uint(value));
			break;

		case PROP_FORCE_PAL:
			rp_language_combo_box_set_force_pal(widget, g_value_get_boolean(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_language_combo_box_get_property(GObject	*object,
				   guint	 prop_id,
				   GValue	*value,
				   GParamSpec	*pspec)
{
	RpLanguageComboBox *const widget = RP_LANGUAGE_COMBO_BOX(object);

	switch (prop_id) {
		case PROP_SELECTED_LC:
			g_value_set_uint(value, rp_language_combo_box_get_selected_lc(widget));
			break;

		case PROP_FORCE_PAL:
			g_value_set_boolean(value, widget->forcePAL);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Dispose / Finalize **/

static void
rp_language_combo_box_dispose(GObject *object)
{
#ifdef USE_GTK_DROP_DOWN
	RpLanguageComboBox *const widget = RP_LANGUAGE_COMBO_BOX(object);

	if (widget->dropDown) {
		gtk_drop_down_set_factory(GTK_DROP_DOWN(widget->dropDown), nullptr);
	}
#endif /* USE_GTK_DROP_DOWN */

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_language_combo_box_parent_class)->dispose(object);
}

/**
 * Set the Force PAL setting.
 * @param forcePAL Force PAL setting.
 */
void
rp_language_combo_box_set_force_pal(RpLanguageComboBox *widget, gboolean forcePAL)
{
	g_return_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget));

	if (widget->forcePAL == forcePAL)
		return;

	widget->forcePAL = forcePAL;
	rp_language_combo_box_rebuild_icons(widget);
	g_object_notify_by_pspec(G_OBJECT(widget), rp_language_combo_box_props[PROP_FORCE_PAL]);
}

/**
 * Get the Force PAL setting.
 * @return Force PAL setting.
 */
gboolean
rp_language_combo_box_get_force_pal(RpLanguageComboBox *widget)
{
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget), false);
	return widget->forcePAL;
}
