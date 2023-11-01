/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyStoreQt.cpp: Key store object for GTK.                               *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyStoreGTK.hpp"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_TOTAL_KEY_COUNT,
	PROP_CHANGED,

	PROP_LAST
} KeyStoreGTKPropID;

/* Signal identifiers */
typedef enum {
	SIGNAL_KEY_CHANGED,		// A key has changed (section/key index)
	SIGNAL_KEY_CHANGED_FLAT,	// A key has changed (flat key index)
	SIGNAL_ALL_KEYS_CHANGED,	// All keys have changed
	SIGNAL_MODIFIED,		// KeyStore has been changed by the user

	SIGNAL_LAST
} KeyStoreGTKSignalID;

static void	rp_key_store_gtk_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	rp_key_store_gtk_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);

static GParamSpec *props[PROP_LAST];
static guint signals[SIGNAL_LAST];

// KeyStoreGTK class
struct _RpKeyStoreGTKClass {
	GObjectClass __parent__;
};

// KeyStoreGTK instance
class RpKeyStoreGTKPrivate;
struct _RpKeyStoreGTK {
	GObject __parent__;

	RpKeyStoreGTKPrivate *d;
};

static void	rp_key_store_gtk_finalize		(GObject	*object);

/**
 * KeyStoreUI implementation for GTK+
 */
class RpKeyStoreGTKPrivate final : public LibRomData::KeyStoreUI
{
public:
	explicit RpKeyStoreGTKPrivate(RpKeyStoreGTK *q)
		: q(q)
	{}

private:
	RpKeyStoreGTK *const q;
	RP_DISABLE_COPY(RpKeyStoreGTKPrivate);

protected: /*signals:*/
	/**
	 * A key has changed.
	 * @param sectIdx Section index.
	 * @param keyIdx Key index.
	 */
	void keyChanged_int(int sectIdx, int keyIdx) final
	{
		g_signal_emit(q, signals[SIGNAL_KEY_CHANGED], 0, sectIdx, keyIdx);
	}

	/**
	 * A key has changed.
	 * @param idx Flat key index.
	 */
	void keyChanged_int(int idx) final
	{
		g_signal_emit(q, signals[SIGNAL_KEY_CHANGED_FLAT], 0, idx);
	}

	/**
	 * All keys have changed.
	 */
	void allKeysChanged_int(void) final
	{
		g_signal_emit(q, signals[SIGNAL_ALL_KEYS_CHANGED], 0);
	}

	/**
	 * KeyStore has been changed by the user.
	 */
	void modified_int(void) final
	{
		g_signal_emit(q, signals[SIGNAL_MODIFIED], 0);
	}
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpKeyStoreGTK, rp_key_store_gtk,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0), { });

static void
rp_key_store_gtk_class_init(RpKeyStoreGTKClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = rp_key_store_gtk_finalize;
	gobject_class->set_property = rp_key_store_gtk_set_property;
	gobject_class->get_property = rp_key_store_gtk_get_property;

	/** Properties **/

	props[PROP_TOTAL_KEY_COUNT] = g_param_spec_int(
		"total-key-count", "total-key-count", "Total key count",
		0, 99999, 0, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	props[PROP_CHANGED] = g_param_spec_boolean(
		"changed", "changed", "Has the user changed anything?",
		FALSE, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);

	/** Signals **/

	/**
	 * A key has changed.
	 * @param sectIdx Section index.
	 * @param keyIdx Key index.
	 */
	signals[SIGNAL_KEY_CHANGED] = g_signal_new("key-changed",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

	/**
	 * A key has changed.
	 * @param idx Flat key index.
	 */
	signals[SIGNAL_KEY_CHANGED_FLAT] = g_signal_new("key-changed-flat",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_INT);

	/**
	 * All keys have changed.
	 */
	signals[SIGNAL_ALL_KEYS_CHANGED] = g_signal_new("all-keys-changed",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);

	/**
	 * KeyStore has been changed by the user.
	 */
	signals[SIGNAL_MODIFIED] = g_signal_new("modified",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);
}

static void
rp_key_store_gtk_init(RpKeyStoreGTK *tab)
{
	// Initialize the KeyStoreGTKPrivate.
	// This is a workaround for GObject not supporting
	// C++ directly, and I don't want to use glibmm/gtkmm.
	tab->d = new RpKeyStoreGTKPrivate(tab);
}

static void
rp_key_store_gtk_finalize(GObject *object)
{
	RpKeyStoreGTK *const keyStore = RP_KEY_STORE_GTK(object);
	delete keyStore->d;

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_key_store_gtk_parent_class)->finalize(object);
}

RpKeyStoreGTK*
rp_key_store_gtk_new(void)
{
	return static_cast<RpKeyStoreGTK*>(g_object_new(RP_TYPE_KEY_STORE_GTK, nullptr));
}

/** Properties **/

static void
rp_key_store_gtk_get_property(GObject		*object,
			      guint		 prop_id,
			      GValue		*value,
			      GParamSpec	*pspec)
{
	RpKeyStoreGTK *const keyStore = RP_KEY_STORE_GTK(object);

	switch (prop_id) {
		case PROP_TOTAL_KEY_COUNT:
			g_value_set_int(value, keyStore->d->totalKeyCount());
			break;

		case PROP_CHANGED:
			g_value_set_enum(value, keyStore->d->hasChanged());
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_key_store_gtk_set_property(GObject		*object,
			      guint		 prop_id,
			      const GValue	*value,
			      GParamSpec	*pspec)
{
	//sRpKeyStoreGTK *const keyStore = RP_KEY_STORE_GTK(object);
	RP_UNUSED(value);

	// TODO: Handling read-only properties?
	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * Get the total key count.
 * @param keyStore KeyStoreGTK
 * @return Total key count
 */
int
rp_key_store_gtk_get_total_key_count(RpKeyStoreGTK *keyStore)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_GTK(keyStore), 0);
	return keyStore->d->totalKeyCount();
}

/**
 * Has the user changed anything?
 * @param keyStore KeyStoreGTK
 * @return True if anything has been changed; false if not.
 */
gboolean
rp_key_store_gtk_has_changed(RpKeyStoreGTK *keyStore)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_GTK(keyStore), FALSE);
	return keyStore->d->hasChanged();
}

/**
 * Get the underlying KeyStoreUI object.
 * @param keyStore KeyStoreGTK
 * @return KeyStoreUI
 */
LibRomData::KeyStoreUI*
rp_key_store_gtk_get_key_store_ui(RpKeyStoreGTK *keyStore)
{
	g_return_val_if_fail(RP_IS_KEY_STORE_GTK(keyStore), nullptr);
	return keyStore->d;
}
