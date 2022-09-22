/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * http-status.hpp: HTTP status codes.                                     *
 *                                                                         *
 * Copyright (c) 2020-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_DOWNLOAD_HTTP_STATUS_HPP__
#define __ROMPROPERTIES_RP_DOWNLOAD_HTTP_STATUS_HPP__

#include "tcharx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a string representation for an HTTP status code.
 * @param code HTTP status code.
 * @return String representation, or nullptr if not found.
 */
const TCHAR *http_status_string(int code);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_RP_DOWNLOAD_HTTP_STATUS_HPP__ */
