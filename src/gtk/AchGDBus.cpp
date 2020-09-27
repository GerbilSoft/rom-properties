/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchGDBus.cpp: GDBus notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchGDBus.hpp"
using LibRpBase::Achievements;

// GDBus
#include <glib-object.h>
#include "Notifications.h"

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
		 * Load the specified sprite sheet.
		 * @param iconSize Icon size. (16, 24, 32, 64)
		 * @return PIMGTYPE, or nullptr on error.
		 */
		PIMGTYPE loadSpriteSheet(int iconSize);

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

	public:
		// Sprite sheets.
		// - Key: Icon size
		// - Value: PIMGTYPE
		unordered_map<int, PIMGTYPE> map_imgAchSheet;
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

	// Delete the achievements sprite sheets.
	std::for_each(map_imgAchSheet.begin(), map_imgAchSheet.end(),
		[](std::pair<int, PIMGTYPE> pair) {
			PIMGTYPE_destroy(pair.second);
		}
	);
}

/**
 * Load the specified sprite sheet.
 * @param iconSize Icon size. (16, 24, 32, 64)
 * @return PIMGTYPE, or nullptr on error.
 */
PIMGTYPE AchGDBusPrivate::loadSpriteSheet(int iconSize)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32 || iconSize == 64);

	// Check if the sprite sheet is already loaded.
	auto iter = map_imgAchSheet.find(iconSize);
	if (iter != map_imgAchSheet.end()) {
		// Sprite sheet is already loaded.
		return iter->second;
	}

	char ach_filename[64];
	snprintf(ach_filename, sizeof(ach_filename),
		"/com/gerbilsoft/rom-properties/ach/ach-%dx%d.png",
		iconSize, iconSize);
	PIMGTYPE imgAchSheet = PIMGTYPE_load_png_from_gresource(ach_filename);
	assert(imgAchSheet != nullptr);
	if (!imgAchSheet) {
		// Unable to load the achievements sprite sheet.
		return nullptr;
	}

	// Make sure the bitmap has the expected size.
	assert(PIMGTYPE_size_check(imgAchSheet,
		(int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS),
		(int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS)));
	if (!PIMGTYPE_size_check(imgAchSheet,
	     (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS),
	     (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS)))
	{
		// Incorrect size. We can't use it.
		PIMGTYPE_destroy(imgAchSheet);
		return nullptr;
	}

#ifdef RP_GTK_USE_CAIRO
	// Cairo: Swap the R and B channels in place.
	int width, height;
	PIMGTYPE_get_size(imgAchSheet, &width, &height);
	uint8_t *bits = PIMGTYPE_get_image_data(imgAchSheet);
	int strideDiff = PIMGTYPE_get_rowstride(imgAchSheet) - (width * sizeof(uint32_t));
	for (int y = height; y > 0; y--) {
		for (int x = width; x > 0; x--, bits += 4) {
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			std::swap(bits[0], bits[2]);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			std::swap(bits[1], bits[3]);
#endif
		}
		bits += strideDiff;
	}
#endif

	// Sprite sheet is correct.
	map_imgAchSheet.emplace(std::make_pair(iconSize, imgAchSheet));
	return imgAchSheet;
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
	OrgFreedesktopNotifications *proxy = nullptr;
	GError *error = nullptr;

	proxy = org_freedesktop_notifications_proxy_new_for_bus_sync(
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
	string text = "<u>";
	text += pAch->getName(id);
	text += "</u>\n";
	text += pAch->getDescUnlocked(id);

	// actions: as
	static const gchar *const actions[] = { "", nullptr };

	// hints: a{sv}
	// NOTE: GDBus seems to take ownership of `hints`...
	GVariantBuilder b_hints;
	g_variant_builder_init(&b_hints, G_VARIANT_TYPE("a{sv}"));

	// Get the icon.
	// FIXME: Icon size. Using 32px for now.
	static const int iconSize = 32;
	PIMGTYPE imgspr = loadSpriteSheet(iconSize);
	PIMGTYPE subIcon = nullptr;
	if (imgspr != nullptr) {
		// Determine row and column.
		const int col = ((int)id % Achievements::ACH_SPRITE_SHEET_COLS);
		const int row = ((int)id / Achievements::ACH_SPRITE_SHEET_COLS);

		// Extract the sub-icon.
		subIcon = PIMGTYPE_get_subsurface(imgspr, col*iconSize, row*iconSize, iconSize, iconSize);
		assert(subIcon != nullptr);

		int width, height;
		PIMGTYPE_get_size(subIcon, &width, &height);
		assert(width > 0);
		assert(height > 0);

		size_t imgDataLen = 0;
		const uint8_t *const pImgData = PIMGTYPE_get_image_data(subIcon, &imgDataLen);

		GVariantBuilder b_image_data;
		g_variant_builder_init(&b_image_data, G_VARIANT_TYPE("(iiibiiay)"));
		g_variant_builder_add(&b_image_data, "i", width);
		g_variant_builder_add(&b_image_data, "i", height);
		g_variant_builder_add(&b_image_data, "i", PIMGTYPE_get_rowstride(subIcon));
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
	}

	GVariant *const hints = g_variant_builder_end(&b_hints);

	const char *const s_summary = C_("Achievements", "Achievement Unlocked");
	org_freedesktop_notifications_call_notify(
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
	if (subIcon) {
		PIMGTYPE_destroy(subIcon);
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
