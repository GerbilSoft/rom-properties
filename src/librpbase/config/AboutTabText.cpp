/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "AboutTabText.hpp"
#include "config.version.h"
#include "git.h"

namespace LibRpBase {

// Program version string.
const char AboutTabText::prg_version[] = RP_VERSION_STRING;

// git versions.
#ifdef RP_GIT_VERSION
const char AboutTabText::git_version[] = RP_GIT_VERSION;
# ifdef RP_GIT_DESCRIBE
const char AboutTabText::git_describe[] = RP_GIT_DESCRIBE;
# else
const char AboutTabText::git_describe[] = "";
# endif
#else
const char AboutTabText::git_version[] = "";
const char AboutTabText::git_describe[] = "";
#endif

/** Credits **/

/**
 * Credits data.
 * Ends with CT_MAX.
 */
const AboutTabText::CreditsData_t AboutTabText::CreditsData[] = {
	// Developers
	{CT_DEVELOPER,		"David Korth", "mailto:gerbilsoft@gerbilsoft.com", "gerbilsoft@gerbilsoft.com", nullptr},
	{CT_CONTINUE,		"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", nullptr},

	// Translators
	{CT_TRANSLATOR,		"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", "ru, uk"},
	{CT_CONTINUE,		"Null Magic", nullptr, nullptr, "pt_BR"},

	// Contributors
	{CT_CONTRIBUTOR,	"CheatFreak47", nullptr, nullptr, nullptr},

	// End of list
	{CT_MAX, nullptr, nullptr, nullptr, nullptr}
};

/** Support **/

/**
 * Support sites.
 * Ends with nullptr.
 */
const AboutTabText::SupportSite_t AboutTabText::SupportSites[] = {
	{"GitHub: GerbilSoft/rom-properties", "https://github.com/GerbilSoft/rom-properties"},
	{"Sonic Retro", "https://forums.sonicretro.org/index.php?showtopic=35692"},
	{"GBAtemp", "https://gbatemp.net/threads/rom-properties-page-shell-extension.442424/"},

	{nullptr, nullptr}
};

}
