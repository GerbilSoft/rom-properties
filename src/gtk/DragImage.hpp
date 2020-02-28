/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DragImage.hpp: Drag & Drop image.                                       *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_DRAGIMAGE_HPP__
#define __ROMPROPERTIES_GTK_DRAGIMAGE_HPP__

#ifdef __cplusplus
namespace LibRpTexture {
	class rp_image;
}

// librpbase
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"

// C++ includes.
#include <array>
#endif /* __cplusplus */

// GTK+ includes.
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _DragImageClass	DragImageClass;
typedef struct _DragImage	DragImage;

#define TYPE_DRAG_IMAGE            (drag_image_get_type())
#define DRAG_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_DRAG_IMAGE, DragImage))
#define DRAG_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_DRAG_IMAGE, DragImageClass))
#define IS_DRAG_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_DRAG_IMAGE))
#define IS_DRAG_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_DRAG_IMAGE))
#define DRAG_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_DRAG_IMAGE, DragImageClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		drag_image_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		drag_image_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*drag_image_new			(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

// TODO: Make these properties?

void drag_image_get_minimum_image_size(DragImage *image, int *width, int *height);
void drag_image_set_minimum_image_size(DragImage *image, int width, int height);

#ifdef __cplusplus
/**
 * Set the rp_image for this image.
 *
 * NOTE: The rp_image pointer is stored and used if necessary.
 * Make sure to call this function with nullptr before deleting
 * the rp_image object.
 *
 * NOTE 2: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param image DragImage
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool drag_image_set_rp_image(DragImage *image, const LibRpTexture::rp_image *img);

/**
 * Set the icon animation data for this image.
 *
 * NOTE: The iconAnimData pointer is stored and used if necessary.
 * Make sure to call this function with nullptr before deleting
 * the IconAnimData object.
 *
 * NOTE 2: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param image DragImage
 * @param iconAnimData IconAnimData, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool drag_image_set_icon_anim_data(DragImage *image, const LibRpBase::IconAnimData *iconAnimData);

/**
 * Clear the rp_image and iconAnimData.
 * This will stop the animation timer if it's running.
 * @param image DragImage
 */
void drag_image_clear(DragImage *image);

/**
 * Start the animation timer.
 * @param image DragImage
 */
void drag_image_start_anim_timer(DragImage *image);

/**
 * Stop the animation timer.
 * @param image DragImage
 */
void drag_image_stop_anim_timer(DragImage *image);

/**
 * Is the animation timer running?
 * @param image DragImage
 * @return True if running; false if not.
 */
bool drag_image_is_anim_timer_running(DragImage *image);

/**
 * Reset the animation frame.
 * This does NOT update the animation frame.
 * @param image DragImage
 */
void drag_image_reset_anim_frame(DragImage *image);

#endif /* __cplusplus */

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_DRAGIMAGE_HPP__ */
