/***************************************************************************
 * ROM Properties Page shell extension. (D-Bus Thumbnailer)                *
 * rptsecure.h: Security options for rp-thumbnailer-dbus.                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_RP_THUMBNAILER_DBUS_RPTSECURE_H__
#define __ROMPROPERTIES_GTK_RP_THUMBNAILER_DBUS_RPTSECURE_H__

#include "stdboolx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable security options.
 * @return 0 on success; negative POSIX error code on error.
 */
int rpt_do_security_options(void);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_GTK_RP_THUMBNAILER_DBUS_RPTSECURE_H__ */
