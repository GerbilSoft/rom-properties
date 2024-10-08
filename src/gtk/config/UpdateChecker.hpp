/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_UPDATE_CHECKER (rp_update_checker_get_type())
G_DECLARE_FINAL_TYPE(RpUpdateChecker, rp_update_checker, RP, UPDATE_CHECKER, GObject)

RpUpdateChecker	*rp_update_checker_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

/**
 * Check for updates.
 * The update check is run asynchronously in a separate thread.
 *
 * Results will be sent as signals:
 * - 'retrieved': Update version retrieved. (uint64_t parameter with the version)
 * - 'error': An error occurred. (gchar* parameter with the error message)
 *
 * @param updChecker Update checker
 */
void		rp_update_checker_run		(RpUpdateChecker *updChecker);

G_END_DECLS
