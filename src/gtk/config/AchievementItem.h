/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AchievementItem.h: Achievement item for GtkDropDown                     *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"
#include "PIMGTYPE.hpp"

#if !GTK_CHECK_VERSION(4,0,0)
#  error GtkDropDown requires GTK4 or later
#endif /* !GTK_CHECK_VERSION(4,0,0) */

G_BEGIN_DECLS

#define RP_TYPE_ACHIEVEMENT_ITEM (rp_achievement_item_get_type())
G_DECLARE_FINAL_TYPE(RpAchievementItem, rp_achievement_item, RP, ACHIEVEMENT_ITEM, GObject)

RpAchievementItem *rp_achievement_item_new		(PIMGTYPE icon, const char *description, GDateTime *unlock_time) G_GNUC_MALLOC;

void		rp_achievement_item_set_icon		(RpAchievementItem *item, PIMGTYPE icon);
PIMGTYPE	rp_achievement_item_get_icon		(RpAchievementItem *item);

void		rp_achievement_item_set_description	(RpAchievementItem *item, const char *description);
const char*	rp_achievement_item_get_description	(RpAchievementItem *item);	// owned by this object

void		rp_achievement_item_set_unlock_time	(RpAchievementItem *item, GDateTime *unlock_time);
GDateTime*	rp_achievement_item_get_unlock_time	(RpAchievementItem *item);

G_END_DECLS
