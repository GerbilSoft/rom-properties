/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * html_entities.hpp: HTML entities handling.                              *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes
// NOTE: gcc-5 on Ubuntu 16.04 is missing <cuchar>...
#include <uchar.h>	// for char16_t

namespace LibRpText { namespace HtmlEntities {

/**
 * Parse an HTML entity.
 * @param entity Pointer to HTML tag (will be modified) (MUST be pointing to a NULL-terminated string!)
 * @return Parsed HTML entity (as a UTF-16 code point)
 */
RP_LIBROMDATA_PUBLIC
char16_t parseHtmlEntity(const char *&entity);

#ifdef _WIN32
/**
 * Parse an HTML entity.
 * @param entity Pointer to HTML tag (will be modified) (MUST be pointing to a NULL-terminated string!)
 * @return Parsed HTML entity (as a UTF-16 code point)
 */
RP_LIBROMDATA_PUBLIC
char16_t parseHtmlEntity(const wchar_t *&entity);
#endif /* _WIN32 */

}} // namespace LibRpText::HtmlEntities
