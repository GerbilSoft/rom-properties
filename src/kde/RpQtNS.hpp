/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQtNS.hpp: RomPropertiesKDE namespace definitions.                     *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(7,0,0)
#  error Needs updating for Qt7
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#  define RP_KDE_SUFFIX KF6
#  define RP_KDE_UPPER "KF6"
#  define RP_KDE_LOWER "kf6"
#  define RomPropertiesKDE RomPropertiesKF6
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  define RP_KDE_SUFFIX KF5
#  define RP_KDE_UPPER "KF5"
#  define RP_KDE_LOWER "kf5"
#  define RomPropertiesKDE RomPropertiesKF5
#elif QT_VERSION >= QT_VERSION_CHECK(4,0,0)
#  define RP_KDE_SUFFIX KDE4
#  define RP_KDE_UPPER "KDE4"
#  define RP_KDE_LOWER "kde4"
#  define RomPropertiesKDE RomPropertiesKDE4
#else /* QT_VERSION < QT_VERSION_CHECK(4,0,0) */
#  error Qt version is too old
#endif
