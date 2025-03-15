/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DragImage.hpp: Drag & Drop image.                                       *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_DRAG_IMAGE (rp_drag_image_get_type())

#if GTK_CHECK_VERSION(4, 0, 0)
#  define _RpDragImage_super		GtkBox
#  define _RpDragImage_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
#  define _RpDragImage_super		GtkEventBox
#  define _RpDragImage_superClass	GtkEventBoxClass
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

G_DECLARE_FINAL_TYPE(RpDragImage, rp_drag_image, RP, DRAG_IMAGE, _RpDragImage_super)

GtkWidget	*rp_drag_image_new		(void) G_GNUC_MALLOC;

bool rp_drag_image_get_ecks_bawks(RpDragImage *image);
void rp_drag_image_set_ecks_bawks(RpDragImage *image, bool new_ecks_bawks);

G_END_DECLS

#ifdef __cplusplus

// C++ includes
#include <memory>

namespace LibRpTexture {
	class rp_image;
}

// librpbase
#include "librpbase/img/IconAnimData.hpp"

/**
 * Set the rp_image for this image.
 *
 * NOTE: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param image RpDragImage
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool rp_drag_image_set_rp_image(RpDragImage *image, const LibRpTexture::rp_image_const_ptr &img);

/**
 * Set the icon animation data for this image.
 *
 * NOTE: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param image RpDragImage
 * @param iconAnimData IconAnimData, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool rp_drag_image_set_icon_anim_data(RpDragImage *image, const LibRpBase::IconAnimDataConstPtr &iconAnimData);

#endif /* __cplusplus */

/**
 * Clear the rp_image and iconAnimData.
 * This will stop the animation timer if it's running.
 * @param image RpDragImage
 */
void rp_drag_image_clear(RpDragImage *image);

/**
 * Start the animation timer.
 * @param image RpDragImage
 */
void rp_drag_image_start_anim_timer(RpDragImage *image);

/**
 * Stop the animation timer.
 * @param image RpDragImage
 */
void rp_drag_image_stop_anim_timer(RpDragImage *image);

/**
 * Is the animation timer running?
 * @param image RpDragImage
 * @return True if running; false if not.
 */
bool rp_drag_image_is_anim_timer_running(RpDragImage *image);

/**
 * Reset the animation frame.
 * This does NOT update the animation frame.
 * @param image RpDragImage
 */
void rp_drag_image_reset_anim_frame(RpDragImage *image);
