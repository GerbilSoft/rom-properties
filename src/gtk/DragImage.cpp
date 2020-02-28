/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DragImage.cpp: Drag & Drop image.                                       *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DragImage.hpp"
#include "PIMGTYPE.hpp"

// librpbase, librptexture
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"
using namespace LibRpBase;
using LibRpTexture::rp_image;

// TODO: Adjust minimum image size based on DPI.
#define DIL_MIN_IMAGE_SIZE 32

static void	drag_image_dispose	(GObject	*object);
static void	drag_image_finalize	(GObject	*object);

// DragImage class.
struct _DragImageClass {
	GtkImageClass __parent__;
};

// DragImage instance.
struct _DragImage {
	GtkImage __parent__;

	// Minimum image size.
	struct {
		int width;
		int height;
	} minimumImageSize;

	// rp_image. (NOTE: Not owned by this object.)
	const rp_image *img;

	// Animated icon data.
	struct anim_vars {
		const IconAnimData *iconAnimData;
		guint tmrIconAnim;	// Timer ID
		int last_delay;		// Last delay value.
		std::array<PIMGTYPE, IconAnimData::MAX_FRAMES> iconFrames;
		IconAnimHelper iconAnimHelper;
		int last_frame_number;	// Last frame number.

		anim_vars()
			: tmrIconAnim(0)
			, last_delay(0)
			, last_frame_number(0)
		{
			iconFrames.fill(nullptr);
		}
		~anim_vars() {
			if (tmrIconAnim > 0) {
				g_source_remove(tmrIconAnim);
			}

			for (auto iter = iconFrames.begin(); iter != iconFrames.end(); ++iter) {
				if (*iter) {
					PIMGTYPE_destroy(*iter);
				}
			}
		}
	};
	anim_vars *anim;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(DragImage, drag_image,
	GTK_TYPE_IMAGE, static_cast<GTypeFlags>(0), {});

static void
drag_image_class_init(DragImageClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = drag_image_dispose;
	gobject_class->finalize = drag_image_finalize;

	// TODO
	//gobject_class->get_property = drag_image_get_property;
	//gobject_class->set_property = drag_image_set_property;
}

static void
drag_image_init(DragImage *image)
{
	image->minimumImageSize.width = DIL_MIN_IMAGE_SIZE;
	image->minimumImageSize.height = DIL_MIN_IMAGE_SIZE;
	image->img = nullptr;
	image->anim = nullptr;
}

static void
drag_image_dispose(GObject *object)
{
	DragImage *const image = DRAG_IMAGE(object);

	// Delete the animation data if present.
	// This will automatically unregister the timer.
	delete image->anim;
	image->anim = nullptr;

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(drag_image_parent_class)->dispose(object);
}

static void
drag_image_finalize(GObject *object)
{
	//DragImage *const image = DRAG_IMAGE(object);

	// Nothing to do here right now...

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(drag_image_parent_class)->finalize(object);
}

GtkWidget*
drag_image_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_DRAG_IMAGE, nullptr));
}

/**
 * Update the pixmap(s).
 * @param image DragImage
 * @return True on success; false on error.
 */
static bool
drag_image_update_pixmaps(DragImage *image)
{
	g_return_val_if_fail(IS_DRAG_IMAGE(image), false);

	auto *const anim = image->anim;
	if (anim && anim->iconAnimData) {
		const IconAnimData *const iconAnimData = anim->iconAnimData;

		// Convert the frames to PIMGTYPE.
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			// Remove the existing frame first.
			if (anim->iconFrames[i]) {
				PIMGTYPE_destroy(anim->iconFrames[i]);
				anim->iconFrames[i] = nullptr;
			}

			const rp_image *const frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				// NOTE: Allowing NULL frames here...
				anim->iconFrames[i] = rp_image_to_PIMGTYPE(frame);
			}
		}

		// Set up the IconAnimHelper.
		anim->iconAnimHelper.setIconAnimData(iconAnimData);
		if (anim->iconAnimHelper.isAnimated()) {
			// Initialize the animation.
			anim->last_frame_number = anim->iconAnimHelper.frameNumber();

			// Animation timer will be set up when animation is started.
		}

		// Show the first frame.
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image),
			anim->iconFrames[anim->iconAnimHelper.frameNumber()]);
		return true;
	}

	if (image->img && image->img->isValid()) {
		// Single image.
		PIMGTYPE pImg = rp_image_to_PIMGTYPE(image->img);
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image), pImg);
		if (pImg) {
			PIMGTYPE_destroy(pImg);
		}
		return true;
	}

	// No image or animated icon data.
	return false;
}

void
drag_image_get_minimum_image_size(DragImage *image, int *width, int *height)
{
	g_return_if_fail(IS_DRAG_IMAGE(image));

	*width = image->minimumImageSize.width;
	*height = image->minimumImageSize.height;
}

void
drag_image_set_minimum_image_size(DragImage *image, int width, int height)
{
	g_return_if_fail(IS_DRAG_IMAGE(image));

	if (image->minimumImageSize.width != width &&
	    image->minimumImageSize.height != height)
	{
		image->minimumImageSize.width = width;
		image->minimumImageSize.height = height;
		drag_image_update_pixmaps(image);
	}
}

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
bool
drag_image_set_rp_image(DragImage *image, const LibRpTexture::rp_image *img)
{
	g_return_val_if_fail(IS_DRAG_IMAGE(image), false);

	if (!img) {
		image->img = nullptr;
		if (!image->anim || !image->anim->iconAnimData) {
			gtk_image_clear(GTK_IMAGE(image));
		} else {
			return drag_image_update_pixmaps(image);
		}
		return false;
	}

	// Don't check if the image pointer matches the
	// previously stored image, since the underlying
	// image may have changed.
	image->img = img;
	return drag_image_update_pixmaps(image);
}

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
bool
drag_image_set_icon_anim_data(DragImage *image, const LibRpBase::IconAnimData *iconAnimData)
{
	g_return_val_if_fail(IS_DRAG_IMAGE(image), false);

	if (!image->anim) {
		image->anim = new DragImage::anim_vars();
	}
	auto *const anim = image->anim;

	if (!iconAnimData) {
		if (anim->tmrIconAnim > 0) {
			g_source_remove(anim->tmrIconAnim);
			anim->tmrIconAnim = 0;
		}
		anim->iconAnimData = nullptr;

		if (!image->img) {
			gtk_image_clear(GTK_IMAGE(image));
		} else {
			return drag_image_update_pixmaps(image);
		}
		return false;
	}

	// Don't check if the data pointer matches the
	// previously stored data, since the underlying
	// data may have changed.
	anim->iconAnimData = iconAnimData;
	return drag_image_update_pixmaps(image);
}

/**
 * Clear the rp_image and iconAnimData.
 * This will stop the animation timer if it's running.
 * @param image DragImage
 */
void
drag_image_clear(DragImage *image)
{
	g_return_if_fail(IS_DRAG_IMAGE(image));

	auto *const anim = image->anim;
	if (anim) {
		if (anim->tmrIconAnim > 0) {
			g_source_remove(anim->tmrIconAnim);
			anim->tmrIconAnim = 0;
		}
		anim->iconAnimData = nullptr;
	}

	image->img = nullptr;
	gtk_image_clear(GTK_IMAGE(image));
}

/**
 * Animated icon timer.
 * @param image DragImage
 * @return True to continue the timer; false to stop it.
 */
static gboolean
drag_image_anim_timer_func(DragImage *image)
{
	g_return_val_if_fail(IS_DRAG_IMAGE(image), false);
	auto *const anim = image->anim;
	g_return_val_if_fail(anim != nullptr, false);

	if (anim->tmrIconAnim == 0) {
		// Shutting down...
		return false;
	}

	// Next frame.
	int delay = 0;
	int frame = anim->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		anim->tmrIconAnim = 0;
		return false;
	}

	if (frame != anim->last_frame_number) {
		// New frame number.
		// Update the icon.
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image), anim->iconFrames[frame]);
		anim->last_frame_number = frame;
	}

	if (anim->last_delay != delay) {
		// Set a new timer and unset the current one.
		anim->last_delay = delay;
		anim->tmrIconAnim = g_timeout_add(delay,
			reinterpret_cast<GSourceFunc>(drag_image_anim_timer_func), image);
		return false;
	}

	// Keep the current timer running.
	return true;
}

/**
 * Start the animation timer.
 * @param image DragImage
 */
void
drag_image_start_anim_timer(DragImage *image)
{
	g_return_if_fail(IS_DRAG_IMAGE(image));

	auto *const anim = image->anim;
	if (!anim || !anim->iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	anim->last_frame_number = anim->iconAnimHelper.frameNumber();
	const int delay = anim->iconAnimHelper.frameDelay();
	assert(delay > 0);
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Stop the animation timer first.
	drag_image_stop_anim_timer(image);

	// Set a single-shot timer for the current frame.
	image->anim->last_delay = delay;
	image->anim->tmrIconAnim = g_timeout_add(delay,
		reinterpret_cast<GSourceFunc>(drag_image_anim_timer_func), image);
}

/**
 * Stop the animation timer.
 * @param image DragImage
 */
void
drag_image_stop_anim_timer(DragImage *image)
{
	g_return_if_fail(IS_DRAG_IMAGE(image));
	auto *const anim = image->anim;
	g_return_if_fail(anim != nullptr);

	if (anim->tmrIconAnim > 0) {
		g_source_remove(anim->tmrIconAnim);
		anim->tmrIconAnim = 0;
		anim->last_delay = 0;
	}
}

/**
 * Is the animation timer running?
 * @param image DragImage
 * @return True if running; false if not.
 */
bool
drag_image_is_anim_timer_running(DragImage *image)
{
	g_return_val_if_fail(IS_DRAG_IMAGE(image), false);
	auto *const anim = image->anim;
	return (anim && (anim->tmrIconAnim > 0));
}

/**
 * Reset the animation frame.
 * This does NOT update the animation frame.
 * @param image DragImage
 */
void
drag_image_reset_anim_frame(DragImage *image)
{
	g_return_if_fail(IS_DRAG_IMAGE(image));

	auto *const anim = image->anim;
	if (anim) {
		anim->last_frame_number = 0;
	}
}
