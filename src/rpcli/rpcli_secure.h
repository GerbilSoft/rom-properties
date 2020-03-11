/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * rpcli_secure.h: Security options for rpcli.                             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_RPCLI_SECURE_H__
#define __ROMPROPERTIES_RPCLI_RPCLI_SECURE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable security options.
 * @return 0 on success; negative POSIX error code on error.
 */
int rpcli_do_security_options(void);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_RPCLI_RPCLI_SECURE_H__ */
