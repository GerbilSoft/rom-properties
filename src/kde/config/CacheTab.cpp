/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * CacheTab.cpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "CacheTab.hpp"
#include "../MessageSound.hpp"
#include "CacheCleaner.hpp"

// Other rom-properties libraries
#include "librpfile/FileSystem.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;

// Qt includes
#include <QtCore/QThread>

#include "ui_CacheTab.h"
class CacheTabPrivate
{
	public:
		CacheTabPrivate(CacheTab *q)
			: q_ptr(q)
			, thrCleaner(nullptr)
			, ccCleaner(nullptr)
		{ };
		~CacheTabPrivate();

	private:
		CacheTab *const q_ptr;
		Q_DECLARE_PUBLIC(CacheTab)
		Q_DISABLE_COPY(CacheTabPrivate)

	public:
		Ui::CacheTab ui;

		// Cache cleaner object and thread.
		QThread *thrCleaner;
		CacheCleaner *ccCleaner;

	public:
		/**
		 * Enable/disable the UI controls.
		 * @param enable True to enable; false to disable.
		 */
		void enableUiControls(bool enable);

		/**
		 * Clear the specified cache directory.
		 * @param cacheDir Cache directory.
		 */
		void clearCacheDir(CacheCleaner::CacheDir cacheDir);
};

/** CacheTabPrivate **/

CacheTabPrivate::~CacheTabPrivate()
{
	if (thrCleaner && thrCleaner->isRunning()) {
		// Make sure the thread is stopped.
		thrCleaner->quit();
		bool ok = thrCleaner->wait(5000);
		if (!ok) {
			// Thread is hung. Terminate it.
			thrCleaner->terminate();
		}
	}

	delete ccCleaner;
	delete thrCleaner;
}

/**
 * Enable/disable the UI controls.
 * @param enable True to enable; false to disable.
 */
void CacheTabPrivate::enableUiControls(bool enable)
{
	// TODO: Disable the main tab control too?
	ui.lblSysCache->setEnabled(enable);
	ui.btnSysCache->setEnabled(enable);
	ui.lblRpCache->setEnabled(enable);
	ui.btnRpCache->setEnabled(enable);

	// Set the busy cursor if needed.
	Q_Q(CacheTab);
	q->setCursor(enable ? Qt::ArrowCursor : Qt::WaitCursor);
}

/**
 * Clear the specified cache directory.
 * @param cacheDir Cache directory.
 */
void CacheTabPrivate::clearCacheDir(CacheCleaner::CacheDir cacheDir)
{
	if (thrCleaner && thrCleaner->isRunning()) {
		// Cleaning thread is already running.
		return;
	}

	// Reset the progress bar.
	ui.pbStatus->setError(false);
	ui.pbStatus->setRange(0, 1);
	ui.pbStatus->setValue(0);

	// Set the label text.
	Q_Q(CacheTab);
	QString s_label;
	switch (cacheDir) {
		default:
			assert(!"Invalid cache directory specified.");
			q->ccCleaner_error(U82Q(C_("CacheTab", "Invalid cache directory specified.")));
			return;
		case CacheCleaner::CD_System:
			s_label = U82Q(C_("CacheTab", "Clearing the system thumbnail cache..."));
			break;
		case CacheCleaner::CD_RomProperties:
			s_label = U82Q(C_("CacheTab", "Clearing the ROM Properties Page cache..."));
			break;
	}
	ui.lblStatus->setText(s_label);

	// Show the progress controls.
	ui.lblStatus->show();
	ui.pbStatus->show();

	// Disable the buttons until we're done.
	enableUiControls(false);

	// Create the QThread and CacheCleaner if necessary.
	if (!thrCleaner) {
		thrCleaner = new QThread(q);
		thrCleaner->setObjectName(QLatin1String("thrCleaner"));
	}
	if (!ccCleaner) {
		ccCleaner = new CacheCleaner(nullptr);
		ccCleaner->setObjectName(QLatin1String("ccCleaner"));
		ccCleaner->moveToThread(thrCleaner);

		// Status slots.
		QObject::connect(ccCleaner, SIGNAL(progress(int,int,bool)),
				 q, SLOT(ccCleaner_progress(int,int,bool)));
		QObject::connect(ccCleaner, SIGNAL(error(QString)),
				 q, SLOT(ccCleaner_error(QString)));
		QObject::connect(ccCleaner, SIGNAL(cacheIsEmpty(CacheCleaner::CacheDir)),
				 q, SLOT(ccCleaner_cacheIsEmpty(CacheCleaner::CacheDir)));
		QObject::connect(ccCleaner, SIGNAL(cacheCleared(CacheCleaner::CacheDir,unsigned int,unsigned int)),
				 q, SLOT(ccCleaner_cacheCleared(CacheCleaner::CacheDir,unsigned int,unsigned int)));
		QObject::connect(ccCleaner, SIGNAL(finished()),
				 q, SLOT(ccCleaner_finished()));

		// Thread signals.
		QObject::connect(thrCleaner, SIGNAL(started()),
				 ccCleaner, SLOT(run()));
		QObject::connect(ccCleaner, SIGNAL(finished()),
				 thrCleaner, SLOT(quit()));
	}

	// Set the cache directory.
	ccCleaner->setCacheDir(cacheDir);

	// Run the cleaning thread.
	thrCleaner->start();
}

/** CacheTab **/

CacheTab::CacheTab(QWidget *parent)
	: super(parent)
	, d_ptr(new CacheTabPrivate(this))
{
	Q_D(CacheTab);
	d->ui.setupUi(this);

	// Hide the status widgets.
	d->ui.lblStatus->hide();
	d->ui.pbStatus->hide();
}

CacheTab::~CacheTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void CacheTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(CacheTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/** Widget slots **/

/**
 * "Clear the System Thumbnail Cache" button was clicked.
 */
void CacheTab::on_btnSysCache_clicked(void)
{
	Q_D(CacheTab);
	d->clearCacheDir(CacheCleaner::CD_System);
}

/**
 * "Clear the ROM Properties Page Download Cache" button was clicked.
 */
void CacheTab::on_btnRpCache_clicked(void)
{
	Q_D(CacheTab);
	d->clearCacheDir(CacheCleaner::CD_RomProperties);
}

/** CacheCleaner slots **/

/**
 * Cache cleaning task progress update.
 * @param pg_cur Current progress.
 * @param pg_max Maximum progress.
 * @param hasError If true, errors have occurred.
 */
void CacheTab::ccCleaner_progress(int pg_cur, int pg_max, bool hasError)
{
	Q_D(CacheTab);
	if (d->ui.pbStatus->maximum() != pg_max) {
		d->ui.pbStatus->setMaximum(pg_max);
	}
	if (d->ui.pbStatus->hasError() != hasError) {
		d->ui.pbStatus->setError(hasError);
	}
	d->ui.pbStatus->setValue(pg_cur);
}

/**
 * An error occurred while clearing the cache.
 * @param error Error description.
 */
void CacheTab::ccCleaner_error(const QString &error)
{
	Q_D(CacheTab);
	d->ui.pbStatus->setMaximum(1);
	d->ui.pbStatus->setValue(1);
	d->ui.pbStatus->setError(true);
	// tr: Error message template. (Qt version, with formatting)
	const QString qs_msg = U82Q(C_("ConfigDialog", "<b>ERROR:</b> %1")).arg(error);
	d->ui.lblStatus->setText(qs_msg);
	MessageSound::play(QMessageBox::Warning, qs_msg, this);
}

/**
 * Cache directory is empty.
 * @param cacheDir Which cache directory was checked.
 */
void CacheTab::ccCleaner_cacheIsEmpty(CacheCleaner::CacheDir cacheDir)
{
	QString qs_msg;
	switch (cacheDir) {
		default:
			assert(!"Invalid cache directory specified.");
			qs_msg = U82Q(C_("CacheTab", "Invalid cache directory specified."));
			break;
		case CacheCleaner::CD_System:
			qs_msg = U82Q(C_("CacheTab", "System thumbnail cache is empty. Nothing to do."));
			break;
		case CacheCleaner::CD_RomProperties:
			qs_msg = U82Q(C_("CacheTab", "rom-properties cache is empty. Nothing to do."));
			break;
	}

	Q_D(CacheTab);
	d->ui.pbStatus->setMaximum(1);
	d->ui.pbStatus->setValue(1);
	d->ui.lblStatus->setText(qs_msg);
	MessageSound::play(QMessageBox::Information, qs_msg, this);
}

/**
 * Cache was cleared.
 * @param cacheDir Which cache dir was cleared.
 * @param dirErrs Number of directories that could not be deleted.
 * @param fileErrs Number of files that could not be deleted.
 */
void CacheTab::ccCleaner_cacheCleared(CacheCleaner::CacheDir cacheDir, unsigned int dirErrs, unsigned int fileErrs)
{
	Q_D(CacheTab);

	if (dirErrs > 0 || fileErrs > 0) {
		// tr: Error message template. (Qt version, with formatting)
		QString qs_msg = U82Q(C_("ConfigDialog", "<b>ERROR:</b> %1")).arg(
			U82Q(rp_sprintf_p(C_("CacheTab", "Unable to delete %1$u file(s) and/or %2$u dir(s)."),
				fileErrs, dirErrs)));
		d->ui.lblStatus->setText(qs_msg);
		MessageSound::play(QMessageBox::Warning, qs_msg, this);
		return;
	}

	QMessageBox::Icon icon;
	QString qs_msg;
	switch (cacheDir) {
		default:
			assert(!"Invalid cache directory specified.");
			qs_msg = U82Q(C_("CacheTab", "Invalid cache directory specified."));
			icon = QMessageBox::Warning;
			break;
		case CacheCleaner::CD_System:
			qs_msg = U82Q(C_("CacheTab", "System thumbnail cache cleared successfully."));
			icon = QMessageBox::Information;
			break;
		case CacheCleaner::CD_RomProperties:
			qs_msg = U82Q(C_("CacheTab", "rom-properties cache cleared successfully."));
			icon = QMessageBox::Information;
			break;
	}

	d->ui.lblStatus->setText(qs_msg);
	MessageSound::play(icon, qs_msg, this);
}

/**
 * Cache cleaning task has completed.
 * This is called when run() exits, regardless of status.
 */
void CacheTab::ccCleaner_finished(void)
{
	Q_D(CacheTab);
	d->enableUiControls(true);
}
