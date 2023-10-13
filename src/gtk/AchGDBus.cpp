/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchGDBus.cpp: GDBus notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchGDBus.hpp"

// librpbase
#include "librpbase/Achievements.hpp"
using LibRpBase::Achievements;

// GDBus
#include <glib-object.h>
#include "Notifications.h"

// Achievement spritesheets
#include "AchSpriteSheet.hpp"

// C++ STL classes.
using std::string;
using std::unordered_map;

class AchGDBusPrivate
{
	public:
		AchGDBusPrivate();
		~AchGDBusPrivate();

	private:
		RP_DISABLE_COPY(AchGDBusPrivate)

	public:
		// Static AchGDBus instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static AchGDBus instance;
		bool hasRegistered;

	public:
		/**
		 * Notification function. (static)
		 * @param user_data	[in] User data. (this)
		 * @param id		[in] Achievement ID.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int RP_C_API notifyFunc(intptr_t user_data, Achievements::ID id);

		/**
		 * Notification function. (non-static)
		 * @param id	[in] Achievement ID.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int notifyFunc(Achievements::ID id);
};

/** AchGDBusPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
AchGDBus AchGDBusPrivate::instance;

AchGDBusPrivate::AchGDBusPrivate()
	: hasRegistered(false)
{
	// NOTE: Cannot register here because the static Achievements
	// instance might not be fully initialized yet.
}

AchGDBusPrivate::~AchGDBusPrivate()
{
	if (hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->clearNotifyFunction(notifyFunc, reinterpret_cast<intptr_t>(this));
	}
}

/**
 * Notification function. (static)
 * @param user_data	[in] User data. (this)
 * @param id		[in] Achievement ID.
 * @return 0 on success; negative POSIX error code on error.
 */
int RP_C_API AchGDBusPrivate::notifyFunc(intptr_t user_data, Achievements::ID id)
{
	AchGDBusPrivate *const pAchGP = reinterpret_cast<AchGDBusPrivate*>(user_data);
	return pAchGP->notifyFunc(id);
}

/**
 * Notification function. (non-static)
 * @param id	[in] Achievement ID.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchGDBusPrivate::notifyFunc(Achievements::ID id)
{
	assert((int)id >= 0);
	assert(id < Achievements::ID::Max);
	if ((int)id < 0 || id >= Achievements::ID::Max) {
		// Invalid achievement ID.
		return -EINVAL;
	}

	// Connect to the service using gdbus-codegen's generated code.
	Notifications *proxy = nullptr;
	GError *error = nullptr;

	proxy = notifications_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		"org.freedesktop.Notifications",	// bus name
		"/org/freedesktop/Notifications",	// object path
		nullptr,				// GCancellable
		&error);				// GError
	if (!proxy) {
		// Unable to connect.
		g_error_free(error);
		return -EIO;
	}

	Achievements *const pAch = Achievements::instance();

	// Build the text.
	// TODO: Better formatting?
	char *const s_achName = g_markup_escape_text(pAch->getName(id), -1);
	char *const s_achDescUnlocked = g_markup_escape_text(pAch->getDescUnlocked(id), -1);
	string text = "<u>";
	text += s_achName;
	text += "</u>\n";
	text += s_achDescUnlocked;
	g_free(s_achName);
	g_free(s_achDescUnlocked);

	// actions: as
	static const gchar *const actions[] = { nullptr };

	// hints: a{sv}
	// NOTE: GDBus seems to take ownership of `hints`...
	GVariantBuilder b_hints;
	g_variant_builder_init(&b_hints, G_VARIANT_TYPE("a{sv}"));

	// Get the icon.
	// FIXME: Icon size. Using 32px for now.
	static const gint iconSize = 32;
	AchSpriteSheet achSpritesheet(iconSize);
	PIMGTYPE icon = achSpritesheet.getIcon(id);
	assert(icon != nullptr);
	if (!icon) {
		// Unable to get the icon.
		return -EIO;
	}

#ifdef RP_GTK_USE_CAIRO
	// NOTE: The R and B channels need to be swapped for XDG notifications.
	// Cairo: Swap the R and B channels in place.
	// TODO: SSSE3-optimized version?
	using LibRpTexture::argb32_t;

	int width, height;
	PIMGTYPE_get_size(icon, &width, &height);
	argb32_t *bits = reinterpret_cast<argb32_t*>(PIMGTYPE_get_image_data(icon));
	const int strideDiff = (PIMGTYPE_get_rowstride(icon) / sizeof(argb32_t)) - width;
	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		unsigned int x;
		for (x = (unsigned int)width; x > 1; x -= 2) {
			// Swap the R and B channels
			std::swap(bits[0].r, bits[0].b);
			std::swap(bits[1].r, bits[1].b);
			bits += 2;
		}
		if (x == 1) {
			// Last pixel
			std::swap(bits->r, bits->b);
			bits++;
		}

		// Next line.
		bits += strideDiff;
	}
	PIMGTYPE_mark_dirty(icon);
#endif /* RP_GTK_USE_CAIRO */

	size_t imgDataLen = 0;
	const uint8_t *const pImgData = PIMGTYPE_get_image_data(icon, &imgDataLen);

	GVariantBuilder b_image_data;
	g_variant_builder_init(&b_image_data, G_VARIANT_TYPE("(iiibiiay)"));
	g_variant_builder_add(&b_image_data, "i", iconSize);	// width
	g_variant_builder_add(&b_image_data, "i", iconSize);	// height
	g_variant_builder_add(&b_image_data, "i", PIMGTYPE_get_rowstride(icon));
	g_variant_builder_add(&b_image_data, "b", TRUE);	// has_alpha
	g_variant_builder_add(&b_image_data, "i", 8);		// 8 bits per *channel*
	g_variant_builder_add(&b_image_data, "i", 4);		// channels

	// FIXME: g_variant_builder_add() fails with "ay".
	//g_variant_builder_add(&b_image_data, "ay", pImgData, imgDataLen);
	GVariant *var = g_variant_new_from_data(G_VARIANT_TYPE("ay"),
		pImgData, imgDataLen, TRUE, nullptr, nullptr);
	g_variant_builder_add_value(&b_image_data, var);

	GVariant *const image_data = g_variant_builder_end(&b_image_data);

	// NOTE: The hint name changed in various versions of the specification.
	// We'll use the oldest version for compatibility purposes.
	// - 1.0: "icon_data"
	// - 1.1: "image_data"
	// - 1.2: "image-data"
	g_variant_builder_add(&b_hints, "{sv}", "icon_data", image_data);

	GVariant *const hints = g_variant_builder_end(&b_hints);

	const char *const s_summary = C_("Achievements", "Achievement Unlocked");
	notifications_call_notify(
		proxy,			// proxy
		"rom-properties",	// app-name [s]
		0,			// replaces_id [u]
		"",			// app_icon [s]
		s_summary,		// summary [s]
		text.c_str(),		// body [s]
		actions,		// actions [as]
		hints,			// hints [a{sv}]
		5000,			// timeout (ms) [i]
		nullptr,		// GCancellable
		nullptr,		// GAsyncReadyCallback
		nullptr);		// user_data

	// NOTE: Not waiting for a response.
	if (icon) {
		PIMGTYPE_unref(icon);
	}
	g_object_unref(proxy);
	return 0;
}

/** AchGDBus **/

AchGDBus::AchGDBus()
	: d_ptr(new AchGDBusPrivate())
{ }

AchGDBus::~AchGDBus()
{
	delete d_ptr;
}

/**
 * Get the AchGDBus instance.
 *
 * This automatically initializes librpbase's Achievement
 * object and reloads the achievements data if it has been
 * modified.
 *
 * @return AchGDBus instance.
 */
AchGDBus *AchGDBus::instance(void)
{
	AchGDBus *const q = &AchGDBusPrivate::instance;

	// NOTE: Cannot register in the private class constructor because
	// the Achievements instance might not be fully initialized yet.
	// Registering here instead.
	AchGDBusPrivate *const d = q->d_ptr;
	if (!d->hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->setNotifyFunction(AchGDBusPrivate::notifyFunc, reinterpret_cast<intptr_t>(d));
		d->hasRegistered = true;
	}

	return q;
}
