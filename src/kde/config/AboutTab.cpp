/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "AboutTab.hpp"
#include "config.version.h"
#include "git.h"

// librpbase
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// C includes. (C++ namespace)
#include <cassert>

#include "ui_AboutTab.h"
class AboutTabPrivate
{
	public:
		AboutTabPrivate() { }

	private:
		Q_DISABLE_COPY(AboutTabPrivate)

	public:
		Ui::AboutTab ui;

		/**
		 * Initialize the program title text.
		 */
		void initProgramTitleText(void);
};

/** AboutTabPrivate **/

/**
 * Initialize the program title text.
 */
void AboutTabPrivate::initProgramTitleText(void)
{
	// Program icon.
	// TODO: Make a custom icon instead of reusing the system icon.
	// TODO: Fallback for older Qt?
#if QT_VERSION >= 0x040600
	QIcon icon = QIcon::fromTheme(QLatin1String("media-flash"));
	if (!icon.isNull()) {
		// Get the 128x128 icon.
		// TODO: Determine the best size.
		ui.lblLogo->setPixmap(icon.pixmap(128, 128));
	} else {
		// No icon...
		ui.lblLogo->hide();
	}
#endif /* QT_VERSION >= 0x040600 */

	// Useful strings.
	const QString b_start = QLatin1String("<b>");
	const QString b_end = QLatin1String("</b>");
	const QString br = QLatin1String("<br/>\n");

	QString sPrgTitle;
	sPrgTitle.reserve(4096);
	sPrgTitle = b_start +
		AboutTab::tr("ROM Properties Page") + b_end + br +
		AboutTab::tr("Shell Extension") + br + br +
		AboutTab::tr("Version %1")
			.arg(QLatin1String(RP_VERSION_STRING));
#ifdef RP_GIT_VERSION
	sPrgTitle += br + QString::fromUtf8(RP_GIT_VERSION);
# ifdef RP_GIT_DESCRIBE
	sPrgTitle += br + QString::fromUtf8(RP_GIT_DESCRIBE);
# endif /* RP_GIT_DESCRIBE */
#endif /* RP_GIT_VERSION */

	ui.lblTitle->setText(sPrgTitle);
}

/** AboutTab **/

AboutTab::AboutTab(QWidget *parent)
	: super(parent)
	, d_ptr(new AboutTabPrivate())
{
	Q_D(AboutTab);
	d->ui.setupUi(this);

	// Initialize the title and tabs.
	d->initProgramTitleText();
}

AboutTab::~AboutTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void AboutTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(AboutTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void AboutTab::reset(void)
{
	// Nothing to do here.
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void AboutTab::loadDefaults(void)
{
	// Nothing to do here.
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void AboutTab::save(QSettings *pSettings)
{
	// Nothing to do here.
	Q_UNUSED(pSettings)
}
