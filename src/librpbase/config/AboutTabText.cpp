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

// FIXME: Change to rp_char[]. (MSVC 2010 issue)

// Program version string.
const rp_char *const AboutTabText::prg_version = _RP(RP_VERSION_STRING);

// git versions.
#ifdef RP_GIT_VERSION
const rp_char *const AboutTabText::git_version = _RP(RP_GIT_VERSION);
# ifdef RP_GIT_DESCRIBE
const rp_char *const AboutTabText::git_describe = _RP(RP_GIT_DESCRIBE);
# else
const rp_char *const AboutTabText::git_describe = nullptr;
# endif
#else
const rp_char *const AboutTabText::git_version = nullptr;
const rp_char *const AboutTabText::git_describe = nullptr;
#endif

/** Credits. **/

// Credits data.
const AboutTabText::CreditsData_t AboutTabText::CreditsData[] = {
	// Developers
	{CT_DEVELOPER,		_RP("David Korth"), _RP("mailto:gerbilsoft@gerbilsoft.com"), _RP("gerbilsoft@gerbilsoft.com"), nullptr},
	{CT_CONTINUE,		_RP("Egor"), _RP("mailto:egor@opensrc.club"), _RP("egor@opensrc.club"), nullptr},

	// Contributors
	{CT_CONTRIBUTOR,	_RP("CheatFreak47"), nullptr, nullptr, nullptr},

	// End of list
	{CT_MAX, nullptr, nullptr, nullptr, nullptr}
};

}
