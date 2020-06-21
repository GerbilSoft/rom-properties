/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
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
 * Ends with CreditType::Max.
 */
const AboutTabText::CreditsData_t AboutTabText::CreditsData[] = {
	// Developers
	{CreditType::Developer,		"David Korth", "mailto:gerbilsoft@gerbilsoft.com", "gerbilsoft@gerbilsoft.com", nullptr},
	{CreditType::Continue,		"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", nullptr},

	// Translators
	{CreditType::Translator,	"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", "ru, uk"},
	{CreditType::Continue,		"Null Magic", nullptr, nullptr, "pt_BR"},

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
const AboutTabText::SupportSite_t AboutTabText::SupportSites[] = {
	{"GitHub: GerbilSoft/rom-properties", "https://github.com/GerbilSoft/rom-properties"},
	{"Sonic Retro", "https://forums.sonicretro.org/index.php?showtopic=35692"},
	{"GBAtemp", "https://gbatemp.net/threads/rom-properties-page-shell-extension.442424/"},

	{nullptr, nullptr}
};

}
