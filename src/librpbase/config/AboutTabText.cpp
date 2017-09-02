/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "AboutTabText.hpp"
#include "config.version.h"
#include "git.h"

namespace LibRpBase {

// NOTE: Can't use rp_char here due to MSVC 2010 issues.
// These strings will have to be converted to rp_char at runtime.
// These are UTF-8 strings.

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
	{CT_DEVELOPER,		_RP("David Korth"), _RP("mailto:gerbilsoft@gerbilsoft.com"), _RP("gerbilsoft@gerbilsoft.com"), nullptr},
	{CT_CONTINUE,		_RP("Egor"), _RP("mailto:egor@opensrc.club"), _RP("egor@opensrc.club"), nullptr},

	// Contributors
	{CT_CONTRIBUTOR,	_RP("CheatFreak47"), nullptr, nullptr, nullptr},

	// End of list
	{CT_MAX, nullptr, nullptr, nullptr, nullptr}
};

/** Support **/

/**
 * Support sites.
 * Ends with nullptr.
 */
const AboutTabText::SupportSite_t AboutTabText::SupportSites[] = {
	{_RP("GitHub: GerbilSoft/rom-properties"), _RP("https://github.com/GerbilSoft/rom-properties")},
	{_RP("Sonic Retro"), _RP("https://forums.sonicretro.org/index.php?showtopic=35692")},
	{_RP("GBAtemp"), _RP("https://gbatemp.net/threads/rom-properties-page-shell-extension.442424/")},

	{nullptr, nullptr}
};

}
