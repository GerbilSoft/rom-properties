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

// Program version string.
extern const char prg_version[];

// git version, or empty string if git was not present.
extern const char git_version[];
// git description, or empty string if git was not present.
extern const char git_describe[];

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
 * Credits data.
 * Ends with CT_MAX.
 */
extern const CreditsData_t CreditsData[];

/** Support **/

struct SupportSite_t {
	const char *name;
	const char *url;
};

/**
 * Support sites.
 * Ends with nullptr.
 */
extern const SupportSite_t SupportSites[];

} }

#endif /* __ROMPROPERTIES_LIBRPBASE_CONFIG_ABOUTTABTEXT_HPP__ */
