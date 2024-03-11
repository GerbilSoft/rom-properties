/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * RomThumbnailCreator.hpp: Thumbnail creator.                             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.kde.h"

#include <QtCore/qglobal.h>

// KDE Frameworks 5.100 introduced a new ThumbnailCreator interface.
// KDE Frameworks 5.101 deprecated the old ThumbCreator interface.
// KDE Frameworks 6 will only have ThumbnailCreator.
#include <KIO/ThumbnailCreator>

class RomThumbCreatorPrivate;
#define RomThumbnailCreatorPrivate RomThumbCreatorPrivate

class RomThumbnailCreator : public KIO::ThumbnailCreator
{
Q_OBJECT

public:
	RomThumbnailCreator(QObject *parent, const QVariantList &args);
	~RomThumbnailCreator() override;

public:
	/**
	 * Create a thumbnail. (new interface added in KF5 5.100)
	 *
	 * @param request ThumbnailRequest
	 * @return ThumbnailResult
	 */
	KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) final;

private:
	typedef KIO::ThumbnailCreator super;
	RomThumbnailCreatorPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(RomThumbnailCreator)
	Q_DISABLE_COPY(RomThumbnailCreator)
};
