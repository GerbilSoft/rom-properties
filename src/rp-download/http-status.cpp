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

// C++ STL classes
#include <algorithm>

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
	// HTTP status code value is in range.
	static constexpr uint16_t HTTP_Status_Code_Max = 598;
	assert(http_status_offtbl[ARRAY_SIZE(http_status_offtbl)-1].code == HTTP_Status_Code_Max);
	if (code > HTTP_Status_Code_Max) {
		// Out of range.
		return nullptr;
	}

	// Do a binary search.
	static const HttpStatusMsg_t *const p_http_status_offtbl_end =
		&http_status_offtbl[ARRAY_SIZE(http_status_offtbl)];
	auto iter = std::lower_bound(http_status_offtbl, p_http_status_offtbl_end, code,
		[](HttpStatusMsg_t msg, int code) noexcept -> bool {
			return (msg.code < code);
		});
	if (iter == p_http_status_offtbl_end || iter->code != code) {
		// Not found.
		return nullptr;
	}

	return &http_status_strtbl[iter->offset];
}
