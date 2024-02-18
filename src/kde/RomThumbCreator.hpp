/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomThumbCreator.hpp: Thumbnail creator.                                 *
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
class RomThumbCreatorPrivate;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <kio/thumbcreator.h>
// TODO: ThumbCreatorV2 on KDE4 for user configuration?
// (This was merged into ThumbCreator for KF5.)

class RomThumbCreator : public ThumbCreator
{
public:
	RomThumbCreator();
	~RomThumbCreator() override;

public:
	/**
	 * Creates a thumbnail.
	 *
	 * Note that this method should not do any scaling.  The @p width and @p
	 * height parameters are provided as hints for images that are generated
	 * from non-image data (like text).
	 *
	 * @param path    The path of the file to create a preview for.  This is
	 *                always a local path.
	 * @param width   The requested preview width (see the note on scaling
	 *                above).
	 * @param height  The requested preview height (see the note on scaling
	 *                above).
	 * @param img     The QImage to store the preview in.
	 *
	 * @return @c true if a preview was successfully generated and store in @p
	 *         img, @c false otherwise.
	 */
	bool create(const QString &path, int width, int height, QImage &img) final;

	/**
	 * Returns the flags for this plugin.
	 *
	 * @return XOR'd flags values.
	 * @see Flags
	 */
	Flags flags(void) const final;

private:
	typedef ThumbCreator super;
	RomThumbCreatorPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(RomThumbCreator)
	Q_DISABLE_COPY(RomThumbCreator)
};
#endif /* QT_VERSION < QT_VERSION_CHECK(6,0,0) */

#ifdef HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H

#include <KIO/ThumbnailCreator>

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

#endif /* HAVE_KIOGUI_KIO_THUMBNAILCREATOR_H */
