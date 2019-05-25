/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * verifykeys.hpp: Verify encryption keys.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_VERIFYKEYS_HPP__
#define __ROMPROPERTIES_RPCLI_VERIFYKEYS_HPP__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Verify encryption keys.
 * @return 0 on success; non-zero on error.
 */
int VerifyKeys(void);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_RPCLI_PROPERTIES_HPP__ */
