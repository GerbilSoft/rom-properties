/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AboutTabText.hpp"
#include "config.version.h"
#include "git.h"

// AboutTabText isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern uint8_t RP_LibRpBase_AboutTabText_ForceLinkage;
	uint8_t RP_LibRpBase_AboutTabText_ForceLinkage;
}

namespace LibRpBase { namespace AboutTabText {

// Program version string.
const char prg_version[] = RP_VERSION_STRING;

// git versions.
#ifdef RP_GIT_VERSION
const char git_version[] = RP_GIT_VERSION;
# ifdef RP_GIT_DESCRIBE
const char git_describe[] = RP_GIT_DESCRIBE;
# else
const char git_describe[] = "";
# endif
#else
const char git_version[] = "";
const char git_describe[] = "";
#endif

/** Credits **/

/**
 * Credits data.
 * Ends with CreditType::Max.
 */
const CreditsData_t CreditsData[] = {
	// Developers
	{CreditType::Developer,		"David Korth", "mailto:gerbilsoft@gerbilsoft.com", "gerbilsoft@gerbilsoft.com", nullptr},
	{CreditType::Continue,		"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", nullptr},

	// Translators
	{CreditType::Translator,	"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", "ru, uk"},
	{CreditType::Continue,		"Null Magic", nullptr, nullptr, "pt_BR"},
	{CreditType::Continue,		"Amnesia1000", nullptr, nullptr, "es"},

	// Contributors
	{CreditType::Contributor,	"CheatFreak47", nullptr, nullptr, nullptr},

	// End of list
	{CreditType::Max, nullptr, nullptr, nullptr, nullptr}
};

/** Support **/

/**
 * Support sites.
 * Ends with nullptr.
 */
const SupportSite_t SupportSites[] = {
	{"GitHub: GerbilSoft/rom-properties", "https://github.com/GerbilSoft/rom-properties"},
	{"Sonic Retro", "https://forums.sonicretro.org/index.php?showtopic=35692"},
	{"GBAtemp", "https://gbatemp.net/threads/rom-properties-page-shell-extension.442424/"},

	{nullptr, nullptr}
};

} }
