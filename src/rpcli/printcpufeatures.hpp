/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * printcpufeatures.hpp: Print CPU features.                               *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Print CPU features.
 * @return 0 on success; non-zero on error.
 */
int PrintCPUFeatures(void);

#ifdef __cplusplus
}
#endif
