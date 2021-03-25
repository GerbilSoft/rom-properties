/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsMenuButton.hpp: Options menu GtkMenuButton subclass.             *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_OPTIONSMENUBUTTON_HPP__
#define __ROMPROPERTIES_GTK_OPTIONSMENUBUTTON_HPP__

#ifdef __cplusplus
#  include "librpbase/RomData.hpp"
#endif /* __cplusplus */

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _OptionsMenuButtonClass	OptionsMenuButtonClass;
typedef struct _OptionsMenuButton	OptionsMenuButton;

#define TYPE_OPTIONS_MENU_BUTTON            (options_menu_button_get_type())
#define OPTIONS_MENU_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_OPTIONS_MENU_BUTTON, OptionsMenuButton))
#define OPTIONS_MENU_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_OPTIONS_MENU_BUTTON, OptionsMenuButtonClass))
#define IS_OPTIONS_MENU_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_OPTIONS_MENU_BUTTON))
#define IS_OPTIONS_MENU_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_OPTIONS_MENU_BUTTON))
#define OPTIONS_MENU_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_OPTIONS_MENU_BUTTON, OptionsMenuButtonClass))

enum StandardOptionID {
	OPTION_EXPORT_TEXT = -1,
	OPTION_EXPORT_JSON = -2,
	OPTION_COPY_TEXT = -3,
	OPTION_COPY_JSON = -4,
};

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		options_menu_button_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		options_menu_button_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*options_menu_button_new		(void) G_GNUC_MALLOC;

GtkArrowType	options_menu_button_get_direction	(OptionsMenuButton *widget);
void		options_menu_button_set_direction	(OptionsMenuButton *widget,
							 GtkArrowType arrowType);

#ifdef __cplusplus
void		options_menu_button_reinit_menu		(OptionsMenuButton *widget,
							 const LibRpBase::RomData *romData);

void		options_menu_button_update_op		(OptionsMenuButton *widget,
							 int id,
							 const LibRpBase::RomData::RomOp *op);
#endif /* __cplusplus */

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_OPTIONSMENUBUTTON_HPP__ */
