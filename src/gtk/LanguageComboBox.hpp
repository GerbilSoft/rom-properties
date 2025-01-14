/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBox.hpp: Language ComboBox                                 *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_LANGUAGE_COMBO_BOX (rp_language_combo_box_get_type())

#if GTK_CHECK_VERSION(3,0,0)
#  define _RpLanguageComboBox_super		GtkBox
#  define _RpLanguageComboBox_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3,0,0) */
#  define _RpLanguageComboBox_super		GtkHBox
#  define _RpLanguageComboBox_superClass	GtkHBoxClass
#endif /* GTK_CHECK_VERSION(3,0,0) */

G_DECLARE_FINAL_TYPE(RpLanguageComboBox, rp_language_combo_box, RP, LANGUAGE_COMBO_BOX, _RpLanguageComboBox_super)

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
void		rp_language_combo_box_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*rp_language_combo_box_new		(void) G_GNUC_MALLOC;

void		rp_language_combo_box_set_lcs		(RpLanguageComboBox *widget, const uint32_t *lcs_array);
uint32_t 	*rp_language_combo_box_get_lcs		(RpLanguageComboBox *widget) G_GNUC_MALLOC;
void		rp_language_combo_box_clear_lcs		(RpLanguageComboBox *widget);

gboolean	rp_language_combo_box_set_selected_lc	(RpLanguageComboBox *widget, uint32_t lc);
uint32_t	rp_language_combo_box_get_selected_lc	(RpLanguageComboBox *widget);

void		rp_language_combo_box_set_force_pal	(RpLanguageComboBox *widget, gboolean forcePAL);
gboolean	rp_language_combo_box_get_force_pal	(RpLanguageComboBox *widget);

G_END_DECLS
