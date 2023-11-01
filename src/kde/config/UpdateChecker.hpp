/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * UpdateChecker.hpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QtCore/QObject>

class UpdateChecker : public QObject
{
Q_OBJECT

public:
	explicit UpdateChecker(QObject *parent);

private:
	typedef QObject super;
	Q_DISABLE_COPY(UpdateChecker)

public slots:
	/**
	 * Run the task.
	 * This should be connected to QThread::started().
	 */
	void run(void);

signals:
	/**
	 * An error occurred while trying to retrieve the update version.
	 * TODO: Error code?
	 * @param error Error message
	 */
	void error(const QString &error);

	/**
	 * Update version retrieved.
	 * @param updateVersion Update version (64-bit format)
	 */
	void retrieved(quint64 updateVersion);

	/**
	 * Update version task has completed.
	 * This is called when run() exits, regardless of status.
	 */
	void finished(void);
};
