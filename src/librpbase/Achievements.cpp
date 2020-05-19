/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Achievements.cpp: Achievements class.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Achievements.hpp"
#include "libi18n/i18n.h"

namespace LibRpBase {

class AchievementsPrivate
{
	public:
		AchievementsPrivate();

	private:
		RP_DISABLE_COPY(AchievementsPrivate)

	public:
		// Static Achievements instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static Achievements instance;

	public:
		// Notification function.
		Achievements::NotifyFunc notifyFunc;
		intptr_t user_data;

	public:
		/**
		 * Get the translated description of an achievement.
		 * @param id Achievement ID.
		 * @return Translated description, or nullptr on error.
		 */
		static const char *description(Achievements::ID id);
};

/** AchievementsPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
Achievements AchievementsPrivate::instance;

AchievementsPrivate::AchievementsPrivate()
	: notifyFunc(nullptr)
	, user_data(0)
{ }

/**
 * Get the translated description of an achievement.
 * @param id Achievement ID.
 * @return Translated description, or nullptr on error.
 */
const char *AchievementsPrivate::description(Achievements::ID id)
{
	static const char *const ach_desc[] = {
		// tr: ViewedDebugCryptedFile
		NOP_C_("Achievements", "Viewed a debug-encrypted file."),
		NOP_C_("Achievements", "Viewed a non-x86/x64 Windows PE executable."),
		NOP_C_("Achievements", "Viewed a BroadOn format Wii WAD file."),
	};

	assert((int)id >= 0);
	assert((int)id < ARRAY_SIZE(ach_desc));
	if ((int)id < 0 || (int)id > ARRAY_SIZE(ach_desc)) {
		// Out of range.
		return nullptr;
	}

	return dpgettext_expr(RP_I18N_DOMAIN, "Achievements", ach_desc[(int)id]);
}

/** Achievements **/

Achievements::Achievements()
	: d_ptr(new AchievementsPrivate())
{ }

Achievements::~Achievements()
{
	delete d_ptr;
}

/**
 * Get the Achievements instance.
 *
 * This automatically initializes the object and
 * reloads the achievements data if it has been modified.
 *
 * @return Achievements instance.
 */
Achievements *Achievements::instance(void)
{
	// Initialize the singleton instance.
	Achievements *const q = &AchievementsPrivate::instance;

	// TODO: Load achievements.
	RP_UNUSED(q);

	// Return the singleton instance.
	return q;
}

/**
 * Set the notification function.
 * This is used for the UI frontends.
 * @param func Notification function.
 * @param user_data User data.
 */
void Achievements::setNotifyFunction(NotifyFunc func, intptr_t user_data)
{
	RP_D(Achievements);
	d->notifyFunc = func;
	d->user_data = user_data;
}

/**
 * Unregister a notification function if set.
 *
 * If both func and user_data match the existing values,
 * then both are cleared.
 *
 * @param func Notification function.
 * @param user_data User data.
 */
void Achievements::clearNotifyFunction(NotifyFunc func, intptr_t user_data)
{
	RP_D(Achievements);
	if (d->notifyFunc == func && d->user_data == user_data) {
		d->notifyFunc = nullptr;
		d->user_data = 0;
	}
}

/**
 * Unlock an achievement.
 * @param id Achievement ID.
 */
void Achievements::unlock(ID id)
{
	RP_D(const Achievements);
	if (d->notifyFunc) {
		const char *const desc = d->description(id);
		if (desc) {
			d->notifyFunc(d->user_data, desc);
		}
	}
}

}
