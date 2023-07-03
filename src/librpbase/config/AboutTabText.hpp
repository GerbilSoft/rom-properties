/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AboutTabText.hpp: About tab for rp-config. (Common text)                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

namespace LibRpBase { namespace AboutTabText {

/** Program version **/

// Program version access macros
// Use with getProgramVersion()
#define RP_PROGRAM_VERSION_MAJOR(ver)		(unsigned int)( ((ver) >> 48U))
#define RP_PROGRAM_VERSION_MINOR(ver)		(unsigned int)((((ver) >> 32U) & 0xFFFFU))
#define RP_PROGRAM_VERSION_REVISION(ver)	(unsigned int)((((ver) >> 16U) & 0xFFFFU))
#define RP_PROGRAM_VERSION_DEVEL(ver)		(unsigned int)(( (ver)         & 0xFFFFU))

#define RP_PROGRAM_VERSION_NO_DEVEL(ver)	( (ver)         & 0xFFFFFFFFFFFF0000ULL)

// Program version creation macro
#define RP_PROGRAM_VERSION(major, minor, revision, devel) \
	( ((((uint64_t)(major))    & 0xFFFFU) << 48U) | \
	  ((((uint64_t)(minor))    & 0xFFFFU) << 32U) | \
	  ((((uint64_t)(revision)) & 0xFFFFU) << 16U) | \
	   (((uint64_t)(devel))    & 0xFFFFU) )

/**
 * Get the program version as a 64-bit unsigned int.
 * Format: [major][minor][revision][devel]
 * @return Program version
 */
RP_LIBROMDATA_PUBLIC
uint64_t getProgramVersion(void);

/** Program information strings **/

enum class ProgramInfoStringID {
	ProgramName = 0,
	ProgramFullName,
	Copyright,
	ProgramVersion,
	GitVersion,
	GitDescription,

	UpdateVersionUrl,
	UpdateVersionCacheKey,

	Max	// NOTE: May change in future releases.
};

/**
 * Get a program information string.
 * @param id String ID
 * @return String, or nullptr if not available.
 */
RP_LIBROMDATA_PUBLIC
const char *getProgramInfoString(ProgramInfoStringID id);

/**
 * Get the program information string count.
 * @return Highest program information string ID.
 */
RP_LIBROMDATA_PUBLIC
ProgramInfoStringID getProgramInfoStringCount(void);

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
