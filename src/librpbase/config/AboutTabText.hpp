/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__

#include "common.h"

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
