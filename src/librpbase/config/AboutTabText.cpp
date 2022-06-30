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

/**
 * Get the program version.
 * @return Program version
 */
const char *getProgramVersion(void)
{
	return RP_VERSION_STRING;
}

/**
 * Get the git version.
 * @return git version, or nullptr if git was not present.
 */
const char *getGitVersion(void)
{
#ifdef RP_GIT_VERSION
	return RP_GIT_VERSION;
#else
	return nullptr;
#endif
}

/**
 * Get the git description.
 * @return git description, or nullptr if git was not present.
 */
const char *getGitDescription(void)
{
#if defined(RP_GIT_VERSION) && defined(RP_GIT_DESCRIBE)
	return RP_GIT_DESCRIBE;
#else
	return nullptr;
#endif
}

/** Credits **/

/**
 * Credits data.
 * Ends with CreditType::Max.
 */
static const CreditsData_t creditsData[] = {
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

/**
 * Get the credits data.
 * Ends with CreditType::Max.
 * @return Credits data
 */
const CreditsData_t *getCreditsData(void)
{
	return creditsData;
}

/** Support **/

/**
 * Support sites.
 * Ends with nullptr.
 */
static const SupportSite_t supportSites[] = {
	{"GitHub: GerbilSoft/rom-properties", "https://github.com/GerbilSoft/rom-properties"},
	{"Sonic Retro", "https://forums.sonicretro.org/index.php?showtopic=35692"},
	{"GBAtemp", "https://gbatemp.net/threads/rom-properties-page-shell-extension.442424/"},

	{nullptr, nullptr}
};

/**
 * Get the support sites.
 * Ends with nullptr entries.
 * @return Support sites
 */
const SupportSite_t *getSupportSites(void)
{
	return supportSites;
}

} }
