/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQtNS.hpp: RomPropertiesKDE namespace definitions.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/qglobal.h>

// NOTE: Using QT_VERSION_CHECK causes errors on moc-qt4 due to CMAKE_AUTOMOC.
// Reference: https://bugzilla.redhat.com/show_bug.cgi?id=1396755
// QT_VERSION_CHECK(6, 0, 0) -> 0x60000
// QT_VERSION_CHECK(5, 0, 0) -> 0x50000
#include <qglobal.h>

#if QT_VERSION >= 0x70000
#  error Needs updating for Qt7
#elif QT_VERSION >= 0x60000
#  define RP_KDE_SUFFIX KF6
#  define RP_KDE_UPPER "KF6"
#  define RP_KDE_LOWER "kf6"
#  define RomPropertiesKDE RomPropertiesKF6
#elif QT_VERSION >= 0x50000
#  define RP_KDE_SUFFIX KF5
#  define RP_KDE_UPPER "KF5"
#  define RP_KDE_LOWER "kf5"
#  define RomPropertiesKDE RomPropertiesKF5
#elif QT_VERSION >= 0x40000
#  define RP_KDE_SUFFIX KDE4
#  define RP_KDE_UPPER "KDE4"
#  define RP_KDE_LOWER "kde4"
#  define RomPropertiesKDE RomPropertiesKDE4
#else /* QT_VERSION < 0x40000 */
#  error Qt version is too old
#endif
