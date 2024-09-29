/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DragImage.cpp: Drag & Drop image.                                       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DragImage.hpp"
#include "PIMGTYPE.hpp"

// Other rom-properties libraries
#include "librpbase/img/IconAnimHelper.hpp"
#include "librpfile/VectorFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::unique_ptr;

// GtkPopover was added in GTK 3.12.
// GMenuModel is also implied by this, since GMenuModel
// support was added to GTK+ 3.4.
#if GTK_CHECK_VERSION(3,11,5)
#  define USE_G_MENU_MODEL 1
#endif /* GTK_CHECK_VERSION(3,11,5) */

// GTK4 introduces GtkPicture, which supports arbitrary images.
// GtkImage has been relegated to icons only, and only really
// supports square images properly.
#if GTK_CHECK_VERSION(3,94,0)
#  define USE_GTK_PICTURE 1
#endif /* GTK_CHECK_VERSION(3,94,0) */

static GQuark ecksbawks_quark = 0;

static void	rp_drag_image_dispose	(GObject	*object);
static void	rp_drag_image_finalize	(GObject	*object);

// Signal handlers
static void	rp_drag_image_map_signal_handler  (RpDragImage *image, gpointer user_data);
static void	rp_drag_image_notify_width_or_height_signal_handler(RpDragImage *image, GParamSpec *pspec, gpointer user_data);
#if GTK_CHECK_VERSION(4,0,0)
static GdkContentProvider *rp_drag_image_drag_source_prepare(GtkDragSource *source, double x, double y, RpDragImage *image);
static void	rp_drag_image_drag_source_drag_begin(GtkDragSource *source, GdkDrag *drag, RpDragImage *image);
static void	rp_drag_image_drag_source_drag_end(GtkDragSource *source, GdkDrag *drag, gboolean delete_data, RpDragImage *image);
#else /* !GTK_CHECK_VERSION(4,0,0) */
static void	rp_drag_image_drag_begin(RpDragImage *image, GdkDragContext *context, gpointer user_data);
static void	rp_drag_image_drag_data_get(RpDragImage *image, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#ifdef USE_G_MENU_MODEL
static void	ecksbawks_action_triggered_signal_handler     (GSimpleAction	*action,
							       GVariant		*parameter,
							       RpDragImage	*widget);
#else /* !USE_G_MENU_MODEL */
static void	ecksbawks_menuItem_triggered_signal_handler   (GtkMenuItem	*menuItem,
							       RpDragImage	*widget);
#endif /* USE_G_MENU_MODEL */

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

// C++ objects
struct _RpDragImageCxx {
	_RpDragImageCxx()
		: anim(nullptr)
	{}

	~_RpDragImageCxx()
	{
#if GTK_CHECK_VERSION(4,0,0)
		if (pngBytes) {
			g_bytes_unref(pngBytes);
		}
#endif /* GTK_CHECK_VERSION(4,0,0) */
	}

	// rp_image (C++ shared_ptr)
	rp_image_const_ptr img;

	// Animated icon data
	struct anim_vars {
		IconAnimDataConstPtr iconAnimData;
		array<PIMGTYPE, IconAnimData::MAX_FRAMES> iconFrames;
		IconAnimHelper iconAnimHelper;
		guint tmrIconAnim;	// Timer ID
		int last_delay;		// Last delay value.
		int last_frame_number;	// Last frame number.

		void unregister_timer(void)
		{
			g_clear_handle_id(&tmrIconAnim, g_source_remove);
		}

		anim_vars()
			: tmrIconAnim(0)
			, last_delay(0)
			, last_frame_number(0)
		{
			iconFrames.fill(nullptr);
		}
		~anim_vars()
		{
			g_clear_handle_id(&tmrIconAnim, g_source_remove);

			for (PIMGTYPE frame : iconFrames) {
				if (frame) {
					PIMGTYPE_unref(frame);
				}
			}
		}
	};
	unique_ptr<anim_vars> anim;

#if GTK_CHECK_VERSION(4,0,0)
	// Temporary buffer for PNG data when dragging and dropping images.
	VectorFilePtr pngData;
	GBytes *pngBytes;
#endif /* GTK_CHECK_VERSION(4,0,0) */
};

// DragImage instance
struct _RpDragImage {
	super __parent__;
	_RpDragImageCxx *cxx;	// C++ objects
	PIMGTYPE curFrame;	// Current frame

	// GtkImage (GTK2/GTK3) or GtkPicture (GTK4) child widget
	GtkWidget *imageWidget;

	bool dirty;	// true if the pixmaps need to be updated on next map
	bool ecksBawks;

#ifdef USE_G_MENU_MODEL
	GMenu *menuEcksBawks;
	GtkWidget *popEcksBawks;		// GtkPopover (3.x); GtkPopoverMenu (4.x)
	GSimpleActionGroup *actionGroup;
#else /* !USE_G_MENU_MODEL */
	GtkWidget *menuEcksBawks;	// GtkMenu
#endif /* USE_G_MENU_MODEL */

#if GTK_CHECK_VERSION(4,0,0)
	GtkDragSource *dragSource;
#endif /* GTK_CHECK_VERSION(4,0,0) */
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
	gobject_class->finalize = rp_drag_image_finalize;

	// TODO
	//gobject_class->set_property = rp_drag_image_set_property;
	//gobject_class->get_property = rp_drag_image_get_property;
}

static void
rp_drag_image_init(RpDragImage *image)
{
	// g_object_new() guarantees that all values are initialized to 0.

	// Initialize C++ objects.
	image->cxx = new _RpDragImageCxx();

	// Create the child GtkImage widget.
#ifdef USE_GTK_PICTURE
	image->imageWidget = gtk_picture_new();
#else /* !USE_GTK_PICTURE */
	image->imageWidget = gtk_image_new();
#endif /* USE_GTK_PICTURE */

	gtk_widget_set_name(image->imageWidget, "imageWidget");
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(image), image->imageWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(image->imageWidget);
	gtk_container_add(GTK_CONTAINER(image), image->imageWidget);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Pixmaps can only be updated once we have a valid size.
	g_signal_connect(G_OBJECT(image), "map", G_CALLBACK(rp_drag_image_map_signal_handler),   nullptr);
	g_signal_connect(G_OBJECT(image), "notify::width-request",  G_CALLBACK(rp_drag_image_notify_width_or_height_signal_handler), nullptr);
	g_signal_connect(G_OBJECT(image), "notify::height-request", G_CALLBACK(rp_drag_image_notify_width_or_height_signal_handler), nullptr);

#if GTK_CHECK_VERSION(4,0,0)
	image->dragSource = gtk_drag_source_new();
	g_signal_connect(G_OBJECT(image->dragSource), "prepare",    G_CALLBACK(rp_drag_image_drag_source_prepare),    image);
	g_signal_connect(G_OBJECT(image->dragSource), "drag-begin", G_CALLBACK(rp_drag_image_drag_source_drag_begin), image);
	g_signal_connect(G_OBJECT(image->dragSource), "drag-end",   G_CALLBACK(rp_drag_image_drag_source_drag_end),   image);
	gtk_widget_add_controller(GTK_WIDGET(image), GTK_EVENT_CONTROLLER(image->dragSource));
#else /* !GTK_CHECK_VERSION(4,0,0) */
	g_signal_connect(G_OBJECT(image), "drag-begin",    G_CALLBACK(rp_drag_image_drag_begin),    nullptr);
	g_signal_connect(G_OBJECT(image), "drag-data-get", G_CALLBACK(rp_drag_image_drag_data_get), nullptr);
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

static void
rp_drag_image_dispose(GObject *object)
{
	RpDragImage *const image = RP_DRAG_IMAGE(object);

	// Unreference the current frame if we still have it.
	g_clear_pointer(&image->curFrame, PIMGTYPE_unref);

	// Unregister the animation timer if it's set.
	if (image->cxx->anim) {
		image->cxx->anim->unregister_timer();
	}

#ifdef USE_G_MENU_MODEL
#  if !GTK_CHECK_VERSION(4,0,0)
	if (image->popEcksBawks) {
		gtk_widget_destroy(image->popEcksBawks);
		image->popEcksBawks = nullptr;
	}
#  endif /* !GTK_CHECK_VERSION(4,0,0) */
	g_clear_object(&image->menuEcksBawks);

	// The GSimpleActionGroup owns the actions, so
	// this will automatically delete the actions.
	g_clear_object(&image->actionGroup);
#else /* !USE_G_MENU_MODEL */
	if (image->menuEcksBawks) {
		gtk_widget_destroy(image->menuEcksBawks);
		image->menuEcksBawks = nullptr;
	}
#endif /* !USE_G_MENU_MODEL */

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_drag_image_parent_class)->dispose(object);
}

static void
rp_drag_image_finalize(GObject *object)
{
	RpDragImage *const image = RP_DRAG_IMAGE(object);

	// Delete the C++ objects.
	delete image->cxx;

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_drag_image_parent_class)->finalize(object);
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
	_RpDragImageCxx *const cxx = image->cxx;
	auto *const anim = cxx->anim.get();
	bool bRet = false;

	if (!gtk_widget_get_mapped(GTK_WIDGET(image))) {
		// RpDragImage is not mapped to the screen.
		// Set the dirty flag and update pixmaps later.
		// The "dirty" flag will force an update when mapped.
		image->dirty = true;

		// Return a value similar to the rest of the function.
		if (anim && anim->iconAnimData) {
			bRet = true;
		} else if (cxx->img && cxx->img->isValid()) {
			bRet = true;
		}
		return bRet;
	}

	g_clear_pointer(&image->curFrame, PIMGTYPE_unref);

#if GTK_CHECK_VERSION(3,0,0)
	// NOTE: In testing, the two sizes (minimum and natural) returned by
	// gtk_widget_get_preferred_size() are both the same if
	// gtk_widget_set_size_request() is called.
	// If it's not called, then both are 0 x 0.
	GtkRequisition req_sz;
	gtk_widget_get_preferred_size(GTK_WIDGET(image), &req_sz, nullptr);
#else /* !GTK_CHECK_VERSION(3,0,0) */
	GtkRequisition req_sz;
	gtk_widget_size_request(GTK_WIDGET(image), &req_sz);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	const bool doRescaleIfNeeded = (req_sz.width > 0 && req_sz.height > 0);

	// FIXME: Transparency isn't working for e.g. GALE01.gci.
	// (Super Smash Bros. Melee)
	if (anim && anim->iconAnimData) {
		const IconAnimDataConstPtr &iconAnimData = anim->iconAnimData;

		// Convert the frames to PIMGTYPE.
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			// Remove the existing frame first.
			g_clear_pointer(&anim->iconFrames[i], PIMGTYPE_unref);

			const rp_image_const_ptr &frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				// NOTE: Allowing NULL frames here...
				PIMGTYPE img = rp_image_to_PIMGTYPE(frame);
				if (img && doRescaleIfNeeded && (frame->width() != req_sz.width || frame->height() != req_sz.height)) {
					// Need to rescale the image.
					// TODO: Only check the first frame, then set a bool?
					// TODO: Verify High-DPI.
					// TODO: Nearest-neighbor scaling?
					PIMGTYPE scale_img = PIMGTYPE_scale(img, req_sz.width, req_sz.height, true);
					if (scale_img) {
						PIMGTYPE_unref(img);
						img = scale_img;
					}
				}
				anim->iconFrames[i] = img;
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
#ifdef USE_GTK_PICTURE
		gtk_picture_set_paintable(GTK_PICTURE(image->imageWidget), GDK_PAINTABLE(image->curFrame));
#else /* !USE_GTK_PICTURE */
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), image->curFrame);
#endif /* USE_GTK_PICTURE */
		bRet = true;
	} else if (cxx->img && cxx->img->isValid()) {
		// Single image.
		PIMGTYPE img = rp_image_to_PIMGTYPE(cxx->img);
		if (img && doRescaleIfNeeded && (cxx->img->width() != req_sz.width || cxx->img->height() != req_sz.height)) {
			// Need to rescale the image.
			// TODO: Verify High-DPI.
			// TODO: Nearest-neighbor scaling?
			PIMGTYPE scale_img = PIMGTYPE_scale(img, req_sz.width, req_sz.height, true);
			if (scale_img) {
				PIMGTYPE_unref(img);
				img = scale_img;
			}
		}
		image->curFrame = img;
#ifdef USE_GTK_PICTURE
		gtk_picture_set_paintable(GTK_PICTURE(image->imageWidget), GDK_PAINTABLE(image->curFrame));
#else /* !USE_GTK_PICTURE */
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), image->curFrame);
#endif /* USE_GTK_PICTURE */
		bRet = true;
	}

	// FIXME: GTK4 has a new Drag & Drop API.
#if !GTK_CHECK_VERSION(4,0,0)
	if (bRet) {
		// Image or animated icon data was set.
		// Set a drag source.
		// TODO: Use text/uri-list and extract to a temporary directory?
		// FIXME: application/octet-stream works on Nautilus, but not Thunar...
		static const GtkTargetEntry targetEntries[2] = {
			{(char*)"image/png",			// target
			 GTK_TARGET_OTHER_APP,			// flags
			 1},					// info

			{(char*)"application/octet-stream",	// target
			 GTK_TARGET_OTHER_APP,			// flags
			 2},					// info
		};
		gtk_drag_source_set(GTK_WIDGET(image), GDK_BUTTON1_MASK, targetEntries, ARRAY_SIZE(targetEntries), GDK_ACTION_COPY);
	} else {
		// No image or animated icon data.
		// Unset the drag source.
		gtk_drag_source_unset(GTK_WIDGET(image));
	}
#endif /* !GTK_CHECK_VERSION(4,0,0) */

	image->dirty = false;
	return bRet;
}

bool rp_drag_image_get_ecks_bawks(RpDragImage *image)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);
	return image->ecksBawks;
}

#if GTK_CHECK_VERSION(4,0,0)
static void
rp_drag_image_on_gesture_pressed_event(GtkGestureClick *self, gint n_press, gdouble x, gdouble y, RpDragImage *image)
{
	RP_UNUSED(self);
	RP_UNUSED(x);
	RP_UNUSED(y);

	if (!image->ecksBawks)
		return;

	// Only show the menu on the first right-click per gesture.
	if (n_press != 1)
		return;

	gtk_popover_popup(GTK_POPOVER(image->popEcksBawks));
}
#else /* !GTK_CHECK_VERSION(4,0,0) */
static void
rp_drag_image_on_button_press_event(RpDragImage *image, GdkEventButton *event, gpointer userdata)
{
	RP_UNUSED(userdata);
	if (!image->ecksBawks)
		return;

	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
#ifdef USE_G_MENU_MODEL
#  if GTK_CHECK_VERSION(3,21,5)
		gtk_popover_popup(GTK_POPOVER(image->popEcksBawks));
#  else /* !GTK_CHECK_VERSION(3,21,5) */
		gtk_widget_show(image->popEcksBawks);
#  endif /* GTK_CHECK_VERSION(3,21,5) */
#else /* !USE_G_MENU_MODEL */
		gtk_menu_popup(GTK_MENU(image->menuEcksBawks),
			nullptr, nullptr, nullptr,
			image, event->button,
			gdk_event_get_time((GdkEvent*)event));
#endif /* USE_G_MENU_MODEL */
	}
}
#endif /* GTK_CHECK_VERSION(4,0,0) */

void rp_drag_image_set_ecks_bawks(RpDragImage *image, bool new_ecks_bawks)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));
	image->ecksBawks = new_ecks_bawks;
	if (!image->ecksBawks)
		return;
	if (image->menuEcksBawks)
		return;

	// Create the Ecks Bawks popup menu.
	if (ecksbawks_quark == 0) {
		ecksbawks_quark = g_quark_from_string("ecksbawks");
	}
	static const array<const char*, 2> menu_items = {{
		"ermahgerd! an ecks bawks ISO!",
		"Yar, har, fiddle dee dee",
	}};
#ifdef USE_G_MENU_MODEL
	image->menuEcksBawks = g_menu_new();
	image->actionGroup = g_simple_action_group_new();

	char prefix[64];
	char buf[128];
	snprintf(prefix, sizeof(prefix), "rp-EcksBawks-%p", image);

	for (int i = 0; i < ARRAY_SIZE_I(menu_items); i++) {
		snprintf(buf, sizeof(buf), "ecksbawks-%d", i+1);
		GSimpleAction *const action = g_simple_action_new(buf, nullptr);
		g_simple_action_set_enabled(action, TRUE);
		g_object_set_qdata(G_OBJECT(action), ecksbawks_quark, GINT_TO_POINTER(i+1));
		g_signal_connect(action, "activate", G_CALLBACK(ecksbawks_action_triggered_signal_handler), image);
		g_action_map_add_action(G_ACTION_MAP(image->actionGroup), G_ACTION(action));
		snprintf(buf, sizeof(buf), "%s.ecksbawks-%d", prefix, i+1);
		g_menu_append(image->menuEcksBawks, menu_items[i], buf);
	}

	gtk_widget_insert_action_group(GTK_WIDGET(image), prefix, G_ACTION_GROUP(image->actionGroup));
#  if GTK_CHECK_VERSION(4,0,0)
	image->popEcksBawks = gtk_popover_menu_new_from_model(G_MENU_MODEL(image->menuEcksBawks));
	// GTK4: Need to set parent. Otherwise, gtk_popover_popup() will crash.
	gtk_widget_set_parent(image->popEcksBawks, GTK_WIDGET(image));
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	image->popEcksBawks = gtk_popover_new_from_model(GTK_WIDGET(image), G_MENU_MODEL(image->menuEcksBawks));
#    if GTK_CHECK_VERSION(3,15,8) && !GTK_CHECK_VERSION(3,21,5)
	gtk_popover_set_transitions_enabled(GTK_POPOVER(image->popEcksBawks), true);
#    endif /* GTK_CHECK_VERSION(3,15,8) && !GTK_CHECK_VERSION(3,21,5) */
#  endif /* GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_set_name(image->popEcksBawks, "popEcksBawks");
#else /* !USE_G_MENU_MODEL */

	image->menuEcksBawks = gtk_menu_new();
	gtk_widget_set_name(image->menuEcksBawks, "menuEcksBawks");

	for (int i = 0; i < ARRAY_SIZE_I(menu_items); i++) {
		GtkWidget *const action = gtk_menu_item_new_with_label(menu_items[i]);
		g_object_set_qdata(G_OBJECT(action), ecksbawks_quark, GINT_TO_POINTER(i+1));
		g_signal_connect(action, "activate", G_CALLBACK(ecksbawks_menuItem_triggered_signal_handler), image);
		gtk_widget_show(action);
		gtk_menu_shell_append(GTK_MENU_SHELL(image->menuEcksBawks), action);
	}
#endif

#if GTK_CHECK_VERSION(4,0,0)
	// GTK4: Use GtkGestureClick to handle right-click.
	// NOTE: GtkWidget takes ownership of the gesture object.
	GtkGesture *const rightClickGesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(rightClickGesture), GDK_BUTTON_SECONDARY);
	gtk_widget_add_controller(GTK_WIDGET(image), GTK_EVENT_CONTROLLER(rightClickGesture));
	g_signal_connect(rightClickGesture, "pressed", G_CALLBACK(rp_drag_image_on_gesture_pressed_event), image);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	// GTK2/GTK3: Show context menu on right-click.
	// NOTE: On my system, programs show context menus on mouse button down.
	// On Windows, it shows the menu on mouse button up?
	g_signal_connect(image, "button-press-event", G_CALLBACK(rp_drag_image_on_button_press_event), nullptr);
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

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
bool
rp_drag_image_set_rp_image(RpDragImage *image, const rp_image_const_ptr &img)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);
	_RpDragImageCxx *const cxx = image->cxx;

	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.

	cxx->img = img;
	if (!img) {
		if (!cxx->anim || !cxx->anim->iconAnimData) {
#ifdef USE_GTK_PICTURE
			gtk_picture_set_paintable(GTK_PICTURE(image->imageWidget), nullptr);
#else /* !USE_GTK_PICTURE */
			gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), nullptr);
#endif /* USE_GTK_PICTURE */
		} else {
			return rp_drag_image_update_pixmaps(image);
		}
		return false;
	}
	return rp_drag_image_update_pixmaps(image);
}

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
bool
rp_drag_image_set_icon_anim_data(RpDragImage *image, const IconAnimDataConstPtr &iconAnimData)
{
	g_return_val_if_fail(RP_IS_DRAG_IMAGE(image), false);
	_RpDragImageCxx *const cxx = image->cxx;

	if (!cxx->anim) {
		cxx->anim.reset(new _RpDragImageCxx::anim_vars());
	}
	auto *const anim = cxx->anim.get();

	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.

	anim->iconAnimData = iconAnimData;
	if (!iconAnimData) {
		g_clear_handle_id(&anim->tmrIconAnim, g_source_remove);

		if (!cxx->img) {
#ifdef USE_GTK_PICTURE
			gtk_picture_set_paintable(GTK_PICTURE(image->imageWidget), nullptr);
#else /* !USE_GTK_PICTURE */
			gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), nullptr);
#endif /* USE_GTK_PICTURE */
		} else {
			return rp_drag_image_update_pixmaps(image);
		}
		return false;
	}
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
	_RpDragImageCxx *const cxx = image->cxx;

	auto *const anim = cxx->anim.get();
	if (anim) {
		g_clear_handle_id(&anim->tmrIconAnim, g_source_remove);
		anim->iconAnimData.reset();
	}

	cxx->img.reset();
#ifdef USE_GTK_PICTURE
	gtk_picture_set_paintable(GTK_PICTURE(image->imageWidget), nullptr);
#else /* !USE_GTK_PICTURE */
	gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), nullptr);
#endif /* USE_GTK_PICTURE */
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
	auto *const anim = image->cxx->anim.get();
	g_return_val_if_fail(anim != nullptr, false);

	if (anim->tmrIconAnim == 0) {
		// Shutting down...
		return false;
	}

	// Next frame.
	int delay = 0;
	const int frame = anim->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		anim->tmrIconAnim = 0;
		return false;
	}

	if (frame != anim->last_frame_number) {
		// New frame number.
		// Update the icon.
#ifdef USE_GTK_PICTURE
		gtk_picture_set_paintable(GTK_PICTURE(image->imageWidget), GDK_PAINTABLE(anim->iconFrames[frame]));
#else /* !USE_GTK_PICTURE */
		gtk_image_set_from_PIMGTYPE(GTK_IMAGE(image->imageWidget), anim->iconFrames[frame]);
#endif /* USE_GTK_PICTURE */
		anim->last_frame_number = frame;
	}

	if (anim->last_delay != delay) {
		// Set a new timer and unset the current one.
		anim->last_delay = delay;
		anim->tmrIconAnim = g_timeout_add(delay,
			G_SOURCE_FUNC(rp_drag_image_anim_timer_func), image);
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

	auto *const anim = image->cxx->anim.get();
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
	anim->last_delay = delay;
	anim->tmrIconAnim = g_timeout_add(delay,
		G_SOURCE_FUNC(rp_drag_image_anim_timer_func), image);
}

/**
 * Stop the animation timer.
 * @param image RpDragImage
 */
void
rp_drag_image_stop_anim_timer(RpDragImage *image)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(image));

	auto *const anim = image->cxx->anim.get();
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
	auto *const anim = image->cxx->anim.get();
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

	auto *const anim = image->cxx->anim.get();
	if (anim) {
		anim->last_frame_number = 0;
	}
}

/** Signal handlers **/

/**
 * DragImage is being mapped onto the screen.
 * @param image RpDragImage
 * @param user_data User data
 */
static void
rp_drag_image_map_signal_handler(RpDragImage *image, gpointer user_data)
{
	RP_UNUSED(user_data);

	if (image->dirty) {
		// rp_drag_image_update_pixmaps() clears image->dirty.
		rp_drag_image_update_pixmaps(image);
	}
}

/**
 * Requested width or height has changed.
 * @param image RpDragImage
 * @param pspec GObject parameter specification
 * @param user_data User data
 */
static void
rp_drag_image_notify_width_or_height_signal_handler(RpDragImage *image, GParamSpec *pspec, gpointer user_data)
{
	RP_UNUSED(pspec);
	RP_UNUSED(user_data);

	if (gtk_widget_get_mapped(GTK_WIDGET(image))) {
		// Update the pixmaps.
		// NOTE: This function might be called twice in a row
		// if both requested width and height are changed.
		rp_drag_image_update_pixmaps(image);
	} else {
		// Mark the image as dirty.
		image->dirty = true;
	}
}

/**
 * Create a PNG file for the drag & drop operation.
 * @param image RpDragImage
 * @return VectorFile containing the PNG data.
 */
static VectorFilePtr
rp_drag_image_create_PNG_file(RpDragImage *image)
{
	_RpDragImageCxx *const cxx = image->cxx;
	auto *const anim = cxx->anim.get();
	const bool isAnimated = (anim && anim->iconAnimData && anim->iconAnimHelper.isAnimated());

	VectorFilePtr pngData = std::make_shared<VectorFile>();
	unique_ptr<RpPngWriter> pngWriter;
	if (isAnimated) {
		// Animated icon.
		pngWriter.reset(new RpPngWriter(pngData, anim->iconAnimData));
	} else if (cxx->img) {
		// Standard icon.
		// NOTE: Using the source image because we want the original
		// size, not the resized version.
		pngWriter.reset(new RpPngWriter(pngData, cxx->img));
	} else {
		// No icon...
		return {};
	}

	if (!pngWriter->isOpen()) {
		// Unable to open the PNG writer.
		return {};
	}

	// TODO: Add text fields indicating the source game.

	int pwRet = pngWriter->write_IHDR();
	if (pwRet != 0) {
		// Error writing the PNG image...
		return {};
	}
	pwRet = pngWriter->write_IDAT();
	if (pwRet != 0) {
		// Error writing the PNG image...
		return {};
	}

	// RpPngWriter will finalize the PNG on delete.
	pngWriter.reset();
	return pngData;
}

#if GTK_CHECK_VERSION(4,0,0)
static GdkContentProvider*
rp_drag_image_drag_source_prepare(GtkDragSource *source, double x, double y, RpDragImage *image)
{
	RP_UNUSED(source);
	RP_UNUSED(x);
	RP_UNUSED(y);

	_RpDragImageCxx *const cxx = image->cxx;
	cxx->pngData = rp_drag_image_create_PNG_file(image);
	if (!cxx->pngData)
		return nullptr;

	if (cxx->pngBytes) {
		g_bytes_unref(cxx->pngBytes);
	}

	const std::vector<uint8_t> &pngVec = cxx->pngData->vector();
	cxx->pngBytes = g_bytes_new_static(pngVec.data(), pngVec.size());

	GdkContentProvider *providers[2] = {
		gdk_content_provider_new_for_bytes("image/png", cxx->pngBytes),
		gdk_content_provider_new_for_bytes("application/octet-stream", cxx->pngBytes),
	};
	return gdk_content_provider_new_union(providers, ARRAY_SIZE(providers));
}

static void
rp_drag_image_drag_source_drag_begin(GtkDragSource *source, GdkDrag *drag, RpDragImage *image)
{
	RP_UNUSED(drag);

	// Set the drag icon.
	// NOTE: gtk_drag_source_set_icon() takes its own reference to the PIMGTYPE.
	// TODO: Hotspot coordinates?
	gtk_drag_source_set_icon(source, GDK_PAINTABLE(image->curFrame), 0, 0);
}

static void
rp_drag_image_drag_source_drag_end(GtkDragSource *source, GdkDrag *drag, gboolean delete_data, RpDragImage *image)
{
	RP_UNUSED(source);
	RP_UNUSED(drag);
	RP_UNUSED(delete_data);

	_RpDragImageCxx *const cxx = image->cxx;
	g_clear_pointer(&cxx->pngBytes, g_bytes_unref);
	cxx->pngData.reset();
}
#else /* !GTK_CHECK_VERSION(4,0,0) */
static void
rp_drag_image_drag_begin(RpDragImage *image, GdkDragContext *context, gpointer user_data)
{
	RP_UNUSED(user_data);

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

	VectorFilePtr pngData = rp_drag_image_create_PNG_file(image);
	if (!pngData)
		return;

	// Set the selection data.
	// NOTE: gtk_selection_data_set() copies the data.
	const std::vector<uint8_t> &pngVec = pngData->vector();
	gtk_selection_data_set(data, gdk_atom_intern_static_string("image/png"), 8,
		pngVec.data(), static_cast<gint>(pngVec.size()));
}
#endif /* GTK_CHECK_VERSION(4,0,0) */

static void
ecksbawks_show_url(gint id)
{
	const char *uri = nullptr;

	switch (id) {
		default:
			assert(!"Invalid ecksbawks URL ID.");
			break;
		case 1:
			uri = "https://twitter.com/DeaThProj/status/1684469412978458624";
			break;
		case 2:
			uri = "https://github.com/xenia-canary/xenia-canary/pull/180";
			break;
	}

	if (uri) {
		g_app_info_launch_default_for_uri(uri, nullptr, nullptr);
	}
}

#ifdef USE_G_MENU_MODEL
static void
ecksbawks_action_triggered_signal_handler(GSimpleAction	*action,
					  GVariant	*parameter,
					  RpDragImage	*widget)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(widget));
	RP_UNUSED(parameter);

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_qdata(G_OBJECT(action), ecksbawks_quark));

	ecksbawks_show_url(id);
}
#else /* !USE_G_MENU_MODEL */
static void
ecksbawks_menuItem_triggered_signal_handler(GtkMenuItem	*menuItem,
					    RpDragImage	*widget)
{
	g_return_if_fail(RP_IS_DRAG_IMAGE(widget));

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_qdata(G_OBJECT(menuItem), ecksbawks_quark));

	ecksbawks_show_url(id);
}
#endif /* USE_G_MENU_MODEL */
