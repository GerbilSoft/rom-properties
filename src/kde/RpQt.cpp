/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.cpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

// librpbase
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// C includes. (C++ namespace)
#include <cassert>

/**
 * Convert an rp_image to QImage.
 * @param image rp_image.
 * @return QImage.
 */
QImage rpToQImage(const rp_image *image)
{
	if (!image || !image->isValid())
		return QImage();

	// We should be using the RpQImageBackend.
	const RpQImageBackend *backend =
		dynamic_cast<const RpQImageBackend*>(image->backend());
	assert(backend != nullptr);
	if (!backend) {
		// Incorrect backend set.
		return QImage();
	}

	return backend->getQImage();
}
