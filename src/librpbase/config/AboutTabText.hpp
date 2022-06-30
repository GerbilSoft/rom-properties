/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__

#include "common.h"

namespace LibRpBase { namespace AboutTabText {

/**
 * Get the program version.
 * @return Program version
 */
RP_LIBROMDATA_PUBLIC
const char *getProgramVersion(void);

/**
 * Get the git version.
 * @return git version, or nullptr if git was not present.
 */
RP_LIBROMDATA_PUBLIC
const char *getGitVersion(void);

/**
 * Get the git description.
 * @return git description, or nullptr if git was not present.
 */
RP_LIBROMDATA_PUBLIC
const char *getGitDescription(void);

/** Credits **/

enum class CreditType {
	Continue = 0,	// Continue previous type.
	Developer,	// Developer
	Contributor,	// Contributor
	Translator,	// Translator (TODO)

	Max
};

struct CreditsData_t {
	CreditType type;
	const char *name;
	const char *url;
	const char *linkText;
	const char *sub;
};

/**
 * Get the credits data.
 * Ends with CreditType::Max.
 * @return Credits data
 */
RP_LIBROMDATA_PUBLIC
const CreditsData_t *getCreditsData(void);

/** Support **/

struct SupportSite_t {
	const char *name;
	const char *url;
};

/**
 * Get the support sites.
 * Ends with nullptr entries.
 * @return Support sites
 */
RP_LIBROMDATA_PUBLIC
const SupportSite_t *getSupportSites(void);

} }

#endif /* __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__ */
