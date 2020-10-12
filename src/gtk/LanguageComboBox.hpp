/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBox.hpp: Language GtkComboBox subclass.                    *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_LANGUAGECOMBOBOX_HPP__
#define __ROMPROPERTIES_GTK_LANGUAGECOMBOBOX_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _LanguageComboBoxClass	LanguageComboBoxClass;
typedef struct _LanguageComboBox	LanguageComboBox;

#define TYPE_LANGUAGE_COMBO_BOX            (language_combo_box_get_type())
#define LANGUAGE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_LANGUAGE_COMBO_BOX, LanguageComboBox))
#define LANGUAGE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_LANGUAGE_COMBO_BOX, LanguageComboBoxClass))
#define IS_LANGUAGE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_LANGUAGE_COMBO_BOX))
#define IS_LANGUAGE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_LANGUAGE_COMBO_BOX))
#define LANGUAGE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_LANGUAGE_COMBO_BOX, LanguageComboBoxClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		language_combo_box_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		language_combo_box_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*language_combo_box_new			(void) G_GNUC_MALLOC;

void		language_combo_box_set_lcs		(LanguageComboBox *widget, const uint32_t *lcs_array);
uint32_t 	*language_combo_box_get_lcs		(LanguageComboBox *widget) G_GNUC_MALLOC;
void		language_combo_box_clear_lcs		(LanguageComboBox *widget);

gboolean	language_combo_box_set_selected_lc	(LanguageComboBox *widget, uint32_t lc);
uint32_t	language_combo_box_get_selected_lc	(LanguageComboBox *widget);

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_MESSAGEWIDGET_HPP__ */
