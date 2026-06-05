/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * kde_register_backends.hpp: Register Backends function.                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.kde.h"

// RpQImageBackend
#include "RpQImageBackend.hpp"
using LibRpTexture::rp_image;

#ifndef RP_KDE_DISABLE_REGISTER_ACHQTDBUS
// Achievements backend
#include "AchQtDBus.hpp"
#endif /* !RP_KDE_DISABLE_REGISTER_ACHQTDBUS */

/**
 * Register the KDE backends for e.g. rp_image.
 * This must be called when a plugin or executable is loaded.
 */
static inline void kde_register_backends(void)
{
	// Register RpQImageBackend and AchQtDBus.
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
#if !defined(RP_KDE_DISABLE_REGISTER_ACHQTDBUS) && defined(ENABLE_ACHIEVEMENTS) && defined(HAVE_QtDBus_NOTIFY)
	AchQtDBus::instance();
#endif /* !RP_KDE_DISABLE_REGISTER_ACHQTDBUS && ENABLE_ACHIEVEMENTS && HAVE_QtDBus_NOTIFY */
}
