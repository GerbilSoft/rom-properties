/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQUrl.hpp: QUrl utility functions                                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <qglobal.h>
#include <QtCore/QUrl>

/**
 * Localize a QUrl.
 * This function automatically converts certain URL schemes, e.g. desktop:/, to local paths.
 *
 * @param qUrl QUrl.
 * @return Localized QUrl, or empty QUrl on error.
 */
QUrl localizeQUrl(const QUrl &url);
