/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__

#include "../common.h"

namespace LibRpBase {

class AboutTabText {
	private:
		// Static class.
		AboutTabText();
		~AboutTabText();
		RP_DISABLE_COPY(AboutTabText);

	public:
		// Program version string.
		static const char prg_version[];

		// git version, or empty string if git was not present.
		static const char git_version[];
		// git description, or empty string if git was not present.
		static const char git_describe[];

	public:
		/** Credits **/

		enum CreditType_t {
			CT_CONTINUE = 0,	// Continue previous type.
			CT_DEVELOPER,		// Developer
			CT_CONTRIBUTOR,		// Contributor
			CT_TRANSLATOR,		// Translator (TODO)

			CT_MAX
		};

		struct CreditsData_t {
			CreditType_t type;
			const char *name;
			const char *url;
			const char *linkText;
			const char *sub;
		};

		/**
		 * Credits data.
		 * Ends with CT_MAX.
		 */
		static const CreditsData_t CreditsData[];

		/** Support **/

		struct SupportSite_t {
			const char *name;
			const char *url;
		};

		/**
		 * Support sites.
		 * Ends with nullptr.
		 */
		static const SupportSite_t SupportSites[];
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__ */
