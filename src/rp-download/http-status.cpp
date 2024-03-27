/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * http-status.hpp: HTTP status codes.                                     *
 *                                                                         *
 * Copyright (c) 2020-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "http-status.hpp"

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
	// Do a binary search.
	static const HttpStatusMsg_t *const p_http_status_offtbl =
		&http_status_offtbl[ARRAY_SIZE(http_status_offtbl)];
	auto pHttp = std::lower_bound(http_status_offtbl, p_http_status_offtbl, code,
		[](const HttpStatusMsg_t &msg, int code) noexcept -> bool {
			return (msg.code < code);
		});
	if (pHttp == p_http_status_offtbl || pHttp->code != code) {
		return nullptr;
	}

	return &http_status_strtbl[pHttp->offset];
}
