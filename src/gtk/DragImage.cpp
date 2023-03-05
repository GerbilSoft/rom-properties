/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DragImage.cpp: Drag & Drop image.                                       *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DragImage.hpp"
#include "PIMGTYPE.hpp"

// librpbase, librpfile, librptexture
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"
#include "librpfile/VectorFile.hpp"
using namespace LibRpBase;
using LibRpFile::VectorFile;
using LibRpTexture::rp_image;

// C++ STL classes.
using std::vector;

// TODO: Adjust minimum image size based on DPI.
#define DIL_MIN_IMAGE_SIZE 32

static void	rp_drag_image_dispose	(GObject	*object);

// Signal handlers
// FIXME: GTK4 has a new Drag & Drop API.
#if !GTK_CHECK_VERSION(4,0,0)
static void	rp_drag_image_drag_begin(RpDragImage *image, GdkDragContext *context, gpointer user_data);
static void	rp_drag_image_drag_data_get(RpDragImage *image, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

// GTK4 no longer needs GtkEventBox, since
// all widgets receive events.
#if GTK_CHECK_VERSION(4,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(4,0,0) */
typedef GtkEventBoxClass superclass;
typedef GtkEventBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_EVENT_BOX
#endif /* GTK_CHECK_VERSION(4,0,0) */

// DragImage class
struct _RpDragImageClass {
	superclass __parent__;
};

// DragImage instance
struct _RpDragImage {
	super __parent__;

	// GtkImage child widget.
	GtkWidget *imageWidget;
	// Current frame.
	PIMGTYPE curFrame;

	// Minimum image size.
	struct {
		int width;
		int height;
	} minimumImageSize;

	// rp_image. [ref()'d]
	const rp_image *img;

	// Animated icon data.
	struct anim_vars {
		const IconAnimData *iconAnimData;
		std::array<PIMGTYPE, IconAnimData::MAX_FRAMES> iconFrames;
		IconAnimHelper iconAnimHelper;
		guint tmrIconAnim;	// Timer ID
		int last_delay;		// Last delay value.
		int last_frame_number;	// Last frame number.

		anim_vars()
			: iconAnimData(nullptr)
			, tmrIconAnim(0)
			, last_delay(0)
			, last_frame_number(0)
		{
			iconFrames.fill(nullptr);
		}
		~anim_vars() {
			g_clear_handle_id(&tmrIconAnim, g_source_remove);

			for (PIMGTYPE frame : iconFrames) {
				if (frame) {
					PIMGTYPE_unref(frame);
				}
			}

			UNREF(iconAnimData);
		}
	};
	anim_vars *anim;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpDragImage, rp_drag_image,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
rp_drag_image_class_init(RpDragImageClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_drag_image_dispose;

	// TODO
	//gobject_class->set_property = rp_drag_image_set_property;
	//gobject_class->get_property = rp_drag_image_get_property;
}

static void
rp_drag_image_init(RpDragImage *image)
{
	image->minimumImageSize.width = DIL_MIN_IMAGE_SIZE;
	image->minimumImageSize.height = DIL_MIN_IMAGE_SIZE;
	image->img = nullptr;
	image->anim = nullptr;

	// Create the child GtkImage widget.
	image->imageWidget = gtk_image_new();
	gtk_widget_set_name(image->imageWidget, "imageWidget");
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(image), image->imageWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(image->imageWidget);
	gtk_container_add(GTK_CONTAINER(image), image->imageWidget);
#endif /* GTK_CHECK_VERSION(4,0,0) */

// FIXME: GTK4 has a new Drag & Drop API.
#if !GTK_CHECK_VERSION(4,0,0)
	g_signal_connect(G_OBJECT(image), "drag-begin",
		G_CALLBACK(rp_drag_image_drag_begin), (gpointer)0);
	g_signal_connect(G_OBJECT(image), "drag-data-get",
		G_CALLBACK(rp_drag_image_drag_data_get), (gpointer)0);
#endif /* !GTK_CHECK_VERSION(4,0,0) */
}

static void
rp_drag_image_dispose(GObject *object)
{
	RpDragImage *const image = RP_DRAG_IMAGE(object);

	// Unreference the current frame if we still have it.
	if (image->curFrame) {
		PIMGTYPE_unref(image->curFrame);
		image->curFrame = nullptr;
	}

	// Delete the animation data if present.
	// This will automatically unregister the timer.
	delete image->anim;
	image->anim = nullptr;

	// Unreference the image.
	UNREF_AND_NULL(image->img);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_drag_image_parent_class)->dispose(object);
}

GtkWidget*
rp_drag_image_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_DRAG_IMAGE, nullptr));
}

/**
 * Update the pixmap(s).
 * @param image RpDragImage
 * @return True on success; false on error.
 */
static bool
rp_drag_image_update_pixmaps(RpDragImage *image)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);
	bool bRet = false;

	if (image->curFrame) {
		PIMGTYPE_unref(image->curFrame);
		image->curFrame = nullptr;
	}

	// FIXME: Transparency isn't working for e.g. GALE01.gci.
	// (Super Smash Bros. Melee)
	auto *const anim = image->anim;
	if (anim && anim->iconAnimData) {
		const IconAnimData *const iconAnimData = anim->iconAnimData;

		// Convert the frames to PIMGTYPE.
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			// Remove the existing frame first.
			if (anim->iconFrames[i]) {
				PIMGTYPE_unref(anim->iconFrames[i]);
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
		image->curFrame = PIMGTYPE_ref(anim->iconFrames[anim->iconAnimHelper.frameNumber()]);
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), image->curFrame);
		bRet = true;
	} else if (image->img && image->img->isValid()) {
		// Single image.
		image->curFrame = rp_image_to_PIMGTYPE(image->img);
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), image->curFrame);
		bRet = true;
	}

	// FIXME: GTK4 has a new Drag & Drop API.
#if !GTK_CHECK_VERSION(4,0,0)
	if (bRet) {
		// Image or animated icon data was set.
		// Set a drag source.
		// TODO: Use text/uri-list and extract to a temporary directory?
		// FIXME: application/octet-stream works on Nautilus, but not Thunar...
		static const GtkTargetEntry targetEntry = {
			const_cast<char*>("application/octet-stream"),	// target
			GTK_TARGET_OTHER_APP,		// flags
			1,				// info
		};
		gtk_drag_source_set(GTK_WIDGET(image), GDK_BUTTON1_MASK, &targetEntry, 1, GDK_ACTION_COPY);
	} else {
		// No image or animated icon data.
		// Unset the drag source.
		gtk_drag_source_unset(GTK_WIDGET(image));
	}
#endif /* !GTK_CHECK_VERSION(4,0,0) */

	return bRet;
}

void
rp_drag_image_get_minimum_image_size(RpDragImage *image, int *width, int *height)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	*width = image->minimumImageSize.width;
	*height = image->minimumImageSize.height;
}

void
rp_drag_image_set_minimum_image_size(RpDragImage *image, int width, int height)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	if (image->minimumImageSize.width != width &&
	    image->minimumImageSize.height != height)
	{
		image->minimumImageSize.width = width;
		image->minimumImageSize.height = height;
		rp_drag_image_update_pixmaps(image);
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
 * @param image RpDragImage
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool
rp_drag_image_set_rp_image(RpDragImage *image, const LibRpTexture::rp_image *img)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);

	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	UNREF_AND_NULL(image->img);

	if (!img) {
		if (!image->anim || !image->anim->iconAnimData) {
			gtk_image_clear(GTK_IMAGE(image->imageWidget));
		} else {
			return rp_drag_image_update_pixmaps(image);
		}
		return false;
	}

	image->img = img->ref();
	return rp_drag_image_update_pixmaps(image);
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
 * @param image RpDragImage
 * @param iconAnimData IconAnimData, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool
rp_drag_image_set_icon_anim_data(RpDragImage *image, const LibRpBase::IconAnimData *iconAnimData)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);

	if (!image->anim) {
		image->anim = new RpDragImage::anim_vars();
	}
	auto *const anim = image->anim;

	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	UNREF_AND_NULL(anim->iconAnimData);

	if (!iconAnimData) {
		g_clear_handle_id(&anim->tmrIconAnim, g_source_remove);

		if (!image->img) {
			gtk_image_clear(GTK_IMAGE(image->imageWidget));
		} else {
			return rp_drag_image_update_pixmaps(image);
		}
		return false;
	}

	anim->iconAnimData = iconAnimData->ref();
	return rp_drag_image_update_pixmaps(image);
}

/**
 * Clear the rp_image and iconAnimData.
 * This will stop the animation timer if it's running.
 * @param image RpDragImage
 */
void
rp_drag_image_clear(RpDragImage *image)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	auto *const anim = image->anim;
	if (anim) {
		g_clear_handle_id(&anim->tmrIconAnim, g_source_remove);
		UNREF_AND_NULL(anim->iconAnimData);
	}

	UNREF_AND_NULL(image->img);
	gtk_image_clear(GTK_IMAGE(image->imageWidget));
}

/**
 * Animated icon timer.
 * @param image RpDragImage
 * @return True to continue the timer; false to stop it.
 */
static gboolean
rp_drag_image_anim_timer_func(RpDragImage *image)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);
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
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), anim->iconFrames[frame]);
		anim->last_frame_number = frame;
	}

	if (anim->last_delay != delay) {
		// Set a new timer and unset the current one.
		anim->last_delay = delay;
		anim->tmrIconAnim = g_timeout_add(delay,
			reinterpret_cast<GSourceFunc>(rp_drag_image_anim_timer_func), image);
		return false;
	}

	// Keep the current timer running.
	return true;
}

/**
 * Start the animation timer.
 * @param image RpDragImage
 */
void
rp_drag_image_start_anim_timer(RpDragImage *image)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

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
	rp_drag_image_stop_anim_timer(image);

	// Set a single-shot timer for the current frame.
	image->anim->last_delay = delay;
	image->anim->tmrIconAnim = g_timeout_add(delay,
		reinterpret_cast<GSourceFunc>(rp_drag_image_anim_timer_func), image);
}

/**
 * Stop the animation timer.
 * @param image RpDragImage
 */
void
rp_drag_image_stop_anim_timer(RpDragImage *image)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	auto *const anim = image->anim;
	if (anim) {
		g_clear_handle_id(&anim->tmrIconAnim, g_source_remove);
		anim->last_delay = 0;
	}
}

/**
 * Is the animation timer running?
 * @param image RpDragImage
 * @return True if running; false if not.
 */
bool
rp_drag_image_is_anim_timer_running(RpDragImage *image)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);
	auto *const anim = image->anim;
	return (anim && (anim->tmrIconAnim > 0));
}

/**
 * Reset the animation frame.
 * This does NOT update the animation frame.
 * @param image RpDragImage
 */
void
rp_drag_image_reset_anim_frame(RpDragImage *image)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	auto *const anim = image->anim;
	if (anim) {
		anim->last_frame_number = 0;
	}
}

/** Signal handlers **/

// FIXME: GTK4 has a new Drag & Drop API.
#if !GTK_CHECK_VERSION(4,0,0)
static void
rp_drag_image_drag_begin(RpDragImage *image, GdkDragContext *context, gpointer user_data)
{
	RP_UNUSED(user_data);
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	// Set the drag icon.
	// NOTE: gtk_drag_set_icon_PIMGTYPE() takes its own reference to the PIMGTYPE.
	// NOTE: Using gtk_drag_set_icon_PIMGTYPE() instead of gtk_drag_source_set_icon_pixbuf():
	// - Setting source is done before dragging.
	// - There's no source variant that takes a Cairo surface.
	gtk_drag_set_icon_PIMGTYPE(context, image->curFrame);
}

static void
rp_drag_image_drag_data_get(RpDragImage *image, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	RP_UNUSED(context);	// TODO
	RP_UNUSED(info);
	RP_UNUSED(time);
	RP_UNUSED(user_data);
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	auto *const anim = image->anim;
	const bool isAnimated = (anim && anim->iconAnimData && anim->iconAnimHelper.isAnimated());

	VectorFile *const pngData = new VectorFile();
	RpPngWriter *pngWriter;
	if (isAnimated) {
		// Animated icon.
		pngWriter = new RpPngWriter(pngData, anim->iconAnimData);
	} else if (image->img) {
		// Standard icon.
		// NOTE: Using the source image because we want the original
		// size, not the resized version.
		pngWriter = new RpPngWriter(pngData, image->img);
	} else {
		// No icon...
		pngData->unref();
		return;
	}

	if (!pngWriter->isOpen()) {
		// Unable to open the PNG writer.
		delete pngWriter;
		pngData->unref();
		return;
	}

	// TODO: Add text fields indicating the source game.

	int pwRet = pngWriter->write_IHDR();
	if (pwRet != 0) {
		// Error writing the PNG image...
		delete pngWriter;
		pngData->unref();
		return;
	}
	pwRet = pngWriter->write_IDAT();
	if (pwRet != 0) {
		// Error writing the PNG image...
		delete pngWriter;
		pngData->unref();
		return;
	}

	// RpPngWriter will finalize the PNG on delete.
	delete pngWriter;

	// Set the selection data.
	// NOTE: gtk_selection_data_set() copies the data.
	const vector<uint8_t> &pngVec = pngData->vector();
	gtk_selection_data_set(data, gdk_atom_intern_static_string("image/png"), 8,
		pngVec.data(), static_cast<gint>(pngVec.size()));

	// We're done here.
	pngData->unref();
}
#endif /* !GTK_CHECK_VERSION(4,0,0) */
