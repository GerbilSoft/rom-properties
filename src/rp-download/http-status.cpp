/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * http-status.hpp: HTTP status codes.                                     *
 *                                                                         *
 * Copyright (c) 2020-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "http-status.hpp"

#include "common.h"

// C includes (C++ namespace)
#include <cassert>
#include <cstdlib>

// HTTP status code messages.
// Reference: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
#include "http-status-data.h"

/**
 * Get a string representation for an HTTP status code.
 * @param code HTTP status code.
 * @return String representation, or nullptr if not found.
 */
const TCHAR *http_status_string(int code)
{
	// The table uses uint16_t as keys, so we have to make sure the
	// HTTP statuc code value is in range.
	static constexpr uint16_t HTTP_Status_Code_Max = 598;
	assert(http_status_offtbl[ARRAY_SIZE(http_status_offtbl)-1].code == HTTP_Status_Code_Max);
	if (code > HTTP_Status_Code_Max) {
		// Out of range.
		return nullptr;
	}

	// Do a binary search.
	const HttpStatusMsg_t key = {static_cast<uint16_t>(code), 0};
	void *ptr = bsearch(&key, http_status_offtbl,
		ARRAY_SIZE(http_status_offtbl), sizeof(http_status_offtbl[0]),
		[](const void *a, const void *b)
		{
			const HttpStatusMsg_t *const pa = static_cast<const HttpStatusMsg_t*>(a);
			const HttpStatusMsg_t *const pb = static_cast<const HttpStatusMsg_t*>(b);
			return static_cast<int>(pa->code) - static_cast<int>(pb->code);
		});
	if (!ptr) {
		return nullptr;
	}

	const HttpStatusMsg_t *const pHttp = static_cast<const HttpStatusMsg_t*>(ptr);
	return &http_status_strtbl[pHttp->offset];
}
