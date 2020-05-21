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

// C++ STL classes.
using std::unordered_map;

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
		// Achievement types
		enum AchType : uint8_t {
			AT_COUNT = 0,	// Count (requires the same action X number of times)
					// For BOOLEAN achievements, set count to 1.
			AT_BITFIELD,	// Bitfield (multiple actions)
		};

		// Achievement information.
		// Array index is the ID.
		struct AchInfo_t {
			const char *name;	// Name (NOP_C_, translatable)
			const char *desc;	// Description (NOP_C_, translatable)
			AchType type;		// Achievement type
			uint8_t count;		// AT_COUNT: Number of times needed to unlock.
						// AT_BITFIELD: Number of bits. (up to 64)
						//              All bits must be 1 to unlock.
		};
		static const AchInfo_t achInfo[];

		// C++14 adds support for enum classes as unordered_map keys.
		// C++11 needs an explicit hash functor.
		struct EnumClassHash {
			inline std::size_t operator()(Achievements::ID t) const
			{
				return std::hash<int>()(static_cast<int>(t));
			}
		};

		// Active achievement data.
		// Two maps: One for boolean/count, one for bitfield.
		// TODO: Map vs. unordered_map for performance?
		unordered_map<Achievements::ID, uint8_t, EnumClassHash> mapAchData_count;
		unordered_map<Achievements::ID, uint64_t, EnumClassHash> mapAchData_bitfield;
};

/** AchievementsPrivate **/

// Achievement information.
const struct AchievementsPrivate::AchInfo_t AchievementsPrivate::achInfo[] = {
	{
		// tr: ViewedDebugCryptedFile (name)
		NOP_C_("Achievements", "You are now a developer!"),
		// tr: ViewedDebugCryptedFile (description)
		NOP_C_("Achievements", "Viewed a debug-encrypted file."),
		AT_COUNT, 1
	},
	{
		// tr: ViewedNonX86PE (name)
		NOP_C_("Achievements", "Now you're playing with POWER!"),
		// tr: ViewedNonX86PE (description)
		NOP_C_("Achievements", "Viewed a non-x86/x64 Windows PE executable."),
		AT_COUNT, 1
	},
	{
		// tr: ViewedBroadOnWADFile (name)
		NOP_C_("Achievements", "Insert Startup Disc"),
		// tr: ViewedBroadOnWADFile (description)
		NOP_C_("Achievements", "Viewed a BroadOn format Wii WAD file."),
		AT_COUNT, 1
	},
	{
		// tr: ViewedMegaDriveSKwithSK (name)
		NOP_C_("Achievements", "Knuckles & Knuckles"),
		// tr: ViewedMegaDriveSKwithSK (description)
		NOP_C_("Achievements", "Viewed a copy of Sonic & Knuckles locked on to Sonic & Knuckles."),
		AT_COUNT, 1
	},
};

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
Achievements AchievementsPrivate::instance;

AchievementsPrivate::AchievementsPrivate()
	: notifyFunc(nullptr)
	, user_data(0)
{ }

/** Achievements **/

Achievements::Achievements()
	: d_ptr(new AchievementsPrivate())
{
	// TODO: Load achievements.
}

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
 * @param bit Bitfield index for AT_BITFIELD achievements.
 */
void Achievements::unlock(ID id, int bit)
{
	RP_D(Achievements);

	// If this achievement is bool/count, increment the value.
	// If the value has hit the maximum, achievement is unlocked.
	assert((int)id >= 0);
	assert(id < ID::Max);
	if ((int)id < 0 || id >= ID::Max) {
		// Invalid achievement ID.
		return;
	}

	// Check the type.
	const AchievementsPrivate::AchInfo_t *const achInfo = &d->achInfo[(int)id];
	switch (achInfo->type) {
		default:
			assert(!"Achievement type not supported.");
			return;

		case AchievementsPrivate::AT_COUNT: {
			// Check if we've already reached the required count.
			uint8_t count = d->mapAchData_count[id];
			if (count >= achInfo->count) {
				// Count has been reached.
				// Achievement is already unlocked.
				return;
			}

			// Increment the count.
			// TODO: Serialize and save the achievements.
			count++;
			d->mapAchData_count[id] = count;
			if (count < achInfo->count) {
				// Not unlocked yet.
				return;
			}
			break;
		}

		case AchievementsPrivate::AT_BITFIELD: {
			// Bitfield value.
			assert(bit >= 0);
			assert(bit < achInfo->count);
			if (bit < 0 || bit >= achInfo->count) {
				// Invalid bit index.
				return;
			}

			// Check if we've already filled the bitfield.
			// TODO: Verify 32-bit and 64-bit operation for values 32 and 64.
			const uint64_t bf_filled = (1ULL << achInfo->count) - 1;
			uint64_t bf_value = d->mapAchData_bitfield[id];
			if (bf_value == bf_filled) {
				// Bitfield has been filled.
				// Achievement is already unlocked.
				return;
			}

			// Set the bit.
			// TODO: Serialize and save the achievements.
			bf_value |= (1 << (unsigned int)bit);
			d->mapAchData_bitfield[id] = bf_value;
			if (bf_value != bf_filled) {
				// Bitfield is not filled yet.
				return;
			}
			break;
		}
	}

	// Achievement unlocked!
	if (d->notifyFunc) {
		d->notifyFunc(d->user_data,
			dpgettext_expr(RP_I18N_DOMAIN, "Achievements", achInfo->name),
			dpgettext_expr(RP_I18N_DOMAIN, "Achievements", achInfo->desc));
	}
}

}
