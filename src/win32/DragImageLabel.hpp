/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DragImageLabel.hpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "libwin32common/RpWin32_sdk.h"

// Other rom-properties libraries
#include "librpbase/img/IconAnimData.hpp"
#include "librptexture/img/rp_image.hpp"

#define IDM_ECKS_BAWKS_MENU_BASE	0x9000

class DragImageLabelPrivate;
class DragImageLabel
{
// TODO: Adjust image size based on DPI.
#define DIL_REQ_IMAGE_SIZE 32

public:
	explicit DragImageLabel(HWND hwndParent);
	~DragImageLabel();

private:
	DragImageLabelPrivate *const d_ptr;
	RP_DISABLE_COPY(DragImageLabel)

public:
	SIZE requiredSize(void) const;
	void setRequiredSize(SIZE requiredSize);
	void setRequiredSize(int width, int height);

	SIZE actualSize(void) const;

	POINT position(void) const;
	void setPosition(POINT position);
	void setPosition(int x, int y);

	/**
	 * Set the rp_image for this label.
	 *
	 * NOTE: If animated icon data is specified, that supercedes
	 * the individual rp_image.
	 *
	 * @param img rp_image, or nullptr to clear.
	 * @return True on success; false on error or if clearing.
	 */
	bool setRpImage(const LibRpTexture::rp_image_const_ptr &img);

	/**
	 * Set the icon animation data for this label.
	 *
	 * NOTE: If animated icon data is specified, that supercedes
	 * the individual rp_image.
	 *
	 * @param iconAnimData IconAnimData, or nullptr to clear.
	 * @return True on success; false on error or if clearing.
	 */
	bool setIconAnimData(const LibRpBase::IconAnimDataConstPtr &iconAnimData);

	/**
	 * Clear the rp_image and iconAnimData.
	 * This will stop the animation timer if it's running.
	 */
	void clearRp(void);

	bool ecksBawks(void) const;
	void setEcksBawks(bool newEcksBawks);

	void tryPopupEcksBawks(LPARAM lParam);

public:
	/**
	 * Start the animation timer.
	 */
	void startAnimTimer(void);

	/**
	 * Stop the animation timer.
	 */
	void stopAnimTimer(void);

	/**
	 * Is the animation timer running?
	 * @return True if running; false if not.
	 */
	bool isAnimTimerRunning(void) const;

	/**
	 * Reset the animation frame.
	 * This does NOT update the animation frame.
	 */
	void resetAnimFrame(void);

public:
	/**
	 * Get the current bitmap frame.
	 * @return HBITMAP.
	 */
	HBITMAP currentFrame(void) const;

	/**
	 * Draw the image.
	 * @param hdc Device context of the parent window.
	 */
	void draw(HDC hdc);

	/**
	 * Invalidate the bitmap rect.
	 * @param bErase Erase the background.
	 */
	void invalidateRect(bool bErase = false);

	/**
	 * Does a given rectangle intersect this control's rectangle?
	 * Typically used for WM_PAINT.
	 *
	 * @param lprcOther Rectangle to check
	 * @return True if it does; false if it doesn't.
	 */
	bool intersects(const RECT *lprcOther) const;
};
