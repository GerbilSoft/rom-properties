/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AboutTabText.hpp"
#include "config.version.h"
#include "git.h"

// C++ STL classes
using std::array;

// AboutTabText isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpBase_AboutTabText_ForceLinkage;
	unsigned char RP_LibRpBase_AboutTabText_ForceLinkage;
}

namespace LibRpBase { namespace AboutTabText {

/** Program version **/

/**
 * Get the program version as a 64-bit unsigned int.
 * Format: [major][minor][revision][devel]
 * @return Program version
 */
RP_LIBROMDATA_PUBLIC
uint64_t getProgramVersion(void)
{
	return RP_PROGRAM_VERSION(
		RP_VERSION_MAJOR, RP_VERSION_MINOR,
		RP_VERSION_PATCH, RP_VERSION_DEVEL);
}

/** Program information strings **/

static const array<const char*, (int)ProgramInfoStringID::Max> ProgramInfoString_tbl = {{
	"rom-properties",				// ProgramName
	"ROM Properties Page Shell Extension",		// ProgramFullName
	"Copyright (c) 2016-2025 by David Korth.",	// Copyright
	RP_VERSION_STRING,

	// GitVersion
#ifdef RP_GIT_VERSION
	RP_GIT_VERSION,
#else /* !RP_GIT_VERSION */
	nullptr,
#endif /* RP_GIT_VERSION */

	// GitDescription
#if defined(RP_GIT_VERSION) && defined(RP_GIT_DESCRIBE)
	RP_GIT_DESCRIBE,
#else /* !(RP_GIT_VERSION && RP_GIT_DESCRIBE) */
	nullptr,
#endif /* RP_GIT_VERSION && RP_GIT_DESCRIBE */

	// UpdateVersionUrl
	"https://rpdb.gerbilsoft.com/sys/version.txt",
	// UpdateVersionCacheKey
	"sys/version.txt",
}};

/**
 * Get a program information string.
 * @param id String ID
 * @return String, or nullptr if not available.
 */
RP_LIBROMDATA_PUBLIC
const char *getProgramInfoString(ProgramInfoStringID id)
{
	assert(static_cast<int>(id) >= 0);
	assert(id < ProgramInfoStringID::Max);
	if (static_cast<int>(id) < 0 || id >= ProgramInfoStringID::Max) {
		return nullptr;
	}

	return ProgramInfoString_tbl[static_cast<int>(id)];
}

/**
 * Get the program information string count.
 * @return Highest program information string ID.
 */
RP_LIBROMDATA_PUBLIC
ProgramInfoStringID getProgramInfoStringCount(void)
{
	return ProgramInfoStringID::Max;
}

/** Credits **/

/**
 * Credits data.
 * Ends with CreditType::Max.
 */
static const array<CreditsData_t, 19+1> creditsData = {{
	// Developers
	{CreditType::Developer,		"David Korth", "mailto:gerbilsoft@gerbilsoft.com", "gerbilsoft@gerbilsoft.com", nullptr},
	{CreditType::Continue,		"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", nullptr},

	// Translators
	{CreditType::Translator,	"Egor", "mailto:egor@opensrc.club", "egor@opensrc.club", "ru, uk"},
	{CreditType::Continue,		"Null Magic", nullptr, nullptr, "pt_BR"},
	{CreditType::Continue,		"Amnesia1000", nullptr, nullptr, "es"},
	{CreditType::Continue,		"Slippy", nullptr, nullptr, "de"},
	{CreditType::Continue,		"CyberYoshi64", nullptr, nullptr, "de"},
	{CreditType::Continue,		"maschell", nullptr, nullptr, "de"},
	{CreditType::Continue,		"WebSnke", nullptr, nullptr, "de"},
	{CreditType::Continue,		"TheOneGoofAli", nullptr, nullptr, "ru"},
	{CreditType::Continue,		"NotaInutilis", nullptr, nullptr, "fr"},
	{CreditType::Continue,		"xxmichibxx", nullptr, nullptr, "de"},
	{CreditType::Continue,		"ThePBone", nullptr, nullptr, "de"},
	{CreditType::Continue,		"ionuttbara", nullptr, nullptr, "ro"},
	{CreditType::Continue,		"MaRod92", nullptr, nullptr, "it"},
	{CreditType::Continue,		"Motwera", nullptr, nullptr, "ar"},
	{CreditType::Continue,		"Chipsum", nullptr, nullptr, "ar"},
	{CreditType::Continue,		"spencerchris8080", nullptr, nullptr, "es"},

	// Contributors
	{CreditType::Contributor,	"CheatFreak47", nullptr, nullptr, nullptr},

	// End of list
	{CreditType::Max, nullptr, nullptr, nullptr, nullptr}
}};

/**
 * Get the credits data.
 * Ends with CreditType::Max.
 * @return Credits data
 */
const CreditsData_t *getCreditsData(void)
{
	return creditsData.data();
}

/** Support **/

/**
 * Support sites.
 * Ends with nullptr.
 */
static const array<SupportSite_t, 3+1> supportSites = {{
	{"GitHub: GerbilSoft/rom-properties", "https://github.com/GerbilSoft/rom-properties"},
	{"Sonic Retro", "https://forums.sonicretro.org/index.php?showtopic=35692"},

	{nullptr, nullptr}
}};

/**
 * Get the support sites.
 * Ends with nullptr entries.
 * @return Support sites
 */
const SupportSite_t *getSupportSites(void)
{
	return supportSites.data();
}

} }

