/***************************************************************************
 * ROM Properties Page shell extension. (rp-stub)                          *
 * rp-stub_secure.h: Security options for rp-stub.                         *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_STUB_RP_STUB_SECURE_H__
#define __ROMPROPERTIES_RP_STUB_RP_STUB_SECURE_H__

#include "stdboolx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable security options.
 * @param config True for rp-config; false for thumbnailing.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_stub_do_security_options(bool config);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_RP_STUB_RP_STUB_SECURE_H__ */
