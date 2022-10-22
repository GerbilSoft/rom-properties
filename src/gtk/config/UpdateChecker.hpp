/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_UPDATECHECKER_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_UPDATECHECKER_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _UpdateCheckerClass	UpdateCheckerClass;
typedef struct _UpdateChecker		UpdateChecker;

#define TYPE_UPDATE_CHECKER            (update_checker_get_type())
#define UPDATE_CHECKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_UPDATE_CHECKER, UpdateChecker))
#define UPDATE_CHECKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_UPDATE_CHECKER, UpdateCheckerClass))
#define IS_UPDATE_CHECKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_UPDATE_CHECKER))
#define IS_UPDATE_CHECKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_UPDATE_CHECKER))
#define UPDATE_CHECKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_UPDATE_CHECKER, UpdateCheckerClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		update_checker_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		update_checker_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

UpdateChecker	*update_checker_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

/**
 * Check for updates.
 * The update check is run asynchronously in a separate thread.
 *
 * Results will be sent as signals:
 * - 'retrieved': Update version retrieved. (guint64 parameter with the version)
 * - 'error': An error occurred. (gchar* parameter with the error message)
 *
 * @param updChecker Update checker
 */
void		update_checker_run		(UpdateChecker *updChecker);

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_UPDATECHECKER_HPP__ */
