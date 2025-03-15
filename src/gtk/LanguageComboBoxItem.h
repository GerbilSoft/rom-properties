/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBoxItem.h: Language ComboBox Item (for GtkDropDown)        *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"
#include "PIMGTYPE.hpp"

#if !GTK_CHECK_VERSION(4, 0, 0)
#  error GtkDropDown requires GTK4 or later
#endif /* !GTK_CHECK_VERSION(4, 0, 0) */

G_BEGIN_DECLS

#define RP_TYPE_LANGUAGE_COMBO_BOX_ITEM (rp_language_combo_box_item_get_type())
G_DECLARE_FINAL_TYPE(RpLanguageComboBoxItem, rp_language_combo_box_item, RP, LANGUAGE_COMBO_BOX_ITEM, GObject)

RpLanguageComboBoxItem *rp_language_combo_box_item_new		(PIMGTYPE icon, const char *name, uint32_t lc) G_GNUC_MALLOC;

void		rp_language_combo_box_item_set_icon		(RpLanguageComboBoxItem *item, PIMGTYPE icon);
PIMGTYPE	rp_language_combo_box_item_get_icon		(RpLanguageComboBoxItem *item);

void		rp_language_combo_box_item_set_name		(RpLanguageComboBoxItem *item, const char *name);
const char*	rp_language_combo_box_item_get_name		(RpLanguageComboBoxItem *item);

void		rp_language_combo_box_item_set_lc		(RpLanguageComboBoxItem *item, uint32_t lc);
uint32_t	rp_language_combo_box_item_get_lc		(RpLanguageComboBoxItem *item);

G_END_DECLS
