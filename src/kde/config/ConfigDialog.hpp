/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QDialog>

class ConfigDialogPrivate;
class ConfigDialog : public QDialog
{
Q_OBJECT

public:
	explicit ConfigDialog(QWidget *parent = nullptr);
	~ConfigDialog() override;

private:
	typedef QDialog super;
	ConfigDialogPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(ConfigDialog)
	Q_DISABLE_COPY(ConfigDialog)

protected:
	// State change event. (Used for switching the UI language at runtime.)
	void changeEvent(QEvent *event) final;

	// Event filter for tracking focus.
	bool eventFilter(QObject *watched, QEvent *event) final;

protected slots:
	/**
	 * The current tab has changed.
	 */
	void on_tabWidget_currentChanged(void);

	/**
	 * The "OK" button was clicked.
	 */
	void accept(void) final;

	/**
	 * The "Apply" button was clicked.
	 */
	void apply(void);

	/**
	 * The "Reset" button was clicked.
	 */
	void reset(void);

	/**
	 * The "Defaults" button was clicked.
	 */
	void loadDefaults(void);

	/**
	 * A tab has been modified.
	 */
	void tabModified(void);
};
