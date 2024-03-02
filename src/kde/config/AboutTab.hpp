/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ITab.hpp"

class AboutTabPrivate;
class AboutTab : public ITab
{
Q_OBJECT

public:
	explicit AboutTab(QWidget *parent = nullptr);
	~AboutTab() override;

private:
	typedef ITab super;
	AboutTabPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(AboutTab);
	Q_DISABLE_COPY(AboutTab)

protected:
	/**
	 * Widget state has changed.
	 * @param event State change event
	 */
	void changeEvent(QEvent *event) final;

	/**
	 * Widget is now visible.
	 * @param event Show event
	 */
	void showEvent(QShowEvent *event) final;

public slots:
	/**
	 * Reset the configuration.
	 */
	void reset(void) final {}		// Nothing to do here.

	/**
	 * Save the configuration.
	 * @param pSettings QSettings object.
	 */
	void save(QSettings *pSettings) final
	{
		// Nothing to do here.
		Q_UNUSED(pSettings)
	}

protected slots:
	/** UpdateChecker slots **/

	/**
	 * An error occurred while trying to retrieve the update version.
	 * TODO: Error code?
	 * @param error Error message
	 */
	void updChecker_error(const QString &error);

	/**
	 * Update version retrieved.
	 * @param updateVersion Update version (64-bit format)
	 */
	void updChecker_retrieved(quint64 updateVersion);
};
